#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void plaintxt_to_life106(FILE *src, FILE *dst)
{
    // make header
    fprintf(dst, "#Life 1.06\n");
    // plain txt file has header starting with '!'
    // skip them all
    char header[256];
    while(1) {
        fgets(header, 256, src);
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
        while(header[x] != '\n' && header[x] != '\0') {
            if(header[x] == 'O') fprintf(dst, "%d %d\n", x, y);
            x++;
        }
        // go to newline
        ++y;
        x = 0;
    }

    
    while ((ch = fgetc(src)) != EOF)
    {
        if (ch == 'O')
        {
            fprintf(dst, "%d %d\n", x, y);
        }
        ++x;
        if (ch == '\n')
        {
            // new line
            ++y;
            x = 0;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        fprintf(stderr, "Usage: ./plainToLife106 newgun\n");
        return EXIT_FAILURE;
    }
    if (argc >= 3) {
        fprintf(stderr, "too many argument\n");
        return EXIT_FAILURE;
    }
    char filename[128];
    int i = 0;
    while (argv[1][i] != '.' && argv[1][i] != '\0') {
        filename[i] = argv[1][i];
        ++i;
    }
    filename[i] = '\0';

    char srcname[200];
    char dstname[200];
    sprintf(srcname, "./data/%s.txt", filename);
    sprintf(dstname, "./data/%s.lif", filename);
    
    FILE *src = fopen(srcname, "r");
    if (src == NULL) {
        fprintf(stderr, "Cannot open file %s\n", srcname);
        exit(EXIT_FAILURE);
    }
    FILE *dst = fopen(dstname, "w");
    if (dst == NULL) {
        fprintf(stderr, "Cannot open file %s\n", dstname);
        exit(EXIT_FAILURE);
    }
    plaintxt_to_life106(src, dst);
    
    fclose(src);
    fclose(dst);

    return EXIT_SUCCESS;
}