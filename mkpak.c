#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include <dirent.h>

#include "pak.h"

static void fail(const char* str);

static void fail(const char* str) {
    perror(str);
    exit(EXIT_FAILURE);
}

int enter_directory(char *path) {
    ;
}

int main(/*int argc, char *argv[]*/) {
    DIR *indir = opendir("./pak0");
    if (indir == NULL) fail("failed to open folder");
    FILE *pakfile = fopen("out.pak", "wb");
    if (pakfile == NULL) fail("failed to open output file");
    struct dirent *infe = NULL;
    FILE *infd = NULL;
    char full_filename[512];

    pak_header h = {.magic = {'P', 'A', 'C', 'K'}};
    file_header fh;
    fseek(pakfile, PAK_HEADER_SZ, SEEK_SET);

    while ((infe = readdir(indir)) != NULL) {
        if (!strncmp(infe->d_name, ".", 2)) continue;
        if (!strncmp(infe->d_name, "..", 3)) continue;
        strncpy(full_filename, "./pak0/", 256);
        strncat(full_filename, infe->d_name, 256);
        if (infe->d_type == DT_DIR) continue;
        infd = fopen(full_filename, "r");
        if (infd == NULL) {
            fprintf(stderr, "failed to open %s: %s\n", full_filename, strerror(errno));
        } else {
            printf("opened %s\n", full_filename);
            int c;
            size_t fsize = 0;
            while ((c = getc(infd)) != EOF) {
                putc(c, pakfile);
                ++fsize;
            }
            fclose(infd);
        }
    }
}
