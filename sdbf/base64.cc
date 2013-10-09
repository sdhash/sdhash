#include <stdint.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

#include "util.h"

#include "../base64/modp_b64.h"

/**
 * Base64 encodes a memory buffer. Result is NULL terminated
 */
char *b64encode(const char *input, int length) {
    uint64_t mylen;
    mylen = modp_b64_encode_len(length);
    char *buffer = (char *)alloc_check( ALLOC_ONLY, mylen+1, "b64encode", "buffer", ERROR_EXIT);
    modp_b64_encode(buffer,input,length);
    if( !buffer)    
        return NULL;
    buffer[mylen] = 0;
    return buffer;
}

/**
 * Base64 decodes a memory buffer
 */
char *b64decode(char *input, int length, int *decoded_len) {
    char *buffer = (char *)alloc_check( ALLOC_ZERO, length, "b64decode", "buffer", ERROR_EXIT);
    if( !buffer)
        return NULL;
    *decoded_len = modp_b64_decode(buffer,input,length);
    return buffer;
}
/**
 * Base64 decodes a memory buffer
 */
uint64_t b64decode_into( const uint8_t *input, uint64_t length, uint8_t *output) {
    uint64_t decoded_len = modp_b64_decode((char*)output,(char*)input,length) ;
    return decoded_len;
}
