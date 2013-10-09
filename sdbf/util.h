/*
 * General definitions (Vassil Roussev)
 */
#ifndef __UTIL_H
#define __UTIL_H

#include <stdio.h>
#include <stdint.h>

#define KB 1024
#define MB (KB*KB)
#define GB (MB*KB)

#define ALLOC_ONLY    1
#define ALLOC_ZERO    2
#define ALLOC_AUTO    3

#define ERROR_IGNORE    0
#define ERROR_EXIT        1

// Struct describing a mapped file
typedef struct {
    char     *name;
    int       fd;
    FILE     *input;
    uint64_t  size;
    uint8_t     *buffer;
} processed_file_t;

processed_file_t *process_file(const char *fname, int64_t min_file_size, uint32_t warnings);

void print256( const uint8_t *buffer);
void *alloc_check( uint32_t alloc_type, uint64_t mem_bytes, const char *fun_name, const char *var_name, uint32_t error_action);
void *realloc_check( void *buffer, uint64_t new_size);

#endif
