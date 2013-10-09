// Header file for bloom_filter object
//
#ifndef _BLOOM_FILTER_H 
#define _BLOOM_FILTER_H


#include <stdint.h>
#include <string>
//#include <strings.h>

using namespace std;

/**
    bloom_filter:  a Bloom filter class.
*/
/// bloom_filter class
class bloom_filter {

public:
    /// base constructor
    bloom_filter(uint64_t size, uint16_t hash_count, uint64_t max_elem, double max_fp); 

    /// construct from file - no folding -- sdbf-idx form
    bloom_filter(string indexfilename);

    /// construct bloom filter from buffer 
    bloom_filter(uint8_t* data,uint64_t size,int id, int bf_elem_ct, uint16_t hamming);
    
    /// construct from string - optional folds
    bloom_filter(string filter, int folds);

    /// destructor
    ~bloom_filter();

    /// insert SHA1 hash
    bool insert_sha1(uint32_t *sha1);
    
    /// query SHA1 hash
    bool query_sha1(uint32_t *sha1);

    /// return element count
    uint64_t elem_count();
    /// return estimate of false positive rate
    double est_fp_rate();    
    /// return bits per element
    double bits_per_elem();
 
    /// name associated with bloom filter
    string name() const;
    /// change name associated with bloom filter
    void set_name(string name);
    /// fold a large bloom filter onto itself
    void fold(uint32_t times);
    /// add another same-sized bloom filter to this one
    int add(bloom_filter *other);

    /// compare to other filter
    int compare(bloom_filter *other,double scale);

    /// write bloom filter to .idx file
    int write_out(string filename);

    /// id associated with bloom filter (used for grouping)
    int bloom_id();  
    void set_bloom_id(int id);
    string to_string () const ;
    void compute_hamming();



private:
    /// actual query/insert function
    bool query_and_set(uint32_t *sha1, bool mode_set);
    /// compress blob
    char* compress() ;
    /// decompress blob and assign to bf
    int32_t decompress(char* src);
public:
    static const uint32_t BIT_MASKS_32[];
    static const uint32_t BITS[];

    uint8_t  *bf;            // Beginning of the BF 
    uint16_t  hamming;        // weight of this bf
    uint32_t  hamminglg;        // weight of this bf
    uint64_t  bf_size;       // BF size in bytes (==m/8)
    uint64_t  bit_mask;      // Bit mask
    uint64_t  max_elem;      // Max number of elements
    uint16_t  hash_count;    // Number of hash functions used (k)
private:
    double    max_fp;        // Max FP rate
    
    //uint64_t  bf_size;       // BF size in bytes (==m/8)
    uint64_t  bf_elem_count; // Actual number of elements inserted
    uint64_t  comp_size;     // size of compressed bf to be read
    string    setname;       // name associated with bloom filter
    bool      created;       // set if we allocated the bloom filter ourselves
    int          bl_id;

};

#endif
