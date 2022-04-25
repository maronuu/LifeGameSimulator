#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> // sleep()関数を使う

#include "hash_table.h"

//// CONSTANTS ////
enum fileformat
{
    LIFE106 = 0,
    RLE = 1,
    PLAIN_TXT = 2,
};

//// GLOBAL VARIABLES ////
pthread_mutex_t key_mutex;
int IS_POSE;
int IS_QUIT;
const int FRAME_RATE_LIST_MICRO_SEC[7] = {1000000, 500000, 250000, 100000,
                                          50000, 25000, 10000};
const double FRAME_RATE_PERCENTAGE_LIST_MICRO_SEC[7] = {
    (double)100000 / 1000000, (double)100000 / 500000, (double)100000 / 250000,
    (double)100000 / 100000, (double)100000 / 50000, (double)100000 / 25000,
    (double)100000 / 10000};
int FRAME_RATE_IDX;

//// UTILS ////
// [lower, upper]における一様乱数
double rand_double(const double lower, const double upper)
{
    assert(lower < upper);
    const double div = (double)RAND_MAX / (upper - lower);
    return lower + (rand() / div);
}
// (h, w)が盤面内に収まっているかどうか
int is_range_valid(const int h, const int w, const int height,
                   const int width)
{
    return (0 <= h && h < height && 0 <= w && w < width);
}
// filenameを見てformat_idxを返す
int filename2format(const int len, char filename[len])
{
    int cur = 0;
    while (filename[cur] != '.' && filename[cur] != '\0')
        cur++;
    if (filename[cur] == '\0')
    {
        fprintf(stderr, "Invalid Filename: Must have extension\n");
        exit(EXIT_FAILURE);
    }
    cur++;
    // extension
    if (strcmp(filename + cur, "txt") == 0)
    {
        return PLAIN_TXT;
    }
    else if (strcmp(filename + cur, "rle") == 0)
    {
        return RLE;
    }
    else if (strcmp(filename + cur, "lif") == 0)
    {
        return LIFE106;
    }
    else
    {
        fprintf(stderr, "Invalid Format\n");
        exit(EXIT_FAILURE);
    }
}
int is_equal_cells(const int height, const int width, int cell1[height][width], int cell2[height][width])
{
    int is_ok = 1;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            if (cell1[y][x] != cell2[y][x])
            {
                is_ok = 0;
                break;
            }
        }
        if (!is_ok)
            break;
    }
    return is_ok;
}
void copy_cell(const int height, const int width, int dst[height][width], const int src[height][width])
{
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
            dst[y][x] = src[y][x];
    }
}
//// Board Encoder / Decoder ////
void encode_cell(const int height, const int width, const int codelen, char code[codelen], const int cell[height][width])
{
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            code[y * height + x] = '0' + cell[y][x];
        }
    }
}
void decode_cell(const int height, const int width, const int codelen, const char code[codelen], int cell[height][width])
{
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            cell[y][x] = code[y * height + x] - '0';
        }
    }
}

