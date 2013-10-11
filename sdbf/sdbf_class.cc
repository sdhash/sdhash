// sdbf_class.cc 
// Authors: Candice Quates, Vassil Roussev
// implementation of sdbf object

#include "sdbf_class.h"
#include "sdbf_defines.h"

#define SDBF_VERSION 3 

#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>

#include <boost/lexical_cast.hpp>

using namespace std;
/** 
    \internal
    Initialize static configuration object with sensible defaults.
*/
sdbf_conf *sdbf::config = new sdbf_conf(1, FLAG_OFF, _MAX_ELEM_COUNT, _MAX_ELEM_COUNT_DD);

/** 
    Create new sdbf from file.  dd_block_size turns on "block" mode. 
    \param filename file to hash
    \param dd_block_size size of block to process file with. 0 is off.

    \throws exception if file cannot be opened or too small 
*/
sdbf::sdbf(const char *filename, uint32_t dd_block_size) {
    processed_file_t *mfile = process_file( filename, MIN_FILE_SIZE, config->warnings);
    if( !mfile)
        throw -1 ; // cannot process file
    sdbf_create(filename);
    this->info=NULL;
    this->orig_file_size=mfile->size;
    if (!dd_block_size) {  // stream mode
        this->max_elem = config->max_elem;
        gen_chunk_sdbf( mfile->buffer,mfile->size, 32*MB);
    } else {  // block mode
        this->max_elem = config->max_elem_dd;
        uint64_t dd_block_cnt =  mfile->size/dd_block_size;
        if( mfile->size % dd_block_size >= MIN_FILE_SIZE)
            dd_block_cnt++;
        this->bf_count = dd_block_cnt;
        this->dd_block_size = dd_block_size;
        this->buffer = (uint8_t *)alloc_check( ALLOC_ZERO, dd_block_cnt*config->bf_size, "sdbf_hash_dd", "this->buffer", ERROR_EXIT);
        this->elem_counts = (uint16_t *)alloc_check( ALLOC_ZERO, sizeof( uint16_t)*dd_block_cnt, "sdbf_hash_dd", "this->elem_counts", ERROR_EXIT);
        gen_block_sdbf_mt( mfile->buffer, mfile->size, dd_block_size, config->thread_cnt);
    }
    free( mfile->buffer);
    compute_hamming();
    free(mfile);
} 

/**
    Generates a new sdbf, with a maximum size read from an open stream.
    dd_block_size enables block mode.
    \param name name of stream
    \param ifs open istream to read raw data from
    \param dd_block_size size of block to divide data with. 0 is off.
    \param msize amount of data to read and process
    \param info block of information about indexes

    \throws exception if file cannot be opened or too small 
*/
sdbf::sdbf(const char *name, std::istream *ifs, uint32_t dd_block_size, uint64_t msize,index_info *info) { 
    uint64_t chunk_size;
    uint8_t *bufferinput;    

    bufferinput = (uint8_t*)alloc_check(ALLOC_ZERO, sizeof(uint8_t)*msize,"sdbf_hash_stream",
        "buffer input", ERROR_EXIT);
    ifs->read((char*)bufferinput,msize);
    chunk_size = ifs->gcount();
    if (chunk_size < MIN_FILE_SIZE) {
        free(bufferinput);
        throw -3; // too small
    }
    sdbf_create(name); // change to filename+offset / size
    this->info=info;
    this->orig_file_size=chunk_size;
    if (!dd_block_size) {  // single stream mode should not be used but we'll support it anyway
        this->max_elem = config->max_elem;
        gen_chunk_sdbf(bufferinput,msize, 32*MB);
    } else { // block mode
        this->max_elem = config->max_elem_dd;
        uint64_t dd_block_cnt =  msize/dd_block_size;
        if( msize % dd_block_size >= MIN_FILE_SIZE)
            dd_block_cnt++;
        this->bf_count = dd_block_cnt;
        this->dd_block_size = dd_block_size;
        this->buffer = (uint8_t *)alloc_check( ALLOC_ZERO, dd_block_cnt*config->bf_size, "sdbf_hash_dd", "this->buffer", ERROR_EXIT);
        this->elem_counts = (uint16_t *)alloc_check( ALLOC_ZERO, sizeof( uint16_t)*dd_block_cnt, "sdbf_hash_dd", "this->elem_counts", ERROR_EXIT);
        gen_block_sdbf_mt( bufferinput, msize, dd_block_size, config->thread_cnt);
    }
    compute_hamming();
    free(bufferinput);
}

