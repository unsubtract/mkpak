/* mkpak.c - make Quake PAK archives
 * by unsubtract, MIT license */
// TODO: warn for >2GB files
// TODO: windows unicode support
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define DIR_FILENAME (FindFileData.cFileName)
#define IS_DIR (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)

#else
#include <dirent.h>
#include <sys/stat.h>
#define DIR_FILENAME (ent->d_name)
#define IS_DIR (S_ISDIR(st.st_mode))
#endif

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "pak.h"

static FILE *pakfile = NULL;
static size_t pakptr_header = 0;
static size_t pakptr_data = 0;

static inline uint32_t htol(uint32_t n) {
    return (union {int x; char c;}){1}.c ? n :
           ((n>>24)&0xFF) | ((n<<8)&0xFF0000) |
           ((n>>8)&0xFF00) | ((n<<24)&0xFF000000);
}

static size_t recurse_directory(char path[4096], size_t p, size_t ap, char w) {
    size_t count = 0;
    FILE *fd;
    file_header fh;

    #ifdef _WIN32
    WIN32_FIND_DATA FindFileData;
    strcat(path, "*"); /* gets removed by a strncpy() later on */
    HANDLE hFind = FindFirstFileA(path, &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "failed to open directory %s\nAborting...\n", path);
        exit(EXIT_FAILURE);
    }
    #else
    struct stat st;
    struct dirent *ent;
    DIR *dp = opendir(path);
    if (dp == NULL) {
        fprintf(stderr, "failed to open directory %s: %s\nAborting...\n",
                path, strerror(errno));
        exit(EXIT_FAILURE);
    }
    #endif

    #ifdef _WIN32
    do {
    #else
    while ((ent = readdir(dp)) != NULL) {
    #endif

        if (!strncmp(DIR_FILENAME, ".", 2)) continue;
        if (!strncmp(DIR_FILENAME, "..", 3)) continue;
        strncpy(path + p, DIR_FILENAME, 256);

        if (strlen(path + ap) > sizeof(fh.name)-1) {
            fprintf(stderr,
                    "path %s is too long (maximum %zu)\nAborting...\n",
                    path + ap, sizeof(fh.name)-1);
            exit(EXIT_FAILURE);
        }

        #ifndef _WIN32
        if (stat(path, &st)) {
            fprintf(stderr, "failed to stat %s: %s\nAborting...\n",
                    path, strerror(errno));
            exit(EXIT_FAILURE);
        }
        #endif

        if (IS_DIR) {
            strcat(path, "/");
            count += recurse_directory(path, strlen(path), ap, w);
            continue;
        }

        ++count;

        if (w) {
            size_t len = 0;
            fd = fopen(path, "rb");
            if (fd == NULL) {
                fprintf(stderr, "failed to open file %s: %s\nAborting...\n",
                        path, strerror(errno));
                exit(EXIT_FAILURE);
            }
            fputs(path + ap, stdout);
            fseek(pakfile, pakptr_data, SEEK_SET);
            fh.offset = htol(pakptr_data);
            for (int c; (c = getc(fd)) != EOF; ++len) putc(c, pakfile);
            fclose(fd);
            pakptr_data += len;
            fh.size = htol(len);
            strncpy((char*)fh.name, path + ap, sizeof(fh.name) - 1);
            fseek(pakfile, pakptr_header, SEEK_SET);
            fwrite(&fh, FILE_HEADER_SZ, 1, pakfile);
            pakptr_header += FILE_HEADER_SZ;
            printf(" (%zu bytes)\n", len);
        }

    #ifdef _WIN32 /* close do-while loop */
    } while (FindNextFileA(hFind, &FindFileData));
    FindClose(hFind);
    #else /* close while loop */
    }
    closedir(dp);
    #endif
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

    size_t file_table_size = enter_directory(argv[1], buf, 0) * FILE_HEADER_SZ;
    pak_header h = {
        .magic = {'P', 'A', 'C', 'K'}, 
        .offset = htol(PAK_HEADER_SZ),
        .size = htol(file_table_size)
    };

    pakfile = fopen(argv[2], "wb");
    if (pakfile == NULL) {
        fprintf(stderr, "failed to open output file %s: %s\n", argv[2], strerror(errno));
        exit(EXIT_FAILURE);
    }

    fwrite(&h, PAK_HEADER_SZ, 1, pakfile);

    pakptr_header = PAK_HEADER_SZ;
    pakptr_data = PAK_HEADER_SZ+file_table_size;

    enter_directory(argv[1], buf, 1);

    fclose(pakfile);
    return EXIT_SUCCESS;
}
