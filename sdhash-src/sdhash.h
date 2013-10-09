/**
 * sdhash.h: header file for sdhash program using api
 * author: candice quates
 */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include "../sdbf/sdbf_class.h"

#define SDBF_VERSION    3 
#define VERSION_INFO    "sdhash 3.4 by Vassil Roussev, Candice Quates [sdhash.org] 10/2013"


// Command line options
//
#define FLAG_OFF      0x00
#define FLAG_ON       0x01

// SDHASH global parameters
typedef struct {
    uint32_t  thread_cnt;
    uint32_t  entr_win_size; // obj
    uint32_t  bf_size; // obj
    uint32_t  block_size; // obj
    uint32_t  pop_win_size; // obj
    int32_t  threshold;
    uint32_t  max_elem; // obj
    int32_t   output_threshold;
    uint32_t  warnings;
    int32_t  dd_block_size;
    uint32_t  sample_size;
    uint32_t  verbose;
    uint64_t  segment_size;
    char *filename;
} sdbf_parameters_t;