/**
    Generates a new sdbf, from a char *string
    dd_block_size enables block mode.
    \param name name of stream
    \param str input to be hashed
    \param dd_block_size size of block to divide data with. 0 is off.
    \param length length of str to be hashed
    \param info block of information about indexes

    \throws exception if data too small 
*/
sdbf::sdbf(const char *name, char *str, uint32_t dd_block_size, uint64_t length,index_info *info) { 
    if (length < MIN_FILE_SIZE) 
        throw -3; // too small
    sdbf_create(name); 
    this->info=info;
    this->orig_file_size=length;
    if (!dd_block_size) {  // single stream mode should not be used but we'll support it anyway
        this->max_elem = config->max_elem;
        gen_chunk_sdbf((uint8_t*)str,length, 32*MB);
    } else { // block mode
        this->max_elem = config->max_elem_dd;
        uint64_t dd_block_cnt =  length/dd_block_size;
        if( length % dd_block_size >= MIN_FILE_SIZE)
            dd_block_cnt++;
        this->bf_count = dd_block_cnt;
        this->dd_block_size = dd_block_size;
        this->buffer = (uint8_t *)alloc_check( ALLOC_ZERO, dd_block_cnt*config->bf_size, "sdbf_hash_dd", "this->buffer", ERROR_EXIT);
        this->elem_counts = (uint16_t *)alloc_check( ALLOC_ZERO, sizeof( uint16_t)*dd_block_cnt, "sdbf_hash_dd", "this->elem_counts", ERROR_EXIT);
        gen_block_sdbf_mt( (uint8_t*)str, length, dd_block_size, config->thread_cnt);
    }
    compute_hamming();
}


