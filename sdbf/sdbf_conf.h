
// Header file for sdbf object
//
#ifndef _SDBF_CONF_H
#define _SDBF_CONF_H

#include "sdbf_defines.h"

#include <stdint.h>
#include <stdio.h>

/**
    Configuration object for sdbf classes.  Used as a static class member to provide
    globally tunable defaults and access to bit counting structures.
*/
class sdbf_conf {

public:
    /// constructor: set defaults
    sdbf_conf(uint32_t thread_cnt, uint32_t warnings, uint32_t max_elem_ct, uint32_t max_elem_ct_dd ); 
      /// destructor 
    ~sdbf_conf(); 
public:
    /// number of pthreads available 
    uint32_t  thread_cnt; 
    uint32_t  entr_win_size; 
    uint32_t  bf_size; 
    uint32_t  pop_win_size; 
    uint32_t  block_size; 
    /// maximum elements per bf
    uint32_t  max_elem;  
    /// maximum elements per bf - dd mode
    uint32_t  max_elem_dd;  
    /// whether to process warnings
    uint32_t  warnings;  
    uint32_t  threshold; 
    bool popcnt;

    // collection of static variables used to hold data-collecting elements
    static uint8_t bit_count_16[64*KB]; 
    static uint16_t bf_est_cache[256][256];
    static uint32_t ENTR64_RANKS[];
    static uint32_t BIT_MASKS[];
    static uint8_t BITS[];
    static uint32_t BF_CLASS_MASKS[];
    uint64_t ENTROPY_64_INT[65];

public: 
    // setup functions 
    void entr64_table_init_int();
    uint64_t entr64_init_int( const uint8_t *buffer, uint8_t *ascii);
    uint64_t entr64_inc_int( uint64_t entropy, const uint8_t *buffer, uint8_t *ascii);

    void init_bit_count_16();

};

#endif
