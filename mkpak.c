/* mkpak.c - make Quake PAK archives
 * by unsubtract, MIT license */
#define _DEFAULT_SOURCE /* needed for DT_DIR */
#include <dirent.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "pak.h"

static FILE *pakfile = NULL;
static size_t pakptr_header = 0;
static size_t pakptr_data = 0;

static size_t recurse_directory(char path[4096], size_t p, size_t ap, char w) {
    size_t count = 0;

    FILE *fd;
    struct dirent *ent;
    DIR *dp = opendir(path);
    if (dp == NULL) {
        fprintf(stderr, "failed to open folder %s: %s\n", path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    while ((ent = readdir(dp)) != NULL) {
        if (!strncmp(ent->d_name, ".", 2)) continue;
        if (!strncmp(ent->d_name, "..", 3)) continue;
        strncpy(path + p, ent->d_name, 256);

        if (ent->d_type == DT_DIR) {
            strcat(path, "/");
            count += recurse_directory(path, strlen(path), ap, w);
        }

        else {
            if (strlen(path + ap) > sizeof(((file_header*)0)->name) - 1) {
                fprintf(stderr, "file %s has too long of a path name\n", path + ap);
                exit(EXIT_FAILURE);
            }
            ++count;

            if (w) {
                fd = fopen(path, "r");
                if (fd == NULL) {
                    fprintf(stderr, "failed to open file %s: %s\nAborting...", path, strerror(errno));
                    exit(EXIT_FAILURE);
                } else {
                    file_header fh;
                    int c;
                    size_t len = 0;
                    fseek(pakfile, pakptr_data, SEEK_SET);
                    fh.offset = pakptr_data;
                    while ((c = getc(fd)) != EOF) {
                        putc(c, pakfile);
                        ++len;
                    }
                    fclose(fd);
                    pakptr_data += len;
                    fh.size = len;
                    strncpy((char*)fh.name, path + ap, sizeof(fh.name));
                    fseek(pakfile, pakptr_header, SEEK_SET);
                    fwrite(&fh, FILE_HEADER_SZ, 1, pakfile);
                    pakptr_header += FILE_HEADER_SZ;
                }
            }
        }
    }
    closedir(dp);
    return count;
}

static size_t enter_directory(char* path, char buf[4096], char should_write) {
    strncpy(buf, path, 2048);
    strcat(buf, "/");

    return recurse_directory(buf, strlen(buf), strlen(buf), should_write);
}

int main(int argc, char *argv[]) {
    char buf[4096];
    if (argc != 3) {
        fprintf(stderr, "usage: %s [input directory] [output file]\n"\
                "The input directory will become the output file's root directory.\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    pak_header h = {.magic = {'P', 'A', 'C', 'K'}, .offset = PAK_HEADER_SZ};
    h.size = enter_directory(argv[1], buf, 0) * FILE_HEADER_SZ;

    pakfile = fopen(argv[2], "wb");
    if (pakfile == NULL) {
        fprintf(stderr, "failed to open output file %s: %s\n", argv[2], strerror(errno));
        exit(EXIT_FAILURE);
    }

    fwrite(&h, PAK_HEADER_SZ, 1, pakfile);

    pakptr_header = h.offset;
    pakptr_data = PAK_HEADER_SZ+h.size;

    enter_directory(argv[1], buf, 1);

    fclose(pakfile);
    return EXIT_SUCCESS;
}
