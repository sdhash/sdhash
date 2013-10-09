/**
 * sdbf.h: libsdbf header file
 * author: Vassil Roussev
 */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>

#include "sdbf_class.h"
#include "sdbf_conf.h"
#include "sdbf_set.h"
#include "util.h"
#include "index_info.h"

#include <vector>

#include <boost/interprocess/sync/interprocess_semaphore.hpp>

#ifndef __SDBF_DEF_H
#define __SDBF_DEF_H

#define _MAX_ELEM_COUNT  160
#define _MAX_ELEM_COUNT_DD  192
#define _FP_THRESHOLD 4

// Command-line related
#define DELIM_CHAR       ':'
#define DELIM_STRING     ":"
#define MAGIC_DD        "sdbf-dd"
#define MAGIC_STREAM    "sdbf"
#define MAX_MAGIC_HEADER 512

#define FLAG_OFF      0x00
#define FLAG_ON       0x01


// System parameters
#define BF_SIZE                256
#define BINS                1000
#define ENTR_POWER            10        
#define ENTR_SCALE            (BINS*(1 << ENTR_POWER))
#define MAX_FILES           1000000
#define MAX_THREADS         512
#define MIN_FILE_SIZE        512
// changing 6 to 16, 3/5/13
#define MIN_ELEM_COUNT     16
#define MIN_REF_ELEM_COUNT  64
#define POP_WIN_SIZE        64
#define SD_SCORE_SCALE      0.3
#define SYNC_SIZE           16384

#define BIGFILTER      32768

// ugly ugly cpuid check.  have to include it for OS X/Linux on same compile

#ifndef _WIN32

#define local_cpuid(func,ax,bx,cx,dx)\
    __asm__ __volatile__ ("cpuid":\
    "=a" (ax), "=b" (bx), "=c" (cx), "=d" (dx) : "a" (func));

#endif

// P-threading task spesicification structure for matching SDBFs 
typedef struct {
    uint32_t  tid;            // Thread id
    uint32_t  tcount;        // Total thread count for the job
    boost::interprocess::interprocess_semaphore sem_start;// Starting semaphore (allows thread to enter iteration)
    boost::interprocess::interprocess_semaphore sem_end;// Ending semaphore (signals the end of an iteration)
    class sdbf   *ref_sdbf;      // Reference SDBF
    uint32_t  ref_index;    // Index of the reference BF
    class sdbf   *tgt_sdbf;        // Target SDBF
    double       result;        // Result: max score for the task
    uint32_t  done;        // Are we finished
} sdbf_task_t; 

// P-threading task specification structure for block hashing 
typedef struct {
    uint32_t  tid;            // Thread id
    uint32_t  tcount;        // Total thread count for the job
    uint8_t  *buffer;       // File buffer to be hashed 
    uint64_t  file_size;    // File size (for the buffer) 
    uint64_t  block_size;   // Block size
    class    sdbf   *sdbf;            // Result SDBF
} blockhash_task_t; 


// P-threading task specification file-parallel stream hashing 
typedef struct {
    uint32_t  tid;          // Thread id
    uint32_t  tcount;       // Total thread count for the job
    char    **filenames;    // Files to be hashed 
    uint32_t  file_count;   // Total number of files 
    sdbf_set *addset;               // where to add the result to
    index_info *info;         // indexes to query against
} filehash_task_t;



// bf_utils.c: bit manipulation
// ----------------------------
uint32_t bf_bitcount( uint8_t *bfilter_1, uint8_t *bfilter_2, uint32_t bf_size);
uint32_t bf_bitcount_cut_256( uint8_t *bfilter_1, uint8_t *bfilter_2, uint32_t cut_off, int32_t slack);
uint32_t bf_bitcount_cut_256_asm( uint8_t *bfilter_1, uint8_t *bfilter_2, uint32_t cut_off, int32_t slack);
uint32_t bf_sha1_insert( uint8_t *bf, uint8_t bf_class, uint32_t *sha1_hash);
uint32_t bf_match_est( uint32_t m, uint32_t k, uint32_t s1, uint32_t s2, uint32_t common);
int32_t  get_elem_count(class sdbf *sdbf, uint64_t index);
void     bf_merge( uint32_t *base, uint32_t *overlay, uint32_t size);

// base64.c: Base64 encoding/decoding
// ----------------------------------
char     *b64encode(const char *input, int length);
char     *b64decode(char *input, int length, int *decoded_len);
uint64_t  b64decode_into( const uint8_t *input, uint64_t length, uint8_t *output);

#endif
