// Header file for sdbf object
//
#ifndef _SDBF_CLASS_H 
#define _SDBF_CLASS_H


#include "sdbf_defines.h"
#include "sdbf_conf.h"
#include "bloom_filter.h"

#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

/**
    sdbf:  a Similarity Digest Bloom Filter class.
*/
/// sdbf class
class sdbf {

    friend std::ostream& operator<<(std::ostream& os, const sdbf& s); ///< output operator
    friend std::ostream& operator<<(std::ostream& os, const sdbf *s); ///< output operator

    /** \example sdbf_test.cc
    *  A very short example program using sdbf.
    */

public:
    /// to read a formatted sdbf from string
    sdbf(const std::string& str); 
    /// to create new from a single file
    sdbf(const char *filename, uint32_t dd_block_size); 
    /// to create by reading from an open stream
    sdbf(const char *name, std::istream *ifs, uint32_t dd_block_size, uint64_t msize, index_info *info) ; 
    /// to create from a c-string
    sdbf(const char *name, char *str, uint32_t dd_block_size, uint64_t length, index_info *info);
    /// destructor
    ~sdbf(); 

    /// object name
    const char *name();  
    /// object size
    uint64_t size();  
    /// source object size
    uint64_t input_size();  

    /// matching algorithm, take other object and run match
    int32_t compare(sdbf *other, uint32_t sample);

    /// return a string representation of this sdbf
    string to_string() const ; 

    /// return results of index search
    string get_index_results() const; 

    /// return a copy of an individual bloom filter from this sdbf
    uint8_t *clone_filter(uint32_t position);
    uint32_t filter_count();

public:
    /// global configuration object
    static class sdbf_conf *config;  
    static int32_t get_elem_count(sdbf *mine, uint64_t index) ;

private:

    int compute_hamming();
    void sdbf_create(const char *filename);
   // static int32_t get_elem_count(sdbf *mine, uint64_t index) ;

    // from sdbf_core.c: Core SDBF generation/comparison functions
    static void gen_chunk_ranks( uint8_t *file_buffer, const uint64_t chunk_size, uint16_t *chunk_ranks, uint16_t carryover);
    static void gen_chunk_scores( const uint16_t *chunk_ranks, const uint64_t chunk_size, uint16_t *chunk_scores, int32_t *score_histo);
    void gen_chunk_hash( uint8_t *file_buffer, const uint64_t chunk_pos, const uint16_t *chunk_scores, const uint64_t chunk_size);
    static void gen_block_hash( uint8_t *file_buffer, uint64_t file_size, const uint64_t block_num, const uint16_t *chunk_scores, const uint64_t block_size, class sdbf *hashto,uint32_t rem, uint32_t threshold, int32_t allowed);
    void gen_chunk_sdbf( uint8_t *file_buffer, uint64_t file_size, uint64_t chunk_size);
    void gen_block_sdbf_mt( uint8_t *file_buffer, uint64_t file_size, uint64_t block_size, uint32_t thread_cnt);

    static void *thread_gen_block_sdbf( void *task_param);
    static int     sdbf_score( sdbf *sd_1, sdbf *sd_2, uint32_t sample);
    static double  sdbf_max_score( sdbf_task_t *task);

    void print_indexes(uint32_t threshold, vector<uint32_t> *matches, uint64_t pos);
    void reset_indexes(vector<uint32_t> *matches);
    bool check_indexes(uint32_t* sha1, vector<uint32_t> *matches);
    bool is_block_null(uint8_t *buffer, uint32_t size);
    
public:
    uint8_t  *buffer;        // Beginning of the BF cluster
    uint16_t *hamming;       // Hamming weight for each BF
   // uint16_t *elem_counts;   // Individual elements counts for each BF (used in dd mode)
    uint32_t  max_elem;      // Max number of elements per filter (n)
private:
    index_info *info;
    string index_results;
        
    // from the C structure 
    char *hashname;          // name (usually, source file)
    uint32_t  bf_count;      // Number of BFs
    uint32_t  bf_size;       // BF size in bytes (==m/8)
    uint32_t  hash_count;    // Number of hash functions used (k)
    uint32_t  mask;          // Bit mask used (must agree with m)
    uint32_t  last_count;    // Actual number of elements in last filter (n_last); 
                                                         // ZERO means look at elem_counts value 
    //uint8_t  *buffer;        // Beginning of the BF cluster
    //uint16_t *hamming;       // Hamming weight for each BF
    uint16_t *elem_counts;   // Individual elements counts for each BF (used in dd mode)
    uint32_t  dd_block_size; // Size of the base block in dd mode
    uint64_t orig_file_size; // size of the original file
    bool     filenamealloc;
public:
    std::vector<class bloom_filter *> *big_filters;

};

#endif
