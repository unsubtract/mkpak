#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

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
    puts("hello world");
}