//// IO ////
// life1.06 format
void read_life106_format(FILE *fp, const int height, const int width,
                         int cell[height][width])
{
    // header of Life1.06 is "#Life 1.06"
    char header[16];
    fgets(header, 16, fp);
    if (strcmp(header, "#Life 1.06\n") != 0)
    {
        fprintf(stderr, "This input file is not Life1.06 Format\n");
        exit(EXIT_FAILURE);
    }
    int x, y;
    while (fscanf(fp, "%d %d", &x, &y) != EOF)
    {
        cell[y][x] = 1;
    }
}
// plain txt format
void read_plaintxt_format(FILE *fp, const int height, const int width,
                          int cell[height][width])
{
    // plain txt file has header starting with '!'
    // skip them all
    char header[128];
    while (1)
    {
        fgets(header, 128, fp);
        if (header[0] == '!')
        {
            if (header[1] == '\0')
                break;
        }
        else
        {
            break;
        }
    }
    char ch;
    int y = 0, x = 0;
    if (header[0] != '!')
    {
        // this is first line of borad
        int cur = 0;
        while (header[cur] != '\n' && header[cur] != '\0')
        {
            if (header[cur] == '.')
                cell[y][x++] = 0;
            else if (header[cur] == 'O')
                cell[y][x++] = 1;
            cur++;
        }
        // fill rest cell in this line with dead
        while (x < width)
        {
            cell[y][x] = 0;
            ++x;
        }
        // go to newline
        ++y;
        x = 0;
    }
    while ((ch = fgetc(fp)) != EOF)
    {
        assert(0 <= x && x < width && 0 <= y && y < height);
        if (ch == '.')
        {
            cell[y][x++] = 0;
        }
        else if (ch == 'O')
        {
            cell[y][x++] = 1;
        }
        else if (ch == '\n')
        {
            // fill rest cell in this line with dead
            while (x < width)
            {
                cell[y][x] = 0;
                ++x;
            }
            // go to newline
            ++y;
            x = 0;
        }
    }
}
// run length encoding format
void read_rle_format(FILE *fp, const int height, const int width,
                     int cell[height][width], FILE *info_fp)
{
    // header
    char header[256];
    while (1)
    {
        fgets(header, 256, fp);
        if (header[0] != '#')
            break; // header end
        if (header[1] == 'C' || header[1] == 'c')
        {
            // comment
            fprintf(info_fp, "Comment: %s\n", header + 2);
        }
        else if (header[1] == 'N')
        {
            // name
            fprintf(info_fp, "Pattern Name: %s\n", header + 2);
        }
        else if (header[1] == 'O')
        {
            // author
            fprintf(info_fp, "Author: %s\n", header + 2);
        }
        else if (header[1] == 'P' || header[1] == 'R' || header[1] == 'r')
        {
            // skip
            continue;
        }
        else
        {
            // invalid header
            fprintf(stderr, "Invalid Header\n");
            exit(EXIT_FAILURE);
        }
    }
    // x, y, rule
    assert(header[0] == 'x');
    int xx, yy;
    char rule[16];
    sscanf(header, "x = %d, y = %d, rule = %s", &xx, &yy, rule);
    printf("x=%d, y=%d\n", xx, yy);
    // header x and y should be within width and height
    assert(0 <= xx && xx <= width && 0 <= yy && yy < height);

    // content
    char numbuf[10];
    numbuf[0] = '1';
    for (int i = 1; i < 10; ++i)
        numbuf[i] = '\0';
    int numidx = 0;

    char ch;
    int y = 0, x = 0;
    while ((ch = fgetc(fp)) != '!')
    {
        if (ch == '\n')
            continue;
        if (ch == '$')
        {
            // fill rest cell in this line with dead
            while (x < width)
            {
                cell[y][x] = 0;
                ++x;
            }
            // go to newline
            int runlen = atoi(numbuf);
            // reset numbuf
            numbuf[0] = '1';
            for (int i = 1; i < 10; ++i)
                numbuf[i] = '\0';
            numidx = 0;
            y += runlen;
            x = 0;
        }
        else if (ch == 'b' || ch == 'o')
        {
            int runlen = atoi(numbuf);
            // reset numbuf
            numbuf[0] = '1';
            for (int i = 1; i < 10; ++i)
                numbuf[i] = '\0';
            numidx = 0;
            // fill
            for (int dx = 0; dx < runlen; ++dx)
            {
                if (ch == 'b')
                    cell[y][x + dx] = 0;
                else if (ch == 'o')
                    cell[y][x + dx] = 1;
                else
                    assert(0); // impossible
            }
            x += runlen;
        }
        else
        {
            numbuf[numidx++] = ch;
        }
    }
}
/*
 グリッドの描画:
 世代情報とグリッドの配列等を受け取り、ファイルポインタに該当する出力にグリッドを描画する
 */
