%module sdbf_class
%{
#include "../../sdbf/sdbf_class.h"
#include "../../sdbf/sdbf_conf.h"
#include "../../sdbf/sdbf_defines.h"
#include <stdint.h>
#include <stdio.h>

%}

#define KB 1024
%include "cpointer.i"
%include "std_string.i"

%pointer_functions(int, intp);

// note these are the linux x86_64 types
typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned long int uint64_t;
typedef long int int64_t;


class sdbf_conf {

public:
    sdbf_conf(uint32_t thread_cnt, uint32_t warnings, uint32_t max_elem_ct, uint32_t max_elem_ct_dd ); // defaults
    // later: rc file for this
    ~sdbf_conf(); // destructor
};


/// sdbf class
class sdbf {

    //friend std::ostream& operator<<(std::ostream& os, const sdbf& s); ///< output operator
    //friend std::ostream& operator<<(std::ostream& os, const sdbf *s); ///< output operator

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
    std::string to_string() const ; 

    /// return results of index search
    std::string get_index_results() const; 

    /// return a copy of an individual bloom filter from this sdbf
    uint8_t *clone_filter(uint32_t position);
    uint32_t filter_count();

public:
    /// global configuration object
    static class sdbf_conf *config;  
    static int32_t get_elem_count(sdbf *mine, uint64_t index) ;

};