/** Reads a generated sdbf from a string.
    \param str string reference

    \throws exception if processing fails
*/
sdbf::sdbf(const std::string& str) {
    if (str.length() < 344) // less than absolute minimum, but probably good enough.
        throw -1; // too small!
    sdbf_create(NULL);

    std::stringstream ss(str);
    string magic,process;
    std::getline(ss,magic,':');
    if (magic.compare(0,4,"sdbf")) 
        throw -2;  // invalid format
    std::getline(ss,process,':');
    std::getline(ss,process,':'); 
    std::getline(ss,process,':'); // copy+allocate in
    this->filenamealloc=true;
    this->hashname = (char*)alloc_check( ALLOC_ZERO, process.length()+1, "sdbf_from_stream", "this->hashname", ERROR_EXIT);
    strncpy(this->hashname,process.c_str(),process.length()+1);
    std::getline(ss,process,':');  // set
    this->orig_file_size=boost::lexical_cast<uint64_t>(process);
    std::getline(ss,process,':');  // sha1 // auto
    std::getline(ss,process,':');  // bloom size (256) 
    this->bf_size=boost::lexical_cast<uint32_t>(process);
    std::getline(ss,process,':');  // 5 -- can auto-set
    this->hash_count=boost::lexical_cast<uint16_t>(process);
    std::getline(ss,process,':');  // mask: we always look up
    std::getline(ss,process,':');  // max elem 
    this->max_elem=boost::lexical_cast<uint32_t>(process);
    std::getline(ss,process,':');  // filter count
    this->bf_count=boost::lexical_cast<uint32_t>(process);
    this->buffer = (uint8_t *)alloc_check( ALLOC_ZERO, this->bf_count*this->bf_size, "sdbf_from_stream", "this->buffer", ERROR_EXIT);
    // dd fork
    int d_len = 0;
    if (!strcmp(magic.c_str(), MAGIC_DD)) { 
       std::getline(ss,process,':');  // dd block size
       this->dd_block_size=boost::lexical_cast<uint32_t>(process);
       // set up elem counts array
       this->elem_counts = (uint16_t *)alloc_check( ALLOC_ZERO, this->bf_count*sizeof(uint16_t), "sdbf_from_stream", "this->elem_counts", ERROR_EXIT);
       for (uint32_t i=0; i< this->bf_count; i++) {
           uint32_t tmpelem=0;
           std::getline(ss,process,':'); // elem_counts
           std::stringstream sstmp;
           sstmp << std::hex << process;
           sstmp >> tmpelem;
           this->elem_counts[i]=boost::lexical_cast<uint16_t>(tmpelem);
           std::getline(ss,process,':'); // buffer
           d_len = b64decode_into( (uint8_t*)process.c_str(), 344, this->buffer + i*this->bf_size); 
           if( d_len != 256) {
               if (config->warnings)
                   fprintf( stderr, "ERROR: Unexpected decoded length for BF: %d. name: %s, BF#: %d\n", d_len, this->hashname, (int)i);
               throw -2; // unsupported format - caller should exit
           }
       }
    // stream fork 
    } else {
        std::getline(ss,process,':');  // last count
        this->last_count=boost::lexical_cast<uint32_t>(process);
        uint32_t b64_len = this->bf_count*this->bf_size;
        b64_len = 4*(b64_len/3 +1*(b64_len % 3 > 0 ? 1 : 0));
        char *b64 = (char*)alloc_check( ALLOC_ZERO, b64_len+2, "sdbf_from_stream", "b64", ERROR_EXIT);
        ss.read(b64,b64_len);
        free(this->buffer);
        this->buffer =(uint8_t*) b64decode( (char*)b64, (int)b64_len, &d_len);
        if( (uint32_t)d_len != this->bf_count*this->bf_size) {
            if (config->warnings)
                fprintf( stderr, "ERROR: Incorrect base64 decoding length. Expected: %d, actual: %d\n", this->bf_count*this->bf_size, d_len);
            free (b64); // cleanup in case of wanting to go on
            throw -2; // unsupported format, caller should exit
        }
        free( b64);
    }
    compute_hamming();
    this->info=NULL;
}

/**
    Destroys this sdbf
*/
sdbf::~sdbf() {
    if (buffer)
        free(buffer);
    if (hamming)
        free(hamming);
    if (elem_counts)
        free(elem_counts);
    if (filenamealloc)
        free(hashname);
} 

/**
    Returns the name of the file or data this sdbf represents.
    \returns char* of file name
*/
const char *
sdbf::name() {
    return (char*)this->hashname;
} 

/** 
    Returns the size of the hash data for this sdbf
    \returns uint64_t length value
*/
uint64_t
sdbf::size() {
    return (this->bf_size)*(this->bf_count);
}

/** 
    Returns the size of the data that the hash was generated from.
    \returns uint64_t length value
*/
uint64_t
sdbf::input_size() {
    return this->orig_file_size;
}


/**
 * Compares this sdbf to other passed sdbf, returns a confidence score
    \param other sdbf* to compare to self
    \param sample sets the number of BFs to sample - 0 uses all
    \returns int32_t confidence score
*/
int32_t
sdbf::compare( sdbf *other, uint32_t sample) {
    if (config->warnings)
        cerr << this->name() << " vs " << other->name() << endl;

    return sdbf_score( this, other, sample);
}

/** 
    Write this sdbf to stream 
*/
ostream& operator<<(ostream& os, const sdbf& s) {
    os << s.to_string();
    return os;
}

/**
    Write sdbf to stream 
*/
ostream& operator<<(ostream& os, const sdbf *s) {
    os << s->to_string();
    return os;
}

