/* mkpak.c - make Quake PAK archives
 * by unsubtract, MIT license */
// TODO: warn for >2GB files
// TODO: windows unicode support
#ifdef _WIN32
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

/* https://stackoverflow.com/a/2182184
 * http://esr.ibiblio.org/?p=5095 */
static inline uint32_t tolittle(uint32_t n) {
    return (*(uint16_t *)"\0\xff" < 0x100) ?
           ((n>>24)&0xff) | ((n<<8)&0xff0000) |
           ((n>>8)&0xff00) | ((n<<24)&0xff000000) :
           n;
}

static size_t recurse_directory(char path[4096], size_t p, size_t ap, char w) {
    size_t count = 0;

    FILE *fd;
    #ifdef _WIN32
    WIN32_FIND_DATA FindFileData;
    strcat(path, "*"); /* gets removed by a strncpy() later on */
    HANDLE hFind = FindFirstFileA(path, &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "failed to open folder %s\n", path);
        exit(EXIT_FAILURE);
    }
    #else
    struct stat st;
    struct dirent *ent;
    DIR *dp = opendir(path);
    if (dp == NULL) {
        fprintf(stderr, "failed to open folder %s: %s\n", path, strerror(errno));
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
        #ifndef _WIN32
        if (stat(path, &st) < 0) {
            fprintf(stderr, "failed to stat %s: %s\n", path, strerror(errno));
            exit(EXIT_FAILURE);
        }
        #endif

        if (IS_DIR) {
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
                fd = fopen(path, "rb");
                if (fd == NULL) {
                    #ifdef _WIN32
                    fprintf(stderr, "failed to open file %s\nAborting...", path);
                    #else
                    fprintf(stderr, "failed to open file %s: %s\nAborting...", path, strerror(errno));
                    #endif
                    exit(EXIT_FAILURE);
                } else {
                    file_header fh;
                    int c;
                    size_t len = 0;
                    fseek(pakfile, pakptr_data, SEEK_SET);
                    fh.offset = tolittle(pakptr_data);
                    while ((c = getc(fd)) != EOF) {
                        putc(c, pakfile);
                        ++len;
                    }
                    fclose(fd);
                    pakptr_data += len;
                    fh.size = tolittle(len);
                    strncpy((char*)fh.name, path + ap, sizeof(fh.name) - 1);
                    fseek(pakfile, pakptr_header, SEEK_SET);
                    fwrite(&fh, FILE_HEADER_SZ, 1, pakfile);
                    pakptr_header += FILE_HEADER_SZ;
                }
            }
        }
    #ifdef _WIN32
    } while (FindNextFileA(hFind, &FindFileData));
    FindClose(hFind);
    #else
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
        .offset = tolittle(PAK_HEADER_SZ),
        .size = tolittle(file_table_size)
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
