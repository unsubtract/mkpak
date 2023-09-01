#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include <dirent.h>

#include "pak.h"

static void fail(const char* str);

static FILE *pakfile = NULL;
static char full_filename[512];
static size_t filename_stack[256];
static size_t filename_sp = 0;

static void fail(const char* str) {
    perror(str);
    exit(EXIT_FAILURE);
}

void enter_directory(char *path) {
    DIR *indir = opendir(path);
    struct dirent *infe = NULL;
    FILE *infd = NULL;

    while ((infe = readdir(indir)) != NULL) {
        if (!strncmp(infe->d_name, ".", 2)) continue;
        if (!strncmp(infe->d_name, "..", 3)) continue;
        if (infe->d_type == DT_DIR) strcat(infe->d_name, "/");
        strncpy(full_filename + filename_stack[filename_sp - 1], infe->d_name, 256);
        //printf("%lu\n", filename_stack[filename_sp]);
        filename_stack[filename_sp++] = strlen(full_filename);
        if (infe->d_type == DT_DIR) enter_directory(full_filename);
        else {
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
        filename_sp--;
    }
    closedir(indir);
}

int main(/*int argc, char *argv[]*/) {
    pakfile = fopen("out.pak", "wb");
    if (pakfile == NULL) fail("failed to open output file");

    strncpy(full_filename, "./pak0/", 256);
    filename_stack[filename_sp++] = strlen(full_filename);

    //pak_header h = {.magic = {'P', 'A', 'C', 'K'}};
    //file_header fh;
    fseek(pakfile, PAK_HEADER_SZ, SEEK_SET);

    enter_directory("./pak0/");

    fclose(pakfile);
    return EXIT_SUCCESS;
}
