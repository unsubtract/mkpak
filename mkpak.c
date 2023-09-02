#ifdef __WIN32__
#include <windows.h>
#else
#define _DEFAULT_SOURCE /* needed for DT_DIR */
#include <dirent.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include "pak.h"

static size_t enter_directory(char* path, int should_write);

static FILE *pakfile = NULL;
static size_t pakptr_header = 0;
static size_t pakptr_data = 0;

static size_t enter_directory(char* path, int should_write) {
    size_t count = 0;
    static char current_path[4096] = {0};
    static size_t path_stack[2048];
    static size_t path_sp = 0;
    static size_t archived_path_p = 0;

    if (path_sp == 0) {
        strncpy(current_path, path, 2047);
        strcat(current_path, "/");
        path_stack[path_sp++] = archived_path_p = strlen(current_path);
    }

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

        if (ent->d_type == DT_DIR) strcat(ent->d_name, "/");
        strncpy(current_path + path_stack[path_sp - 1], ent->d_name, 256);
        path_stack[path_sp++] = strlen(current_path);
        if (ent->d_type == DT_DIR) count += enter_directory(current_path, should_write);

        else {
            if (strlen(current_path + archived_path_p) > sizeof(((file_header*)0)->name) - 1) {
                fprintf(stderr, "file %s has too long of a path name\n", current_path + archived_path_p);
                exit(EXIT_FAILURE);
            }
            ++count;

            if (should_write) {
                fd = fopen(current_path, "r");
                if (fd == NULL) {
                    fprintf(stderr, "failed to open file %s: %s\nAborting...", current_path, strerror(errno));
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
                    strncpy((char*)fh.name, current_path + archived_path_p, sizeof(fh.name));
                    fseek(pakfile, pakptr_header, SEEK_SET);
                    fwrite(&fh, FILE_HEADER_SZ, 1, pakfile);
                    pakptr_header += FILE_HEADER_SZ;
                }
            }
        }
        --path_sp;
    }
    closedir(dp);
    return count;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s [input directory] [output file]\n"\
                "The input directory will be the root directory of the output file.\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    pak_header h = {.magic = {'P', 'A', 'C', 'K'}, .offset = PAK_HEADER_SZ};
    h.size = enter_directory(argv[1], 0) * FILE_HEADER_SZ;

    pakfile = fopen(argv[2], "wb");
    if (pakfile == NULL) {
        fprintf(stderr, "failed to open output file %s: %s\n", argv[2], strerror(errno));
        exit(EXIT_FAILURE);
    }

    fwrite(&h, PAK_HEADER_SZ, 1, pakfile);

    pakptr_header = h.offset;
    pakptr_data = PAK_HEADER_SZ+h.size;

    enter_directory(argv[1], 1);

    fclose(pakfile);
    return EXIT_SUCCESS;
}