double calc_alive_ratio(const int height, const int width,
                        int cell[height][width])
{
    int alive_cnt = 0;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            if (cell[y][x] == 1)
                alive_cnt++;
        }
    }
    return (double)alive_cnt / (height * width);
}
void print_cells(FILE *fp, const unsigned long long int gen, const int height, const int width,
                 int cell[height][width], const double ratio)
{
    // display generation and ratio
    fprintf(fp, "Generation = %llu, Alive Ratio = %.4f%%\r\n", gen, ratio * 100);

    // top line
    fprintf(fp, "+");
    for (int w = 0; w < width; ++w)
        fprintf(fp, "-");
    fprintf(fp, "+\r\n");

    // board
    for (int y = 0; y < height; ++y)
    {
        fprintf(fp, "|");
        for (int x = 0; x < width; ++x)
        {
            if (cell[y][x] == 1)
            {
                fprintf(fp, "\x1b[31m#");
                fprintf(fp, "\x1b[39m");
            }
            else
                fprintf(fp, " ");
        }
        fprintf(fp, "|\r\n");
    }

    // bottom line
    fprintf(fp, "+");
    for (int w = 0; w < width; ++w)
        fprintf(fp, "-");
    fprintf(fp, "+\r\n");
}
// gen世代目の状況をLife1.06形式で保存する
int save_board_as_life106(const int height, const int width, int cell[height][width],
                          const unsigned long long int gen)
{
    if (gen >= 10000)
        return 1;
    char filename[128];
    sprintf(filename, "./log/lif/gen%04llu.lif", gen);
    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        fprintf(stderr, "Cannot Save at %s\r\n", filename);
        return 1;
    }
    // record
    fprintf(fp, "#Life 1.06\n");
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            if (cell[y][x] == 1)
            {
                fprintf(fp, "%d %d\r\n", x, y);
            }
        }
    }
    fclose(fp);
    return 0;
}
// write image from cell
void write_ppm_image(FILE *fp, const int height, const int width, int cell[height][width], const int scale)
{
    assert(scale > 0);
    // header
    fprintf(fp, "P1\n");
    fprintf(fp, "%d %d\n", scale * width, scale * height);
    for (int y = 0; y < height; ++y)
    {
        for (int cy = 0; cy < scale; ++cy)
        {
            for (int x = 0; x < width; ++x)
            {
                for (int cx = 0; cx < scale; ++cx)
                {
                    if (cell[y][x] == 1)
                    {
                        fprintf(fp, "1");
                    }
                    else
                    {
                        fprintf(fp, "0");
                    }
                }
            }
            fprintf(fp, "\n");
        }
    }
}
// thread function to wait key input
void *wait_key()
{
    char ch;
    while (1)
    {
        ch = getchar();
        pthread_mutex_lock(&key_mutex);
        switch (ch)
        {
        case 'q':
            IS_QUIT = 1;
            printf("\r\nSimulation is over\r\n");
            system("/bin/stty cooked");
            exit(0);
        case 'f':
            IS_POSE = 1;
            printf("Press 'f' to restart\r\n");
            printf("In thread\r\n");
            char key;
            while ((key = getchar()) != 'f')
                ;
            IS_POSE = 0;
            break;
        case 'd': // speed down
            if (FRAME_RATE_IDX < 6)
                FRAME_RATE_IDX++;
            break;
        case 'a': // speed up
            if (FRAME_RATE_IDX > 0)
                FRAME_RATE_IDX--;
            break;
        default:
            break;
        }
        pthread_mutex_unlock(&key_mutex);
    };

    return (void *)ch;
}

//// CONTROLS ////
/*
 ファイルによるセルの初期化:
 生きているセルの座標が記述されたファイルをもとに2次元配列の状態を初期化する
 fp = NULL
 のときは、関数内で適宜定められた初期状態に初期化する。関数内初期値はdefault.lif
 と同じもの
 */
void init_cells(const int height, const int width, int cell[height][width],
                FILE *fp, const int format_idx)
{
    if (fp == NULL)
    {
        // default initialization (at random)
        const double thresh = 0.1;
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                if (rand_double(0.0, 1.0) < thresh)
                    cell[y][x] = 1;
                else
                    cell[y][x] = 0;
            }
        }
    }
    else
    {
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                cell[y][x] = 0;
            }
        }
        if (format_idx == LIFE106)
        {
            read_life106_format(fp, height, width, cell);
        }
        else if (format_idx == RLE)
        {
            FILE *info_fp = fopen("./log/fileinfo.txt", "w");
            read_rle_format(fp, height, width, cell, info_fp);
            fclose(info_fp);
        }
        else if (format_idx == PLAIN_TXT)
        {
            read_plaintxt_format(fp, height, width, cell);
        }
        else
        {
            fprintf(stderr, "Invalid Format\r\n");
            assert(0);
            return;
        }
    }
}
/*
 着目するセルの周辺の生きたセルをカウントする関数
 */
