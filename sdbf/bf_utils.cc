/**
 * bf_utils.c: Bloom filter utilities
 * author: Vassil Roussev
 */

#include <math.h>
#include "sdbf_class.h"
#include "sdbf_conf.h"
#include <boost/math/special_functions/round.hpp>

#ifndef _M_IX86
#include <smmintrin.h>
#endif
/**
 * Estimate number of expected matching bits
 */
uint32_t bf_match_est( uint32_t m, uint32_t k, uint32_t s1, uint32_t s2, uint32_t common) {

    // This cache should work >99% of the time
    if( !common && sdbf::config->bf_est_cache[s1][s2]) {
        return sdbf::config->bf_est_cache[s1][s2];
    }
    double ex = 1-1.0/m;
    uint32_t result = boost::math::round((double)m*(1 - pow( ex, (double)k*s1) - pow( ex, (double)k*s2) + pow( ex, (double)k*(s1+s2-common))));
    sdbf::config->bf_est_cache[s1][s2] = (uint16_t)result;
    return result;
}

/**
 * Insert a SHA1 hash into a Bloom filter
 */
uint32_t bf_sha1_insert( uint8_t *bf, uint8_t bf_class, uint32_t *sha1_hash) {
    uint32_t i, k, insert_cnt = 0, bit_mask = sdbf::config->BF_CLASS_MASKS[bf_class];
   uint32_t insert=0; 
    for( i=0; i<5; i++) {
      insert=sha1_hash[i] & bit_mask;
        k = insert >> 3;
        if( !(bf[k] & sdbf::config->BITS[insert & 0x7]))
            insert_cnt++;
        bf[k] |= sdbf::config->BITS[insert & 0x7];
    }
    return insert_cnt;
}

/**
 * bf_merge(): Performs bitwise OR on two BFs
 */
void bf_merge( uint32_t *base, uint32_t *overlay, uint32_t size) {
    uint32_t i;
    for( i=0; i<size; i++)
        base[i] |= overlay[i];
}

/**
 * Compute the number of common bits b/w two filters
 * todo: make it work with any size BF
 */
uint32_t bf_bitcount( uint8_t *bfilter_1, uint8_t *bfilter_2, uint32_t bf_size) {
    uint32_t i, result=0;
    uint64_t buff64[32];
    uint64_t *f1_64 = (uint64_t *)bfilter_1;
    uint64_t *f2_64 = (uint64_t *)bfilter_2;
    uint16_t *buff16 = (uint16_t *)buff64;
    for( i=0; i<bf_size/8; i++)
        buff64[i] = f1_64[i] & f2_64[i];

    for( i=0; i<bf_size/2; i++) {
        result += sdbf::config->bit_count_16[buff16[i]];
    }
    return result;
   
}

