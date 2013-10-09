/**
 * sdhash.h: header file for sdhash program using api
 * author: candice quates
 */
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
//#include <strings.h>
#include <stdlib.h>
#include <string.h>
//#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
//#include <unistd.h>

//#include "../sdbf/sdbf_class.h"

#define SDBF_VERSION    3 
#define VERSION_INFO    "sdhash-cli 3.3 by Vassil Roussev, Candice Quates, 07/2013"

// Defaults

#define PORT 9090
#define HOST "localhost"


// Command line options
//
#define OPT_MODE          0
#define MODE_HASH      0x01
#define MODE_COMP     0x02
#define MODE_QUERY      0x04
#define MODE_LIST         0x08
#define MODE_IMPORT    0x10
#define MODE_INFILE        0x20
#define MODE_DISP        0x40
#define MODE_VIEW     0x80
#define MODE_EXPORT     0x100
#define MODE_CONTENT     0x200
#define MODE_RESULT     0x400
#define MODE_INDEX     0x800

#define FLAG_OFF 0
#define FLAG_ON 0x01



// SDHASH global parameters
typedef struct {
    char *hostname;
    uint32_t port;
    uint32_t dd_block_size;
    int32_t   output_threshold;
    uint32_t  warnings;
    uint32_t  sample_size;
    uint32_t  options;
    uint32_t  options2;
    int32_t   resultid;
    char *username;
} sdhash_parameters_t;


