/* unpak.c - extract files from Quake PAK archives
 * by unsubtract, MIT license */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#else
#include <sys/stat.h>
#endif

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pak.h"

static FILE *pakfile = NULL;
static size_t pakptr_header = 0;
static size_t pakptr_data = 0;

/* https://stackoverflow.com/a/2182184
 * http://esr.ibiblio.org/?p=5095 */
static inline uint32_t ltoh(uint32_t n) {
    return (*(uint16_t *)"\0\xff" < 0x100) ?
           ((n>>24)&0xff) | ((n<<8)&0xff0000) |
           ((n>>8)&0xff00) | ((n<<24)&0xff000000) :
           n;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s [input file] [output directory]\n"\
                "The input file's root directory will become the output directory.\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    pakfile = fopen(argv[1], "rb");
    if (pakfile == NULL) {
        fprintf(stderr, "failed to open input file %s: %s\n", argv[1], strerror(errno));
        exit(EXIT_FAILURE);
    }

    pak_header h;
    fread(&h, 1, PAK_HEADER_SZ, pakfile);
    h.offset = pakptr_header = ltoh(h.offset), h.size = ltoh(h.size);
    if (memcmp(h.magic, "PACK", 4) || h.size % FILE_HEADER_SZ != 0) {
        fprintf(stderr, "%s is not a PAK file\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    printf("file table offset: %i\nfile table size: %i\n", h.offset, h.size);

    //    

    fclose(pakfile);
    return 0;
}
