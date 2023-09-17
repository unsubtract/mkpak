/* mkpak.c - make Quake PAK archives
 * by unsubtract, MIT license */
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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "pak.h"

static FILE *pakfile = NULL;
static size_t pakptr_header = 0;
static size_t pakptr_data = 0;

typedef struct {
    char buf[4096];
    size_t p;
} pathbuf;

static inline uint32_t htol(uint32_t n);
static int write_entry(pathbuf* pb);
static size_t recurse_directory(pathbuf* pb, char w);
static size_t enter_directory(char* path, char should_write);

static inline uint32_t htol(uint32_t n) {
    return (union {int x; char c;}){1}.c ? n :
           ((n>>24)&0xFF) | ((n<<8)&0xFF0000) |
           ((n>>8)&0xFF00) | ((n<<24)&0xFF000000);
}

static int write_entry(pathbuf* pb) {
    size_t start_p = pakptr_data;
    file_header fh = {0};
    FILE *fd = fopen(pb->buf, "rb");
    if (fd == NULL) {
        fprintf(stderr, "failed to open file %s: %s\n", pb->buf, strerror(errno));
        return -1;
    }

    fputs(pb->buf + pb->p, stdout);

    fseek(pakfile, pakptr_data, SEEK_SET);
    fh.offset = htol(pakptr_data);
    for (int c; (c = getc(fd)) != EOF; ++pakptr_data) {
        if (pakptr_data >= 2147483647) {
            fputs("error: archive has exceeded 2 GiB limit\n", stderr);
            return -1;
        }
        putc(c, pakfile);
    }
    fclose(fd);

    fh.size = htol(pakptr_data - start_p);
    strncpy((char*)fh.name, pb->buf + pb->p, sizeof(fh.name)-1);
    fseek(pakfile, pakptr_header, SEEK_SET);
    fwrite(&fh, sizeof(file_header), 1, pakfile);
    pakptr_header += sizeof(file_header);

    printf(" (%zu bytes)\n", pakptr_data - start_p);
    return 0;
}

static size_t recurse_directory(pathbuf* pb, char w) {
    size_t count = 0;
    size_t path_base = strlen(pb->buf);

    #ifdef _WIN32
    WIN32_FIND_DATA FindFileData;
    strcat(pb->buf, "*"); /* gets removed by a strncpy() later on */
    HANDLE hFind = FindFirstFileA(pb->buf, &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "failed to open directory %s\nAborting...\n", pb->buf);
        exit(EXIT_FAILURE);
    }
    #else
    struct stat st;
    struct dirent *ent;
    DIR *dp = opendir(pb->buf);
    if (dp == NULL) {
        fprintf(stderr, "failed to open directory %s: %s\nAborting...\n",
                pb->buf, strerror(errno));
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
        strncpy(pb->buf + path_base, DIR_FILENAME, 256);

        #ifndef _WIN32
        if (stat(pb->buf, &st)) {
            fprintf(stderr, "failed to stat %s: %s\nAborting...\n",
                    pb->buf, strerror(errno));
            exit(EXIT_FAILURE);
        }
        #endif

        if (IS_DIR) {
            strcat(pb->buf, "/");
            count += recurse_directory(pb, w);
            continue;
        }

        if (strlen(pb->buf + pb->p) > sizeof(((file_header*)0)->name)-1) {
            fprintf(stderr,
                    "path %s is too long (maximum %zu, got %zu)\nAborting...\n",
                    pb->buf + pb->p, sizeof(((file_header*)0)->name)-1, strlen(pb->buf + pb->p));
            exit(EXIT_FAILURE);
        }

        ++count;

        if (w) {
            if (write_entry(pb)) {
                puts("Aborting...");
                exit(EXIT_FAILURE);
            }
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

static size_t enter_directory(char* path, char should_write) {
    pathbuf pb;
    strncpy(pb.buf, path, 2048);
    strcat(pb.buf, "/");
    pb.p = strlen(pb.buf);

    return recurse_directory(&pb, should_write);
}

int main(int argc, char *argv[]) {
    /* catch any possible struct padding */
    assert(sizeof(pak_header) == PAK_HEADER_SZ);
    assert(sizeof(file_header) == FILE_HEADER_SZ);

    if (argc != 3) {
        fprintf(stderr, "usage: %s [input directory] [output file]\n"\
                "The input directory will become the output file's root directory.\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    size_t file_table_size = enter_directory(argv[1], 0)*sizeof(file_header);
    pak_header h = {
        .magic = {'P', 'A', 'C', 'K'}, 
        .offset = htol(sizeof(pak_header)),
        .size = htol(file_table_size)
    };

    pakfile = fopen(argv[2], "wb");
    if (pakfile == NULL) {
        fprintf(stderr, "failed to open output file %s: %s\n", argv[2], strerror(errno));
        exit(EXIT_FAILURE);
    }

    fwrite(&h, sizeof(pak_header), 1, pakfile);

    pakptr_header = sizeof(pak_header);
    pakptr_data = sizeof(pak_header)+file_table_size;

    enter_directory(argv[1], 1);

    fclose(pakfile);
    return EXIT_SUCCESS;
}