/** 
   Test code for compiler intrinsic 64-bit popcnt
*/
uint32_t
bf_bitcount_cut_256_asm( uint8_t *bfilter_1, uint8_t *bfilter_2, uint32_t cut_off, int32_t slack) {
//bf_bitcount_asm( uint8_t *bfilter_1, uint8_t *bfilter_2) {
    uint32_t result=0;
    uint64_t buff64[32];
    uint64_t *f1_64 = (uint64_t *)bfilter_1;
    uint64_t *f2_64 = (uint64_t *)bfilter_2;
    // Partial computation (1/8 of full computation):
    buff64[0]= f1_64[0] & f2_64[0];
    buff64[1]= f1_64[1] & f2_64[1];
    buff64[2]= f1_64[2] & f2_64[2];
    buff64[3]= f1_64[3] & f2_64[3];
#ifndef _M_IX86 // allowing win32 shortcut
    result += _mm_popcnt_u64(buff64[0]);
    result += _mm_popcnt_u64(buff64[1]);
    result += _mm_popcnt_u64(buff64[2]);
    result += _mm_popcnt_u64(buff64[3]);
    // First shortcircuit for the computation
    if( cut_off > 0 && (8*result + slack) < cut_off) {
        return 0;
    }
    buff64[4]= f1_64[4] & f2_64[4];
    buff64[5]= f1_64[5] & f2_64[5];
    buff64[6]= f1_64[6] & f2_64[6];
    buff64[7]= f1_64[7] & f2_64[7];
    result += _mm_popcnt_u64(buff64[4]);
    result += _mm_popcnt_u64(buff64[5]);
    result += _mm_popcnt_u64(buff64[6]);
    result += _mm_popcnt_u64(buff64[7]);
    // Second shortcircuit for the computation
    if( cut_off > 0 && (4*result + slack) < cut_off) {
        return 0;
    }
    buff64[8]= f1_64[8] & f2_64[8];
    buff64[9]= f1_64[9] & f2_64[9];
    buff64[10]= f1_64[10] & f2_64[10];
    buff64[11]= f1_64[11] & f2_64[11];
    buff64[12]= f1_64[12] & f2_64[12];
    buff64[13]= f1_64[13] & f2_64[13];
    buff64[14]= f1_64[14] & f2_64[14];
    buff64[15]= f1_64[15] & f2_64[15];

    result += _mm_popcnt_u64(buff64[8]);
    result += _mm_popcnt_u64(buff64[9]);
    result += _mm_popcnt_u64(buff64[10]);
    result += _mm_popcnt_u64(buff64[11]);
    result += _mm_popcnt_u64(buff64[12]);
    result += _mm_popcnt_u64(buff64[13]);
    result += _mm_popcnt_u64(buff64[14]);
    result += _mm_popcnt_u64(buff64[15]);
    // Third shortcircuit for the computation
    if( cut_off > 0 && (2*result + slack) < cut_off) {
        return 0;
    }
    buff64[16]= f1_64[16] & f2_64[16];
    buff64[17]= f1_64[17] & f2_64[17];
    buff64[18]= f1_64[18] & f2_64[18];
    buff64[19]= f1_64[19] & f2_64[19];
    buff64[20]= f1_64[20] & f2_64[20];
    buff64[21]= f1_64[21] & f2_64[21];
    buff64[22]= f1_64[22] & f2_64[22];
    buff64[23]= f1_64[23] & f2_64[23];
    buff64[24]= f1_64[24] & f2_64[24];
    buff64[25]= f1_64[25] & f2_64[25];
    buff64[26]= f1_64[26] & f2_64[26];
    buff64[27]= f1_64[27] & f2_64[27];
    buff64[28]= f1_64[28] & f2_64[28];
    buff64[29]= f1_64[29] & f2_64[29];
    buff64[30]= f1_64[30] & f2_64[30];
    buff64[31]= f1_64[31] & f2_64[31];

    result += _mm_popcnt_u64(buff64[16]);
    result += _mm_popcnt_u64(buff64[17]);
    result += _mm_popcnt_u64(buff64[18]);
    result += _mm_popcnt_u64(buff64[19]);
    result += _mm_popcnt_u64(buff64[20]);
    result += _mm_popcnt_u64(buff64[21]);
    result += _mm_popcnt_u64(buff64[22]);
    result += _mm_popcnt_u64(buff64[23]);
    result += _mm_popcnt_u64(buff64[24]);
    result += _mm_popcnt_u64(buff64[25]);
    result += _mm_popcnt_u64(buff64[26]);
    result += _mm_popcnt_u64(buff64[27]);
    result += _mm_popcnt_u64(buff64[28]);
    result += _mm_popcnt_u64(buff64[29]);
    result += _mm_popcnt_u64(buff64[30]);
    result += _mm_popcnt_u64(buff64[31]);
#endif
    //for( i=0; i<32; i++) {
       //result += _mm_popcnt_u64(buff64[i]);
        //result += sdbf::config->bit_count_16[buff16[i]];
    //}
    return result;
}   
    /**
     * Computer the number of common bits (dot product) b/w two filters--conditional optimized version for 256-byte BFs.
     * The conditional looks first at the dot product of the first 32/64/128 bytes; if it is less than the threshold,
     * it returns 0; otherwise, proceeds with the rest of the computation.
     */
