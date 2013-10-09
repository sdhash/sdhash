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
bloom_vector::add_filter(blooms::BloomFilter *srcfilter, int folds, bool collapse) {
    uint8_t *bf = (uint8_t*)malloc(srcfilter->bf_size());
    uint64_t *b64tmp=(uint64_t*)bf;
    for (int j=0;j< srcfilter->filter_size(); j++) {
        b64tmp[j]=srcfilter->filter(j);
    }
    bloom_filter *tmp=new bloom_filter(bf,srcfilter->bf_size(),srcfilter->id(),srcfilter->elem_count(),0);
    tmp->fold(folds);
    tmp->set_name(srcfilter->name());
    if (collapse && items->size() > 0) {
        items->at(0)->add(tmp);
        free(tmp);
        items->at(0)->compute_hamming();
    } else {
        tmp->compute_hamming();
        items->push_back(tmp);
    }
    free(bf);
}

string
bloom_vector::name() {
    return this->objname;
}

int
bloom_vector::compare(bloom_vector *other, double scale) {
    // these scores are always 0-100... 
    int thislen, otherlen;
    thislen=this->items->size();
    otherlen=other->items->size();
    uint8_t *scorematrix=(uint8_t*)malloc(thislen*otherlen+1);
    uint32_t runtotal=0;
    int answer=0;
    for (uint i=0; i < thislen ; i++) {
    // could put pragma parallel here
        for (uint j=0; j< otherlen; j++) {
            uint8_t score=(uint8_t)this->items->at(i)->compare(other->items->at(j),scale);
            scorematrix[i*otherlen+j]=score;
        }
    }
    // finds max per row, also parallelizable
    if ( thislen <= otherlen) {
        for (uint i=0; i < thislen ; i++) {
            uint8_t imax=0;
            for (uint j=0 ; j< otherlen; j++)
                if (scorematrix[i*otherlen+j] > imax) imax=scorematrix[i*otherlen+j];
            runtotal+=imax;
        }
        answer=boost::math::round((float)(runtotal/thislen));
    } else { // finds max per column -- this way we take the answer like sdhash
        for (uint j=0; j < otherlen; j++) {
            uint8_t imax=0;
            for (uint i=0 ; i< thislen; i++)
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
                if (scorematrix[i*otherlen+j] > 0) cout << "+";
                else cout <<"-";
            }
            cout <<"|"<< endl;
        }
#endif
    return answer;
}
