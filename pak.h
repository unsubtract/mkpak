/* pak.h - Quake PAK archive format structs
 * by unsubtract, MIT license */
#ifndef PAK_H_
#define PAK_H_
#include <stdint.h>
#define PAK_HEADER_SZ 12
typedef struct __attribute__((__packed__)) {
    uint8_t magic[4];
    uint32_t offset;
    uint32_t size;
} pak_header;

#define FILE_HEADER_SZ 64
typedef struct __attribute__((__packed__)) {
    uint8_t name[56];
    uint32_t offset;
    uint32_t size;
} file_header;
#endif /* PAK_H_ */
