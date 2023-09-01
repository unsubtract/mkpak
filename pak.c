#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static void fail(const char* str);

#define HEADER_SZ 12
typedef struct {
    uint8_t magic[4];
    int32_t offset;
    int32_t size;
} pak_header;

#define FILE_HEADER_SZ 64
typedef struct {
    uint8_t name[56];
    int32_t offset;
    int32_t size;
} file_header;

#define BUFSZ_FACTOR 4
static uint8_t buffer[FILE_HEADER_SZ*BUFSZ_FACTOR];

static void fail(const char* str) {
    fputs(str, stderr);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    pak_header h = {.magic = {'P', 'A', 'C', 'K'}};
    FILE *fd = fopen("PAK0.pak", "rb");

    fread(buffer, 1, HEADER_SZ, fd);
    for (size_t i = 0; i < sizeof(h.magic); ++i) {
        if (buffer[i] != h.magic[i]) fail("not a PAK file\n");
    }
    h.offset = *(int32_t*)&buffer[4];
    h.size = *(int32_t*)&buffer[8];

    printf("file table offset: %i\nfile table size: %i\n\n", h.offset, h.size);

    fseek(fd, h.offset, SEEK_SET);
    for (size_t i = 0; i < (size_t)(h.size / FILE_HEADER_SZ); i += 1) {
        fread(buffer, 1, FILE_HEADER_SZ, fd);
        file_header* fh = (file_header*)&buffer[0];
        printf("filename: %s\n  filesize: %i\n", fh->name, fh->size);
    }

    fclose(fd);
    return 0;
}
