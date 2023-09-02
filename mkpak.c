#define _DEFAULT_SOURCE /* needed for DT_DIR */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include <dirent.h>

#include "pak.h"

static size_t count_directory(char* path);
static void enter_directory(char* path);

static FILE *pakfile = NULL;
static size_t pakptr_header = 0;
static size_t pakptr_data = 0;

static size_t count_directory(char* path) {
    size_t count = 0;
    static char current_path[512] = {0};
    static size_t path_stack[256];
    static size_t path_sp = 0;

    if (path_sp == 0) {
        strncpy(current_path, path, 254);
        strcat(current_path, "/");
        path_stack[path_sp++] = strlen(current_path);
    }

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
        if (ent->d_type == DT_DIR) count += count_directory(current_path);
        else ++count;
        --path_sp;
    }
    closedir(dp);
    return count;
}

static void enter_directory(char* path) {
    static char current_path[512] = {0};
    static size_t path_stack[256];
    static size_t path_sp = 0;
    static size_t archived_path_p = 0;

    if (path_sp == 0) {
        strncpy(current_path, path, 254);
        strcat(current_path, "/");
        path_stack[path_sp++] = archived_path_p = strlen(current_path);
    }

    DIR *indir = opendir(path);
    if (indir == NULL) {
        fprintf(stderr, "failed to open folder %s: %s\n", path, strerror(errno));
        exit(EXIT_FAILURE);
    }
    struct dirent *infe = NULL;
    FILE *infd = NULL;

    while ((infe = readdir(indir)) != NULL) {
        if (!strncmp(infe->d_name, ".", 2)) continue;
        if (!strncmp(infe->d_name, "..", 3)) continue;
        if (infe->d_type == DT_DIR) strcat(infe->d_name, "/");
        strncpy(current_path + path_stack[path_sp - 1], infe->d_name, 256);
        path_stack[path_sp++] = strlen(current_path);
        if (infe->d_type == DT_DIR) enter_directory(current_path);
        else {
            infd = fopen(current_path, "r");
            if (infd == NULL) {
                fprintf(stderr, "failed to open file %s: %s\n", current_path, strerror(errno));
            } else {
                file_header fh;
                int c;
                size_t len = 0;
                fseek(pakfile, pakptr_data, SEEK_SET);
                fh.offset = pakptr_data;
                while ((c = getc(infd)) != EOF) {
                    putc(c, pakfile);
                    ++len;
                }
                fclose(infd);
                pakptr_data += len;
                fh.size = len;
                strncpy((char*)fh.name, current_path + archived_path_p, sizeof(fh.name));
                fseek(pakfile, pakptr_header, SEEK_SET);
                fwrite(&fh, FILE_HEADER_SZ, 1, pakfile);
                pakptr_header += FILE_HEADER_SZ;
            }
        }
        --path_sp;
    }
    closedir(indir);
}

int main(int argc, char *argv[]) {
    char filename[256];
    if (argc < 2) {
        fputs("please provide a folder name\n", stderr);
        exit(EXIT_FAILURE);
    }
    strncpy(filename, argv[1], 251);
    size_t i = 0;
    while (filename[i] != '\0') ++i;
    while (filename[--i] == '/') filename[i] = '\0';
    strncat(filename, ".pak", 251);
    puts(filename);

    pak_header h = {.magic = {'P', 'A', 'C', 'K'}, .offset = PAK_HEADER_SZ};
    size_t file_count = count_directory(argv[1]);

    pakfile = fopen(filename, "wb");
    if (pakfile == NULL) {
        fprintf(stderr, "failed to open output file %s: %s\n", filename, strerror(errno));
        exit(EXIT_FAILURE);
    }

    h.size = file_count*FILE_HEADER_SZ;
    fwrite(&h, PAK_HEADER_SZ, 1, pakfile);

    pakptr_header = h.offset;
    pakptr_data = PAK_HEADER_SZ+h.size;

    enter_directory(argv[1]);

    fclose(pakfile);
    return EXIT_SUCCESS;
}