uint32_t bf_bitcount_cut_256( uint8_t *bfilter_1, uint8_t *bfilter_2, uint32_t cut_off, int32_t slack) {
        uint32_t result=0;
        uint64_t buff64[32];
        uint64_t *f1_64 = (uint64_t *)bfilter_1;
        uint64_t *f2_64 = (uint64_t *)bfilter_2;
        uint16_t *buff16 = (uint16_t *)buff64;

        // Partial computation (1/8 of full computation):
        buff64[0]= f1_64[0] & f2_64[0];
        buff64[1]= f1_64[1] & f2_64[1];
        buff64[2]= f1_64[2] & f2_64[2];
        buff64[3]= f1_64[3] & f2_64[3];
        result += sdbf::config->bit_count_16[buff16[0]];
        result += sdbf::config->bit_count_16[buff16[1]];
        result += sdbf::config->bit_count_16[buff16[2]];
        result += sdbf::config->bit_count_16[buff16[3]];
        result += sdbf::config->bit_count_16[buff16[4]];
        result += sdbf::config->bit_count_16[buff16[5]];
        result += sdbf::config->bit_count_16[buff16[6]];
        result += sdbf::config->bit_count_16[buff16[7]];
        result += sdbf::config->bit_count_16[buff16[8]];
        result += sdbf::config->bit_count_16[buff16[9]];
        result += sdbf::config->bit_count_16[buff16[10]];
        result += sdbf::config->bit_count_16[buff16[11]];
        result += sdbf::config->bit_count_16[buff16[12]];
        result += sdbf::config->bit_count_16[buff16[13]];
        result += sdbf::config->bit_count_16[buff16[14]];
        result += sdbf::config->bit_count_16[buff16[15]];

        // First shortcircuit for the computation
        if( cut_off > 0 && (8*result + slack) < cut_off) {
            return 0;
        }
    buff64[4]= f1_64[4] & f2_64[4];
    buff64[5]= f1_64[5] & f2_64[5];
    buff64[6]= f1_64[6] & f2_64[6];
    buff64[7]= f1_64[7] & f2_64[7];
    result += sdbf::config->bit_count_16[buff16[16]];
    result += sdbf::config->bit_count_16[buff16[17]];
    result += sdbf::config->bit_count_16[buff16[18]];
    result += sdbf::config->bit_count_16[buff16[19]];
    result += sdbf::config->bit_count_16[buff16[20]];
    result += sdbf::config->bit_count_16[buff16[21]];
    result += sdbf::config->bit_count_16[buff16[22]];
    result += sdbf::config->bit_count_16[buff16[23]];
    result += sdbf::config->bit_count_16[buff16[24]];
    result += sdbf::config->bit_count_16[buff16[25]];
    result += sdbf::config->bit_count_16[buff16[26]];
    result += sdbf::config->bit_count_16[buff16[27]];
    result += sdbf::config->bit_count_16[buff16[28]];
    result += sdbf::config->bit_count_16[buff16[29]];
    result += sdbf::config->bit_count_16[buff16[30]];
    result += sdbf::config->bit_count_16[buff16[31]];

    // Second shortcircuit for the computation
    if( cut_off > 0 && (4*result + slack) < cut_off) {
        return 0;
    }
    buff64[8]= f1_64[8] & f2_64[8];
    buff64[9]= f1_64[9] & f2_64[9];
    buff64[10]= f1_64[10] & f2_64[10];
    buff64[11]= f1_64[11] & f2_64[11];
    buff64[12]= f1_64[12] & f2_64[12];
    buff64[13]= f1_64[13] & f2_64[13];
    buff64[14]= f1_64[14] & f2_64[14];
    buff64[15]= f1_64[15] & f2_64[15];
    result += sdbf::config->bit_count_16[buff16[32]];
    result += sdbf::config->bit_count_16[buff16[33]];
    result += sdbf::config->bit_count_16[buff16[34]];
    result += sdbf::config->bit_count_16[buff16[35]];
    result += sdbf::config->bit_count_16[buff16[36]];
    result += sdbf::config->bit_count_16[buff16[37]];
    result += sdbf::config->bit_count_16[buff16[38]];
    result += sdbf::config->bit_count_16[buff16[39]];
    result += sdbf::config->bit_count_16[buff16[40]];
    result += sdbf::config->bit_count_16[buff16[41]];
    result += sdbf::config->bit_count_16[buff16[42]];
    result += sdbf::config->bit_count_16[buff16[43]];
    result += sdbf::config->bit_count_16[buff16[44]];
    result += sdbf::config->bit_count_16[buff16[45]];
    result += sdbf::config->bit_count_16[buff16[46]];
    result += sdbf::config->bit_count_16[buff16[47]];
    result += sdbf::config->bit_count_16[buff16[48]];
    result += sdbf::config->bit_count_16[buff16[49]];
    result += sdbf::config->bit_count_16[buff16[50]];
    result += sdbf::config->bit_count_16[buff16[51]];
    result += sdbf::config->bit_count_16[buff16[52]];
    result += sdbf::config->bit_count_16[buff16[53]];
    result += sdbf::config->bit_count_16[buff16[54]];
    result += sdbf::config->bit_count_16[buff16[55]];
    result += sdbf::config->bit_count_16[buff16[56]];
    result += sdbf::config->bit_count_16[buff16[57]];
    result += sdbf::config->bit_count_16[buff16[58]];
    result += sdbf::config->bit_count_16[buff16[59]];
    result += sdbf::config->bit_count_16[buff16[60]];
    result += sdbf::config->bit_count_16[buff16[61]];
    result += sdbf::config->bit_count_16[buff16[62]];
    result += sdbf::config->bit_count_16[buff16[63]];

    // Third shortcircuit for the computation
    if( cut_off > 0 && (2*result + slack) < cut_off) {
        return 0;
    }
    buff64[16]= f1_64[16] & f2_64[16];
    buff64[17]= f1_64[17] & f2_64[17];
    buff64[18]= f1_64[18] & f2_64[18];
    buff64[19]= f1_64[19] & f2_64[19];
    buff64[20]= f1_64[20] & f2_64[20];
    buff64[21]= f1_64[21] & f2_64[21];
    buff64[22]= f1_64[22] & f2_64[22];
    buff64[23]= f1_64[23] & f2_64[23];
    buff64[24]= f1_64[24] & f2_64[24];
    buff64[25]= f1_64[25] & f2_64[25];
    buff64[26]= f1_64[26] & f2_64[26];
    buff64[27]= f1_64[27] & f2_64[27];
    buff64[28]= f1_64[28] & f2_64[28];
    buff64[29]= f1_64[29] & f2_64[29];
    buff64[30]= f1_64[30] & f2_64[30];
    buff64[31]= f1_64[31] & f2_64[31];
    result += sdbf::config->bit_count_16[buff16[64]];
    result += sdbf::config->bit_count_16[buff16[65]];
    result += sdbf::config->bit_count_16[buff16[66]];
    result += sdbf::config->bit_count_16[buff16[67]];
    result += sdbf::config->bit_count_16[buff16[68]];
    result += sdbf::config->bit_count_16[buff16[69]];
    result += sdbf::config->bit_count_16[buff16[70]];
    result += sdbf::config->bit_count_16[buff16[71]];
    result += sdbf::config->bit_count_16[buff16[72]];
    result += sdbf::config->bit_count_16[buff16[73]];
    result += sdbf::config->bit_count_16[buff16[74]];
    result += sdbf::config->bit_count_16[buff16[75]];
    result += sdbf::config->bit_count_16[buff16[76]];
    result += sdbf::config->bit_count_16[buff16[77]];
    result += sdbf::config->bit_count_16[buff16[78]];
    result += sdbf::config->bit_count_16[buff16[79]];
    result += sdbf::config->bit_count_16[buff16[80]];
    result += sdbf::config->bit_count_16[buff16[81]];
    result += sdbf::config->bit_count_16[buff16[82]];
    result += sdbf::config->bit_count_16[buff16[83]];
    result += sdbf::config->bit_count_16[buff16[84]];
    result += sdbf::config->bit_count_16[buff16[85]];
    result += sdbf::config->bit_count_16[buff16[86]];
    result += sdbf::config->bit_count_16[buff16[87]];
    result += sdbf::config->bit_count_16[buff16[88]];
    result += sdbf::config->bit_count_16[buff16[89]];
    result += sdbf::config->bit_count_16[buff16[90]];
    result += sdbf::config->bit_count_16[buff16[91]];
    result += sdbf::config->bit_count_16[buff16[92]];
    result += sdbf::config->bit_count_16[buff16[93]];
    result += sdbf::config->bit_count_16[buff16[94]];
    result += sdbf::config->bit_count_16[buff16[95]];
    result += sdbf::config->bit_count_16[buff16[96]];
    result += sdbf::config->bit_count_16[buff16[97]];
    result += sdbf::config->bit_count_16[buff16[98]];
    result += sdbf::config->bit_count_16[buff16[99]];
    result += sdbf::config->bit_count_16[buff16[100]];
    result += sdbf::config->bit_count_16[buff16[101]];
    result += sdbf::config->bit_count_16[buff16[102]];
    result += sdbf::config->bit_count_16[buff16[103]];
    result += sdbf::config->bit_count_16[buff16[104]];
    result += sdbf::config->bit_count_16[buff16[105]];
    result += sdbf::config->bit_count_16[buff16[106]];
    result += sdbf::config->bit_count_16[buff16[107]];
    result += sdbf::config->bit_count_16[buff16[108]];
    result += sdbf::config->bit_count_16[buff16[109]];
    result += sdbf::config->bit_count_16[buff16[110]];
    result += sdbf::config->bit_count_16[buff16[111]];
    result += sdbf::config->bit_count_16[buff16[112]];
    result += sdbf::config->bit_count_16[buff16[113]];
    result += sdbf::config->bit_count_16[buff16[114]];
    result += sdbf::config->bit_count_16[buff16[115]];
    result += sdbf::config->bit_count_16[buff16[116]];
    result += sdbf::config->bit_count_16[buff16[117]];
    result += sdbf::config->bit_count_16[buff16[118]];
    result += sdbf::config->bit_count_16[buff16[119]];
    result += sdbf::config->bit_count_16[buff16[120]];
    result += sdbf::config->bit_count_16[buff16[121]];
    result += sdbf::config->bit_count_16[buff16[122]];
    result += sdbf::config->bit_count_16[buff16[123]];
    result += sdbf::config->bit_count_16[buff16[124]];
    result += sdbf::config->bit_count_16[buff16[125]];
    result += sdbf::config->bit_count_16[buff16[126]];
    result += sdbf::config->bit_count_16[buff16[127]];
    return result;
}




