#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

//// CONSTANTS ////
enum fileformat {
    LIFE106 = 0,
    RLE = 1,
    PLAIN_TXT = 2,
};
//// UTILS ////
// [lower, upper]における一様乱数
double rand_double(const double lower, const double upper) {
    assert(lower < upper);
    const double div = (double)RAND_MAX / (upper - lower);
    return lower + (rand() / div);
}
//// IO ////
// life1.06 format
void read_life106_format(FILE *fp, const int height, const int width,
                         int cell[height][width]) {
    // header of Life1.06 is "#Life 1.06"
    char header[16];
    fgets(header, 16, fp);
    if(strcmp(header, "#Life 1.06\n") != 0) {
        fprintf(stderr, "This input file is not Life1.06 Format\n");
        exit(EXIT_FAILURE);
    }
    int x, y;
    while(fscanf(fp, "%d %d", &x, &y) != EOF) {
        cell[y][x] = 1;
    }
}
// plain txt format
void read_plaintxt_format(FILE *fp, const int height, const int width,
                          int cell[height][width]) {
    // plain txt file has header starting with '!'
    // skip them all
    char header[128];
    while(1) {
        fgets(header, 128, fp);
        if(header[0] == '!') {
            if(header[1] == '\0')
                break;
        } else {
            break;
        }
    }
    char ch;
    int y = 0, x = 0;
    if(header[0] != '!') {
        // this is first line of borad
        int cur = 0;
        while(header[cur] != '\n' && header[cur] != '\0') {
            if(header[cur] == '.')
                cell[y][x++] = 0;
            else if(header[cur] == 'O')
                cell[y][x++] = 1;
            cur++;
        }
        // fill rest cell in this line with dead
        while(x < width) {
            cell[y][x] = 0;
            ++x;
        }
        // go to newline
        ++y;
        x = 0;
    }
    while((ch = fgetc(fp)) != EOF) {
        assert(0 <= x && x < width && 0 <= y && y < height);
        if(ch == '.') {
            cell[y][x++] = 0;
        } else if(ch == 'O') {
            cell[y][x++] = 1;
        } else if(ch == '\n') {
            // fill rest cell in this line with dead
            while(x < width) {
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
                     int cell[height][width], FILE *info_fp) {
    // header
    char header[256];
    while(1) {
        fgets(header, 256, fp);
        if(header[0] != '#')
            break; // header end
        if(header[1] == 'C' || header[1] == 'c') {
            // comment
            fprintf(info_fp, "Comment: %s\n", header + 2);
        } else if(header[1] == 'N') {
            // name
            fprintf(info_fp, "Pattern Name: %s\n", header + 2);
        } else if(header[1] == 'O') {
            // author
            fprintf(info_fp, "Author: %s\n", header + 2);
        } else if(header[1] == 'P' || header[1] == 'R' || header[1] == 'r') {
            // skip
            continue;
        } else {
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
    // header x and y should be within width and height
    assert(0 <= xx && xx <= width && 0 <= yy && yy < height);

    // content
    char numbuf[10];
    numbuf[0] = '1';
    for(int i = 1; i < 10; ++i)
        numbuf[i] = '\0';
    int numidx = 0;

    char ch;
    int y = 0, x = 0;
    while((ch = fgetc(fp)) != '!') {
        if(ch == '\n')
            continue;
        if(ch == '$') {
            // fill rest cell in this line with dead
            while(x < width) {
                cell[y][x] = 0;
                ++x;
            }
            // go to newline
            int runlen = atoi(numbuf);
            // reset numbuf
            numbuf[0] = '1';
            for(int i = 1; i < 10; ++i)
                numbuf[i] = '\0';
            numidx = 0;
            y += runlen;
            x = 0;
        } else if(ch == 'b' || ch == 'o') {
            int runlen = atoi(numbuf);
            // reset numbuf
            numbuf[0] = '1';
            for(int i = 1; i < 10; ++i)
                numbuf[i] = '\0';
            numidx = 0;
            // fill
            for(int dx = 0; dx < runlen; ++dx) {
                if(ch == 'b')
                    cell[y][x + dx] = 0;
                else if(ch == 'o')
                    cell[y][x + dx] = 1;
                else
                    assert(0); // impossible
            }
            x += runlen;
        } else {
            numbuf[numidx++] = ch;
        }
    }
}
void init_cells(const int height, const int width, int cell[height][width],
                FILE *fp, const int format_idx) {
    if(fp == NULL) {
        // default initialization (at random)
        const double thresh = 0.1;
        for(int y = 0; y < height; ++y) {
            for(int x = 0; x < width; ++x) {
                if(rand_double(0.0, 1.0) < thresh)
                    cell[y][x] = 1;
                else
                    cell[y][x] = 0;
            }
        }
    } else {
        for(int y = 0; y < height; ++y) {
            for(int x = 0; x < width; ++x) {
                cell[y][x] = 0;
            }
        }
        if(format_idx == LIFE106) {
            read_life106_format(fp, height, width, cell);
        } else if(format_idx == RLE) {
            FILE *info_fp = fopen("./log/fileinfo_test.txt", "w");
            read_rle_format(fp, height, width, cell, info_fp);
            fclose(info_fp);
        } else if(format_idx == PLAIN_TXT) {
            read_plaintxt_format(fp, height, width, cell);
        } else {
            fprintf(stderr, "Invalid Format\r\n");
            assert(0);
            return;
        }
    }
}
char *cut_tail_n(char *txt, const int n) {
    const int len = strlen(txt);
    txt[len-n] = '\0';
    return txt;
}

int main(int argc, char *argv[]) {
    if (argc >= 3) {
        fprintf(stderr, "Too Many Arguments\n");
        return EXIT_FAILURE;
    }
    const int height = 40;
    const int width = 70;
    int cell_lif[height][width];
    int cell_rle[height][width];
    int cell_txt[height][width];
    
    FILE *test_data_names_fp;
    if (argc == 1) {
        test_data_names_fp = fopen("test_data_names.txt", "r");
    } else if (argc == 2) {
        test_data_names_fp = fopen(argv[1], "r");
    } else {
        fprintf(stderr, "Fatal: argc is inconsistent\n");
        return EXIT_FAILURE;
    }
    char filename[128];
    while (fgets(filename, 128, test_data_names_fp) != NULL) {
        // file path
        char lif_name[138];
        char rle_name[138];
        char txt_name[138];
        char *filebody = cut_tail_n(filename, 1);
        sprintf(lif_name, "./data/%s.lif", filebody);
        sprintf(rle_name, "./data/%s.rle", filebody);
        sprintf(txt_name, "./data/%s.txt", filebody);
        // open files
        FILE *lif_fp = fopen(lif_name, "r");
        if (lif_fp == NULL) {
            printf("Cannot open file\n");
            exit(EXIT_FAILURE);
        }
        FILE *rle_fp = fopen(rle_name, "r");
        if (rle_fp == NULL) {
            printf("Cannot open file\n");
            exit(EXIT_FAILURE);
        }
        FILE *txt_fp = fopen(txt_name, "r");
        if (txt_fp == NULL) {
            printf("Cannot open file\n");
            exit(EXIT_FAILURE);
        }
        // initialize
        init_cells(height, width, cell_lif, lif_fp, LIFE106);
        init_cells(height, width, cell_rle, rle_fp, RLE);
        init_cells(height, width, cell_txt, txt_fp, PLAIN_TXT);
        // close files
        fclose(lif_fp);
        fclose(rle_fp);
        fclose(txt_fp);

        int is_ok = 1;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (!(cell_lif[y][x] == cell_rle[y][x] && cell_rle[y][x] == cell_txt[y][x])) {
                    is_ok = 0;
                    break;
                }
            }
            if (!is_ok) break;
        }

        if (is_ok) {
            printf("\x1b[32mSUCCESS\x1b[39m: %s\n", filename);
        } else {
            printf("\x1b[31mFAILED\x1b[39m: %s\n", filename);
            return EXIT_FAILURE;
        }
    }
    printf("\x1b[32mAll tests passed.\x1b[39m\n");
    
    return EXIT_SUCCESS;
}