/**
 * error.c: Generic error/debug routines 
 * author: Vassil Roussev
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"


/**
 * Allocate memory, check result, and print error (if necessary).
 */
void *alloc_check( uint32_t alloc_type, uint64_t mem_bytes, const char *fun_name, const char *var_name, uint32_t error_action) {
    void *mem_chunk = NULL;

    switch( alloc_type) {
        case ALLOC_ONLY:
            mem_chunk = malloc( mem_bytes);
            break;
        case ALLOC_AUTO:
            mem_chunk = malloc( mem_bytes);
            break;
        case ALLOC_ZERO:
            mem_chunk = calloc( 1, mem_bytes);
            break;
        default:
            return NULL;
    }
    if( mem_chunk == NULL) {
        fprintf( stderr, "Could not allocate %dKB (%dMB) for %s in function %s(). System message: \"%s\".", 
                        (int)(mem_bytes >> 10), (int)(mem_bytes >> 20), var_name, fun_name, strerror( errno));
        if( error_action == ERROR_EXIT) {
            fprintf( stderr, " Exiting.\n");
            exit(-1);
        }
    }
    return mem_chunk;
}
void *realloc_check( void *buffer, uint64_t new_size) {
    void *mem_chunk = realloc( buffer, new_size);

    return mem_chunk;
}

/**
 * Print a 256-byte buffer
 */
void print256( const uint8_t *buffer) {
    uint16_t i;
    fprintf( stderr, "      ");
    for( i=0; i<32; i++)
        fprintf( stderr, "%02x ", i);
    fprintf( stderr, "\n");
    
    for( i=0; i<256; i++) {
        if( i % 32 == 0)
            fprintf( stderr, "%04x: ", i);
        fprintf( stderr, "%02x ", buffer[i]);
        if( i % 32 == 31)
            fprintf( stderr, "\n");
    }
}