/**
    Encode this sdbf and return it as a string.
    \returns std::string containing sdbf suitable for display or writing to file
*/
string
sdbf::to_string () const { // write self to stream
    std::stringstream hash;
    // Stream version
    if( !this->elem_counts) {
        hash.fill('0');
        hash << MAGIC_STREAM << ":" << setw (2) << SDBF_VERSION << ":";    
        hash << (int)strlen((char*)this->hashname) << ":" << this->hashname << ":" << this->orig_file_size << ":sha1:";    
        hash << this->bf_size << ":" << this->hash_count<< ":" << hex << this->mask << ":" << dec;    
        hash << this->max_elem << ":" << this->bf_count << ":" << this->last_count << ":";    
        uint64_t qt = this->bf_count/6, rem = this->bf_count % 6;
        uint64_t i, pos=0, b64_block = 6*this->bf_size;
        for( i=0,pos=0; i<qt; i++,pos+=b64_block) {
            char *b64 = b64encode( (char*)this->buffer + pos, b64_block);
            hash << b64;
            free( b64);
        }
        if( rem>0) {
            char *b64 = b64encode( (char*)this->buffer + pos, rem*this->bf_size);
            hash << b64;
            free( b64);
        }
    } else { // block version
        hash.fill('0');
        hash << MAGIC_DD << ":" << setw (2) << SDBF_VERSION << ":";    
        hash << (int)strlen((char*)this->hashname) << ":" << this->hashname << ":" << this->orig_file_size << ":sha1:";    
        hash << this->bf_size << ":" << this->hash_count<< ":" << hex << this->mask << ":" << dec;    
        hash << this->max_elem << ":" << this->bf_count << ":" << this->dd_block_size ;
        uint32_t i;
        for( i=0; i<this->bf_count; i++) {
            char *b64 = b64encode( (char*)this->buffer+i*this->bf_size, this->bf_size);
            hash << ":" << setw (2) << hex << this->elem_counts[i];
            hash << ":" << b64;    
            free(b64);
        }
    }
    hash << endl;
    return hash.str();
}

string
sdbf::get_index_results() const{
    return index_results;
}
/** 
    Clones a copy of a single bloom filter in
    this sdbf.  
    
    Warning: 256-bytes long, not terminated, may contain nulls.

    \param position index of bloom filter
    \returns uint8_t* pointer to 256-byte long bloom filter
*/
uint8_t*
sdbf::clone_filter(uint32_t position) {
    if (position < this->bf_count) {
        uint8_t *filter=(uint8_t*)alloc_check(ALLOC_ZERO,bf_size*sizeof(uint8_t),"single_bloom_filter","return buffer",ERROR_EXIT);
        memcpy(filter,this->buffer + position*bf_size,bf_size);
        return filter;    
    } else {
        return NULL;
    }
}
        
/** \internal
 * Create and initialize an sdbf structure ready for stream mode.
 */
void 
sdbf::sdbf_create(const char *name) {
    this->hashname = (char*)name;
    this->bf_size = config->bf_size;
    this->hash_count = 5;
    this->mask = config->BF_CLASS_MASKS[0];
    this->bf_count = 1;
    this->last_count = 0;
    this->elem_counts= 0;
    this->orig_file_size = 0;
    this->hamming = NULL;
    this->buffer = NULL;
    this->info=NULL;
    this->filenamealloc=false;
}


/** \internal
 * Pre-compute Hamming weights for each BF and adds them to the SDBF descriptor.
 */ 
int 
sdbf::compute_hamming() {
    uint32_t pos, bf_count = this->bf_count;
    this->hamming = (uint16_t *) alloc_check( ALLOC_ZERO, bf_count*sizeof( uint16_t), "compute_hamming", "this->hamming", ERROR_EXIT);
        
    uint64_t i, j;
    uint16_t *buffer16 = (uint16_t *)this->buffer;
    for( i=0,pos=0; i<bf_count; i++) {
        for( j=0; j<BF_SIZE/2; j++,pos++) {
            this->hamming[i] += config->bit_count_16[buffer16[pos]];
        }
    }
    return 0;
}

/** \internal
   get element count for comparisons 
*/
int32_t 
sdbf::get_elem_count( sdbf *mine,uint64_t index) {
    if( !mine->elem_counts) {
        return (index < mine->bf_count-1) ? mine->max_elem : mine->last_count; 
    // DD fork
    } else {
        return mine->elem_counts[index];
    }

}

uint32_t
sdbf::filter_count() {
    return bf_count;
}

