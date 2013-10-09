#include <vector>

#include "bloom_filter.h"
#include "blooms.pb.h"


class bloom_vector {

public:
    bloom_vector(blooms::BloomVector *vect);
    void add_filter(blooms::BloomFilter *srcfilter, int folds,bool collapse);
    int compare(bloom_vector *other, double scale);
//    void collapse();
//    void fold(int folds);
    string name();
    std::vector<bloom_filter*> *items;

private:
    string objname;
    int filter_count;
    uint64_t filesize;
};
