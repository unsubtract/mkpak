#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <sys/stat.h>


#include "pak.h"

static void fail(const char* str);

#define BUFSZ_FACTOR 4
static uint8_t buffer[FILE_HEADER_SZ*BUFSZ_FACTOR];

static void fail(const char* str) {
    fputs(str, stderr);
    exit(EXIT_FAILURE);
}

int main(/*int argc, char *argv[]*/) {
    pak_header h = {.magic = {'P', 'A', 'C', 'K'}};
    FILE *pakfile = fopen("PAK0.PAK", "rb");
    if (pakfile == NULL) {
        perror("could not open file");
        exit(EXIT_FAILURE);
    }

    fread(buffer, 1, PAK_HEADER_SZ, pakfile);
    for (size_t i = 0; i < sizeof(h.magic); ++i) {
        if (buffer[i] != h.magic[i]) fail("not a PAK file\n");
    }
    h.offset = *(int32_t*)&buffer[4];
    h.size = *(int32_t*)&buffer[8];

    printf("file table offset: %i\nfile table size: %i\n\n", h.offset, h.size);

    size_t header_offset = h.offset;
    for (size_t i = 0; i < (size_t)(h.size / FILE_HEADER_SZ); i += 1) {
        fseek(pakfile, header_offset, SEEK_SET);
        fread(buffer, 1, FILE_HEADER_SZ, pakfile);
        header_offset = ftell(pakfile);

        file_header* fh = (file_header*)&buffer[0];
        //printf("filename: %s\n  filesize: %i\n", fh->name, fh->size);
        if (mkdir((char*)fh->name, 0755) < 0) perror("mkdir");
        FILE *outfile = fopen((char*)&fh->name, "w");
        if (outfile == NULL) perror((char*)fh->name);
        else {
            fseek(pakfile, fh->offset, SEEK_SET);
            int c = 0;
            size_t len = 0;
            while (c != EOF && len < (size_t)fh->size) {
                c = getc(pakfile);
                putc(c, outfile);
                ++len;
            }
            fclose(outfile);
        }
    }

    fclose(pakfile);
    return 0;
}