int count_adjacent_cells(int h, int w, const int height, const int width,
                         int cell[height][width])
{
    int cnt = 0;
    for (int dh = -1; dh <= 1; ++dh)
    {
        for (int dw = -1; dw <= 1; ++dw)
        {
            if (dh == 0 && dw == 0)
                continue;
            const int nh = h + dh;
            const int nw = w + dw;
            if (!is_range_valid(nh, nw, height, width))
                continue;
            if (cell[nh][nw] == 1)
                cnt++;
        }
    }
    return cnt;
}
/*
 ライフゲームのルールに基づいて2次元配列の状態を更新する
 */
void update_cells(const int height, const int width, int cell[height][width])
{
    int after_cell[height][width];
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
            after_cell[y][x] = cell[y][x];
    }

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const int cnt = count_adjacent_cells(y, x, height, width, cell);
            if (cell[y][x] == 1)
            {
                // (y, x) is alive
                if (cnt != 2 && cnt != 3)
                    after_cell[y][x] = 0;
            }
            else
            {
                // (y, x) is not alive
                if (cnt == 3)
                    after_cell[y][x] = 1;
            }
        }
    }
    // update
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
            cell[y][x] = after_cell[y][x];
    }
}

int main(int argc, char **argv)
{
    system(
        "/bin/stty raw onlcr"); // enterを押さなくてもキー入力を受け付けるようになる
    // display frame rate
    FRAME_RATE_IDX = 3;
    // set seed
    srand(time(NULL));
    FILE *fp = stdout;
    const int height = 40;
    const int width = 70;

    int cell[height][width];
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            cell[y][x] = 0;
        }
    }

    /* ファイルを引数にとるか、ない場合はデフォルトの初期値を使う */
    if (argc > 2)
    {
        fprintf(stderr, "usage: %s [filename for init]\n", argv[0]);
        system("/bin/stty cooked");
        return EXIT_FAILURE;
    }
    else if (argc == 2)
    {
        // get format idx
        const int format_idx = filename2format(strlen(argv[1]), argv[1]);
        FILE *lgfile;
        if ((lgfile = fopen(argv[1], "r")) != NULL)
        {
            init_cells(height, width, cell, lgfile,
                       format_idx); // ファイルによる初期化
        }
        else
        {
            fprintf(stderr, "cannot open file %s\r\n", argv[1]);
            system("/bin/stty cooked");
            return EXIT_FAILURE;
        }
        fclose(lgfile);
    }
    else
    {
        init_cells(height, width, cell, NULL, -1);
    }
    // int fast[height][width];
    // copy_cell(height, width, fast, cell);
    int is_cycle_detected = 0;

    double ratio = calc_alive_ratio(height, width, cell);
    print_cells(fp, 0, height, width, cell, ratio);
    usleep(FRAME_RATE_LIST_MICRO_SEC[FRAME_RATE_IDX]);
    fprintf(fp, "\e[%dA",
            height + 3); // height+3 の分、カーソルを上に戻す(壁2、表示部1)

    // thread initialization
    pthread_t wait_key_thread;
    int thread_status =
        pthread_create(&wait_key_thread, NULL, (void *)wait_key, NULL);
    if (thread_status != 0)
    {
        fprintf(stderr, "Cannot create thread\n");
        return EXIT_FAILURE;
    }
    // initialize mutex
    pthread_mutex_init(&key_mutex, NULL);

    // csv file to describe statistics on each generation
    FILE *stats_fp = fopen("./log/stats.csv", "w");
    fprintf(stats_fp, "generation,alive_ratio\n");
    fprintf(stats_fp, "%d,%.04f\n", 0, ratio * 100);
    FILE *summary_fp = fopen("./log/summary.txt", "w");
    fprintf(summary_fp, "=== Summary ===\n");

    // record
    const int hash_table_size = 123456;
    HashTable *tbl = hash_table_create(hash_table_size);
    char cell_code[height * width];
    encode_cell(height, width, height * width, cell_code, cell);
    hash_table_insert(tbl, cell_code, 0ull);

    /* 世代を進める*/
    for (unsigned long long gen = 1;; gen++)
    {
        system("clear");
        printf("Generation is %llu\r\n", gen);
        printf("Press [q] to end simulation\r\n");
        printf("Press [f] to pose\r\n");
        printf("Speed: [a] <<");
        for (int f_id = 0; f_id < 7; ++f_id)
        {
            if (f_id == FRAME_RATE_IDX)
            {
                printf(" \e[32m[[%.1fx]]\e[39m ",
                       FRAME_RATE_PERCENTAGE_LIST_MICRO_SEC[f_id]);
            }
            else
            {
                printf(" %.1fx ", FRAME_RATE_PERCENTAGE_LIST_MICRO_SEC[f_id]);
            }
        }
        printf("<< [d]\r\n");

        if (IS_QUIT)
        {
            break;
        }
        // check pose
        while (IS_POSE)
        {
            system("clear");
            printf("Generation is %lld\r\n", gen);
            printf("Posed...........\r\n");
            printf("Press [f] to restart\r\n");
            printf("Speed: <<");
            for (int f_id = 0; f_id < 7; ++f_id)
            {
                if (f_id == FRAME_RATE_IDX)
                {
                    printf(" \e[32m[[%.1fx]]\e[39m ",
                           FRAME_RATE_PERCENTAGE_LIST_MICRO_SEC[f_id]);
                }
                else
                {
                    printf(" %.1fx ",
                           FRAME_RATE_PERCENTAGE_LIST_MICRO_SEC[f_id]);
                }
            }
            printf("<<\r\n");
            print_cells(fp, gen, height, width, cell, ratio);
            usleep(50000);
        }
        // update slow and fast
        update_cells(height, width, cell);
        // update_cells(height, width, fast); // fast is updated twice
        // update_cells(height, width, fast);

        // cycle check
        encode_cell(height, width, height * width, cell_code, cell);
        unsigned long long last_visited;
        if (!is_cycle_detected && (last_visited = hash_table_get(tbl, cell_code)) != -1)
        {
            // cycle !
            is_cycle_detected = 1;
            fprintf(summary_fp, "Cycle: Yes\n");
            fprintf(summary_fp, "Cycle Starts At Generation %llu\n", last_visited);
        }

        // record to table
        hash_table_insert(tbl, cell_code, gen);
        // fast and slow pointer cycle detection
        // if (is_cycle_detected <= 1 && is_equal_cells(height, width, cell, fast)) {
        //     // detected
        //     if (is_cycle_detected == 0) {
        //         fprintf(summary_fp, "Cycle: Yes\n");
        //         fprintf(summary_fp, "Cycle Detected At Gen %llu\n", gen);
        //         last_meet_gen = gen;
        //     } else if (is_cycle_detected == 1) {
        //         fprintf(summary_fp, "Cycle Length: %llu\n", gen - last_meet_gen);
        //     }
        //     is_cycle_detected++;
        // }

        ratio = calc_alive_ratio(height, width, cell);
        print_cells(fp, gen, height, width, cell, ratio);
        // logging
        fprintf(stats_fp, "%llu,%.04f\n", gen, ratio * 100);
        usleep(FRAME_RATE_LIST_MICRO_SEC[FRAME_RATE_IDX]);
        fprintf(fp, "\e[%dA", height + 5); // bring the cursor back to the top
        if (gen % 100 == 0 && gen < 10000)
        {
            save_board_as_life106(height, width, cell, gen);
        }
        if ((gen <= 100) || (gen > 100 && gen % 20 == 0))
        {
            // save image
            char image_name[128];
            sprintf(image_name, "./log/image/gen%.04lld.pbm", gen);
            FILE *image_fp = fopen(image_name, "w");
            write_ppm_image(image_fp, height, width, cell, 8);
            fclose(image_fp);
        }
    }
    system("clear");
    system("/bin/stty cooked");

    pthread_mutex_destroy(&key_mutex);

    printf("Simulation is Over!\r\n");
    fclose(stats_fp);
    fclose(summary_fp);

    return EXIT_SUCCESS;
}
