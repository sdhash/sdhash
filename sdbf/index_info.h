#include "sdbf_set.h"
#include "bloom_filter.h"

#include <vector>

#ifndef INDEX_INFO
#define INDEX_INFO

typedef struct {
     bloom_filter *index;
     std::vector<bloom_filter *> *indexlist;
     std::vector<sdbf_set *> *setlist;
     bool search_deep;
     bool search_first;
     bool basename;
} index_info ;

#endif

