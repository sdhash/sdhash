#include <vector>

#include "bloom_filter.h"
#include "blooms.pb.h"

#ifndef BLOOM_VECTOR_H
#define BLOOM_VECTOR_H

class bloom_vector {

public:
    bloom_vector(blooms::BloomVector *vect);
    void add_filter(blooms::BloomFilter *srcfilter);
    int compare(bloom_vector *other, double scale);
    string name();
    string details();
    std::vector<bloom_filter*> *items;

    int filter_count;
    uint64_t filesize;

private:
    string objname;
};

#endif
