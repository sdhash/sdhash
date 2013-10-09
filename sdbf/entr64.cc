/*
 * 64-byte entropy function implementations
 */

#include "sdbf_conf.h"

//static uint64_t ENTROPY_64_INT[65];

/**
 * \internal
 * Entropy lookup table setup--int64 version (to be called once)
 */
void 
sdbf_conf::entr64_table_init_int() {
    uint32_t i;
    ENTROPY_64_INT[0] = 0;
    for( i=1; i<=64; i++) {
        double p = (double)i/64;
        ENTROPY_64_INT[i] = (uint64_t) ((-p*(log(p)/log((double)2))/6)*ENTR_SCALE);
    }
}

/**
 * \internal
 * Baseline entropy computation for a 64-byte buffer--int64 version (to be called periodically)
 */
uint64_t 
sdbf_conf::entr64_init_int( const uint8_t *buffer, uint8_t *ascii) {
    uint32_t i;

    memset( ascii, 0, 256);
    for( i=0; i<64; i++) {
        uint8_t bf = buffer[i];
        ascii[bf]++;
    }
    uint64_t entr=0;
    for( i=0; i<256; i++)
    if( ascii[i])
        entr += ENTROPY_64_INT[ ascii[i]];
    return entr;
}

/**
 * \internal
 * Incremental (rolling) update to entropy computation--int64 version
 */
uint64_t 
sdbf_conf::entr64_inc_int( uint64_t prev_entropy, const uint8_t *buffer, uint8_t *ascii) {
  if( buffer[0] == buffer[64])
    return prev_entropy;

  uint32_t old_char_cnt = ascii[buffer[0]];
  uint32_t new_char_cnt = ascii[buffer[64]];

  ascii[buffer[0]]--;
  ascii[buffer[64]]++;
 
  if( old_char_cnt == new_char_cnt+1)
    return prev_entropy;

  int64_t old_diff = int64_t(ENTROPY_64_INT[old_char_cnt])   - int64_t(ENTROPY_64_INT[old_char_cnt-1]);
  int64_t new_diff = int64_t(ENTROPY_64_INT[new_char_cnt+1]) - int64_t(ENTROPY_64_INT[new_char_cnt]);

  int64_t entropy =int64_t(prev_entropy) - old_diff + new_diff;
  if( entropy < 0)
    entropy = 0;
  else if( entropy > ENTR_SCALE)
    entropy = ENTR_SCALE;

  return (uint64_t)entropy;
}

