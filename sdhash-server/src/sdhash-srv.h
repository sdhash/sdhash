/**
 * sdhash.h: header file for sdhash program using api
 * author: candice quates
 */
//#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>

#include "sdbf_class.h"

#define RCFILE "/usr/local/etc/sdhash-srv.conf"
#define PORT 2700

#define SDBF_VERSION     2
#define VERSION_INFO    "sdhash-3.3 by Vassil Roussev, Candice Quates, 07/2013"

// Command line options
#define OPT_MAX       3
//
#define OPT_MODE          0
#define MODE_GEN      0x01
#define MODE_COMP     0x02
#define MODE_DIR      0x04
#define MODE_PAIR         0x08
#define MODE_FIRST        0x10
#define MODE_INFILE        0x20
//
#define OPT_MAP       1
#define FLAG_OFF      0x00
#define FLAG_ON       0x01

// SDHASH global parameters
typedef struct {
    uint32_t  thread_cnt;
    uint32_t  warnings;
    uint32_t  port;
    uint32_t  maxconn;
    string *host;
    string *home;
    string *sources;
} sdbf_parameters_t;

sdbf_parameters_t* read_args (int ac, char* av[], int quiet);

