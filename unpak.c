/* unpak.c - extract files from Quake PAK archives
 * by unsubtract, MIT license */
// TODO: windows unicode support

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#else
#include <sys/stat.h>
#endif

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pak.h"

static inline uint32_t ltoh(uint32_t n);
static int mkdir_p(char* path);

static inline uint32_t ltoh(uint32_t n) {
    return (union {int x; char c;}){1}.c ? n :
           ((n>>24)&0xFF) | ((n<<8)&0xFF0000) |
           ((n>>8)&0xFF00) | ((n<<24)&0xFF000000);
}

static int mkdir_p(char* path) {
#ifdef _WIN32
    for (char *p = strpbrk(path + 1, "/\\"); p; p = strpbrk(p + 1, "/\\")) {
        *p = '\0';
        if (!CreateDirectoryA(path, NULL)) {
            if (GetLastError() != ERROR_ALREADY_EXISTS) {
#else
    for (char *p = strchr(path + 1, '/'); p; p = strchr(p + 1, '/')) {
        *p = '\0';
        if (mkdir(path, S_IRWXU)) {
            if (errno != EEXIST) {
#endif
                *p = '/';
                return errno;
            }
        }
        *p = '/';
    }
    return 0;
}

int main(int argc, char *argv[]) {
    /* catch any possible struct padding */
    assert(sizeof(pak_header) == PAK_HEADER_SZ);
    assert(sizeof(file_header) == FILE_HEADER_SZ);

    char path[4096];
    size_t path_p = 0;
    FILE *pakfile = NULL;
    size_t pakptr_header = 0;

    if (argc != 3) {
        fprintf(stderr, "usage: %s [input file] [output directory]\n"\
                "The input file's root directory will become the output directory.\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    strncpy(path, argv[2], 4000);
    strcat(path, "/");
    path_p = strlen(path);
    pakfile = fopen(argv[1], "rb");
    if (pakfile == NULL) {
        fprintf(stderr, "failed to open input file %s: %s\n", argv[1], strerror(errno));
        exit(EXIT_FAILURE);
    }

    pak_header h;
    fread(&h, 1, sizeof(pak_header), pakfile);
    h.offset = pakptr_header = ltoh(h.offset), h.size = ltoh(h.size);
    if (memcmp(h.magic, "PACK", 4) || h.size % sizeof(file_header) != 0) {
        fprintf(stderr, "%s is not a PAK file\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    file_header fh;
    FILE *fd;
    while (pakptr_header < h.offset + h.size) {
        uint8_t buf[4096];
        size_t total = 0;

        fseek(pakfile, pakptr_header, SEEK_SET);
        fread(&fh, 1, sizeof(file_header), pakfile);
        pakptr_header = ftell(pakfile);
        fh.offset = ltoh(fh.offset), fh.size = ltoh(fh.size);

        strcpy(path + path_p, (char*)fh.name);

        reopen_file:
        if ((fd = fopen(path, "wb")) == NULL) {
            if (errno == ENOENT) {
                mkdir_p(path);
                goto reopen_file;
            }
            fprintf(stderr, "failed to open %s: %s\n"\
                            "skipping file...\n", path, strerror(errno));
            continue;
        }

        fputs((char*)fh.name, stdout);
        fseek(pakfile, fh.offset, SEEK_SET);
        while (total < fh.size && !feof(pakfile)) {
            size_t n = fread(buf, 1, fh.size - total > sizeof(buf) ?
                             sizeof(buf) : fh.size - total, pakfile);
            fwrite(buf, 1, n, fd);
            total += n;
        }
        printf(" (%zu bytes)\n", total);
    }

    fclose(pakfile);
    return 0;
}
