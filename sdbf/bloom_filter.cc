// bloom_filter class implementation
// author: candice quates

#include "bloom_filter.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <smmintrin.h> 

#include <boost/math/special_functions/round.hpp>

#include "../lz4/lz4.h"
#include "sdbf_defines.h"

const uint32_t 
bloom_filter::BITS[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

const uint32_t 
bloom_filter::BIT_MASKS_32[] = {
    0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF,
    0x01FF, 0x03FF, 0x07FF, 0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF,
    0x01FFFF, 0x03FFFF, 0x07FFFF, 0x0FFFFF, 0x1FFFFF, 0x3FFFFF, 0x7FFFFF, 0xFFFFFF,
    0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF
};    

/**
   Create new empty bloom filter
   \param size of bloom filter
   \param hash_count number of hashes for each insertion or query
   \param max_elem max element size (0 ok)
   \param max_fp max false positive rate (0 ok)
*/
bloom_filter::bloom_filter( uint64_t size, uint16_t hash_count, uint64_t max_elem, double max_fp) {
    this->bf_size = size;
    this->hash_count = hash_count;
    this->max_elem = max_elem;
    this->max_fp = max_fp;
    
    // Make sure size if a power of 2 and at least 64
    if( size && !(size & (size - 1)) && size >= 64) {
        // Find log2(size)
        uint16_t log_size = 0;
        for( uint64_t tmp=size; tmp; tmp >>= 1, log_size++);
        bit_mask = BIT_MASKS_32[log_size+1];
    } else {
        throw -1; // sizes invalid
    } 
    bf = (uint8_t*)malloc(size);
    memset( bf, 0, size);
    bf_elem_count = 0;
    created=true;
}

/** 
    Read bloom filter(INDEX)  from a file
    \param indexfilename file to read
*/
bloom_filter::bloom_filter(string indexfilename){
    ifstream ifs(indexfilename.c_str(),ifstream::in|ios::binary);
    if (ifs.is_open()) {
        string process;
        // headerbit
        getline(ifs,process,':'); // ignore
        // bf_size
        getline(ifs,process,':');
        bf_size=boost::lexical_cast<uint64_t>(process);
        // elem_count
        getline(ifs,process,':');
        bf_elem_count=boost::lexical_cast<uint64_t>(process);
        // hash_count
        getline(ifs,process,':');
        hash_count=boost::lexical_cast<uint16_t>(process);
        // bit_mask
        getline(ifs,process,':');
        bit_mask=boost::lexical_cast<uint64_t>(process);
        // compressed_size
        getline(ifs,process,':');
        uint64_t comp_size=boost::lexical_cast<uint64_t>(process);
        // setname
        getline(ifs,setname);
        // endl
        // compressed data, size from header
        char* bf_comp =(char*)malloc(comp_size+1);
        bf = (uint8_t*) malloc(bf_size);
        ifs.read((char*)bf_comp,comp_size);
        // decompress.      
        int32_t result=this->decompress(bf_comp);
        if (result < 0) // failed to parse
            throw -2;
        free(bf_comp);
    } else 
       throw -1; // failed to read
    created=true;
    //created=false;
}

/**
    Creates bloom filter from existing buffer of bloom filter data.
       Experimental: sized for sdbf 256-byte bloom filters at the moment
    \param data buffer of bloom filter data
    \param size of bloom filter data
    \param id identifier for clustering bloom filters
    \param bf_elem_ct # of elements in filter
    \param hamming weight of filter
*/
bloom_filter::bloom_filter(uint8_t* data, uint64_t size, int id, int bf_elem_ct, uint16_t hamming) {
    // this makes several testing assumptions.  
    // this is for 256-byte BFs,
    // and elem_ct = 192, bit_mask=2047 (pre-calculated)
    // and hash_count=5 
    uint16_t log_size = 0;
    for( uint64_t tmp=size; tmp; tmp >>= 1, log_size++);
    bit_mask = BIT_MASKS_32[log_size+1];
    //bit_mask=2047;
    bf_size=size;
    hash_count=5;
    bf_elem_count=bf_elem_ct; // could also be 160
    bl_id = id;
    this->hamming = hamming;
    bf=(uint8_t*)malloc(size);
    memcpy(bf,data,size);
    // marker for non-destructive destroy?
    created=true;
}

/** 
    Destroys bloom filter and frees buffer 
*/
bloom_filter::~bloom_filter() {
    if (created)
        free(bf);
}

/** 
    Returns number of elements present in bloom filter
    \returns number of elements 
*/
uint64_t bloom_filter::elem_count() { return bf_elem_count;}

/** 
    Returns estimated false positive rate (not implemented)
    \returns estimate
*/
double  bloom_filter::est_fp_rate() { return -1.0;}

/** 
    Returns bits per element in bloom filter
    \returns estimate
*/
double bloom_filter::bits_per_elem() { return (double) (bf_size << 3)/bf_elem_count;}

/**
   \internal
   Compresses bloom filter for writing to disk and returns it
*/
char *
bloom_filter::compress() {
    char *dest=(char*)malloc(160*MB);
    int res = LZ4_compress_limitedOutput((const char*)bf,dest,bf_size,160*MB);
    if (res == 0) {
    comp_size = 0;
    free(dest);
        return NULL;
    } else {
        comp_size = res;
        return dest;
    }
}

/**
   \internal
   Decompresses bloom filter as read from file.  Arguments unpacked
   in read function.
   \param src contains source data 
*/
int32_t
bloom_filter::decompress(char *src) {
    // bf already allocated to correct size
    int res = LZ4_uncompress((const char*)src,(char*)bf,bf_size);
    return res;
}

/**
   Writes bloom filter out to a file.
   \param filename file to be written 
   \returns status -1 if compression fails, -2 if cannot open file
*/
int32_t
bloom_filter::write_out(string filename) {
    char *compressed=this->compress();
    if (compressed==NULL) 
        return -1;
    std::filebuf fb;
    fb.open (filename.c_str(),ios::out|ios::binary);
    if (fb.is_open()) {
        std::ostream os(&fb);
        os << "sdbf-idx:" << bf_size << ":" << bf_elem_count << ":"<< hash_count;
        os << ":" << bit_mask << ":" << comp_size << ":";
        os << setname;
        os << endl;
        os.write(compressed,comp_size);
        fb.close();
    } else {
        free(compressed);
        return -2;
    }
    free(compressed);
    return 0;
}

/**
   Returns name associated with bloom filter
   \returns name 
*/
string
bloom_filter::name() const{
     return setname;    
}

/**
   Changes name associated with bloom filter
   \param name new name
*/
void
bloom_filter::set_name(string name) {
     setname = name;
}

int 
bloom_filter::bloom_id() {
    return bl_id;
}

void 
bloom_filter::set_bloom_id(int id) {
    this->bl_id = id;
}
/**
   Folds bloom filter by half N times by or'ing the 
   second half of the bloom filter onto the first half. 
   \param times amount of times to fold filter
*/
void
bloom_filter::fold(uint32_t times){
    // probably also need some sort of mutex while this happens
    // size divided by 8,to use 64-bit chunks, and cast the bf 
    uint64_t rsize = bf_size/8;
    uint64_t *bf_64 = (uint64_t *)bf;
    for (uint32_t i=0; i<times; i++) {
        for (uint64_t j=0;j<rsize/2;j++) 
            bf_64[j] |= bf_64[j+(rsize/2)];
        rsize=rsize/2;
        if (rsize == 32) 
            break; // also error?
    }
    bf_size=rsize*8;
    // recalculate mask
        // Find log2(size)
    uint16_t log_size = 0;
    for( uint64_t tmp=bf_size; tmp; tmp >>= 1, log_size++);
    bit_mask = BIT_MASKS_32[log_size+1];
    uint8_t *oldbf=bf;
    bf = (uint8_t*)malloc(bf_size);
    memset(bf, 0, bf_size);
    // copy in
    memcpy(bf,oldbf,bf_size);
    // delete
    free(oldbf);
}

/**
   Adds another bloom filter to this one
   \param other bloom filter
   \return 0 if successful 1 if not the same size
*/
int
bloom_filter::add(bloom_filter *other) {
    uint64_t *bf_64 = (uint64_t *)bf;
    uint64_t *bf2_64 = (uint64_t *)other->bf;
    if (other->bf_size != bf_size) 
        return 1; // must add two of same size
    for (uint32_t j=0;j < bf_size/8;j++)
        bf_64[j]|=bf2_64[j];
    return 0;
}

/** 
   Inserts hash data into this bloom filter
   \param sha1 buffer of sha1 hash values
   \returns exists or not exists
*/
bool 
bloom_filter::insert_sha1(uint32_t *sha1) {
    return query_and_set(sha1, true);
}
    
/** 
   Queries this bloom filter with hash data 
   \param sha1 buffer of sha1 hash values
   \returns exists or not exists
*/
bool
bloom_filter::query_sha1(uint32_t *sha1) {
    return query_and_set(sha1, false);
}

/**
   \internal
    Actual meat of insert/query functionality
   \param sha1 
   \param mode_set true to set, false to query
   \returns exists or not exists
*/
bool
bloom_filter::query_and_set(uint32_t *sha1, bool mode_set) {
    
    uint32_t pos, i, k, bit_cnt=0;
    for( i=0; i<hash_count; i++) {
        pos = sha1[i] & bit_mask;
        k = pos >> 3;
        // Bit is set
        if( (bf[k] & BITS[pos & 0x7])) {
            bit_cnt++;
        // Bit is not set
        } else {
            if( mode_set)
                bf[k] |= BITS[pos & 0x7];
            else
                return false;
        }        
    }
    if(mode_set) {
        if(bit_cnt < hash_count) {
            bf_elem_count++;
            return true;
        } else
            return false;
    } else
        return bit_cnt == hash_count;
}

int
bloom_filter::compare(bloom_filter *other, double scale) {
    if (this->bf_size != other->bf_size) 
        return -1; // must compare equal sized filters
    uint64_t *bf_64 = (uint64_t *)bf;
    uint64_t *bf2_64 = (uint64_t *)other->bf;
    int64_t res=0;
    for (uint i=0 ; i < bf_size / 8 ; i++) {
        res+=_mm_popcnt_u64(bf_64[i] & bf2_64[i]);
    }
    int max_est = (this->hamminglg < other->hamminglg) ? this->hamminglg : other->hamminglg;
    double m = bf_size*8;
    double k = 5;
    double exp = 1-1.0/m;
    int x=this->bf_elem_count;
    int y=other->bf_elem_count;
    int min_est = boost::math::round((double)m*(1 - pow(exp,(double)k*x) - pow(exp,(double)k*y) + pow(exp,(double)k*(x+y))) );
    int cut_off=boost::math::round((scale*(double)(max_est-min_est))+(double)min_est);
//cout << "scale*max_est-min_est " << scale*(double)(max_est-min_est) << " min_est " << min_est << endl;
//max_est = boost::math::round((double)max_est / (double)(bf_size/256));
    int32_t result=(res > cut_off) ? boost::math::round<int32_t>(100.0*(double)(res-cut_off)/(double)(max_est-cut_off)) : 0;
cout << " " << (int)(m/bf_elem_count) << " " <<min_est << " " << res << " " << max_est <<" " << cut_off << " "; // << endl;
    //int32_t result=res; // TEMP DEBUGGING
    return result;

}
/** 
   \internal
   Temporary for output
*/
string
bloom_filter::to_string () const {
    std::stringstream string_bf;
    //string_bf << "sdbf-mr:" << bf_size << ":" << bf_elem_count << ":"<< hash_count;
    //string_bf << ":" << bit_mask << ":0:" ; // 0 is compressed size, we are not compressed
    //string_bf << setname << ":" ;
    char *b64=b64encode((char*)bf,bf_size+3);
    string_bf << b64; 
   // string_bf << endl;
    free(b64);
    return string_bf.str();
}

// possibly change folds to target size? 
bloom_filter::bloom_filter(string filter, int folds) {
    std::istringstream iss(filter);
    //if (ifs.is_open()) {
    string process;
    // headerbit
    getline(iss,process,':'); // ignore
    // bf_size
    getline(iss,process,':');
    bf_size=boost::lexical_cast<uint64_t>(process);
    // elem_count
    getline(iss,process,':');
    bf_elem_count=boost::lexical_cast<uint64_t>(process);
    // hash_count
    getline(iss,process,':');
    hash_count=boost::lexical_cast<uint16_t>(process);
    // bit_mask
    getline(iss,process,':');
    bit_mask=boost::lexical_cast<uint64_t>(process);
    // compressed_size
    getline(iss,process,':');
    //uint64_t comp_size=boost::lexical_cast<uint64_t>(process);
    // setname
    getline(iss,setname,':');
    // endl
    getline(iss,process); // raw BF left base64
    // allocate size
    bf = new uint8_t[bf_size];
    // decode
    int len;
    bf = (uint8_t *)b64decode((char*)process.c_str(),process.length(),&len);
    created=true;
    hamming=0;
    fold(folds);
    compute_hamming();
}

void
bloom_filter::compute_hamming() {
    hamming=0;
    hamminglg=0;
    uint64_t *b64 = (uint64_t *)this->bf;
    for( uint j=0; j<bf_size/8; j++) {
        hamminglg += _mm_popcnt_u64(b64[j]);
    }
}
