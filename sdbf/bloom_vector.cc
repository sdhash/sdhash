// bloom vector class implementation
// this is to represent a single object as a vector of blooms.
// author: candice quates

#include "bloom_vector.h"
#include "bloom_filter.h"
#include "blooms.pb.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <smmintrin.h>

#include <boost/math/special_functions/round.hpp>

#include "../lz4/lz4.h"
#include "sdbf_defines.h"


// NOTE: this reads all new-style multi-res'es but not 
// all old-style sdbf's.  something with lengths not matching up
// on some files.
// possibly change folds to target size? 
//bloom_vector::bloom_vector(string filter, int folds) {
bloom_vector::bloom_vector(blooms::BloomVector *vect) {
    items=new std::vector<bloom_filter*>();
    this->objname=vect->name();
    this->filter_count=vect->filter_count();
    this->filesize=vect->filesize();
}

void
bloom_vector::add_filter(blooms::BloomFilter *srcfilter) {
    uint8_t *bf = (uint8_t*)malloc(srcfilter->bf_size());
    uint64_t *b64tmp=(uint64_t*)bf;
    for (int j=0;j< srcfilter->filter_size(); j++) {
        b64tmp[j]=srcfilter->filter(j);
    }
    bloom_filter *tmp=new bloom_filter(bf,srcfilter->bf_size(),srcfilter->id(),srcfilter->elem_count(),0);
    tmp->set_name(srcfilter->name());
    tmp->compute_hamming();
    tmp->max_elem=srcfilter->max_elem();
    items->push_back(tmp);
    free(bf);
}

string
bloom_vector::name() {
    return this->objname;
}

string
bloom_vector::details() {
    std::stringstream detail;
    detail << this->objname << " size " << this->filesize << " filter count " << this->filter_count ;
    return detail.str();
}

int
bloom_vector::compare(bloom_vector *other, double scale) {
    int thislen, otherlen;
    thislen=this->items->size();
    otherlen=other->items->size();
    int32_t *scorematrix=(int32_t*)malloc(sizeof(int32_t)*(thislen*otherlen+1));
    memset(scorematrix, 0, sizeof(uint32_t)*(thislen*otherlen+1));
    int32_t runtotal=0;
    int answer=0;
    for (int i=0; i < thislen ; i++) {
    // could put pragma parallel here
        for (int j=0; j< otherlen; j++) {
            int32_t score=(int32_t)this->items->at(i)->compare(other->items->at(j),scale);
            scorematrix[i*otherlen+j]=score;
        }
    }
    // finds max per row, also parallelizable
    if ( thislen <= otherlen) {
        for (int i=0; i < thislen ; i++) {
            int32_t imax=0;
            for (int j=0 ; j< otherlen; j++)
                if (scorematrix[i*otherlen+j] > imax) imax=scorematrix[i*otherlen+j];
            runtotal+=imax;
        }
        answer=boost::math::round((float)(runtotal/thislen));
    } else { // finds max per column -- this way we take the answer like sdhash
        for (int j=0; j < otherlen; j++) {
            int32_t imax=0;
            for (int i=0 ; i< thislen; i++)
                if (scorematrix[i*otherlen+j] > imax) imax=scorematrix[i*otherlen+j];
            runtotal+=imax;
        }
        answer=boost::math::round((float)(runtotal/otherlen));
    }
#ifdef MAPNO
    if (answer > 0) // MAP  
        for (int i=0; i<thislen; i++) {
            cout <<"|";
            for (int j=0; j< otherlen; j++) {
                if (scorematrix[i*otherlen+j] > 0) cout << "+" ;
                else cout <<"-";
            }
            cout <<"|"<< endl;
        }
#endif
    return answer;
}
