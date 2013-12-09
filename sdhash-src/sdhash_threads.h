
#ifndef __SDHASH_THREADS_H
#define __SDHASH_THREADS_H

void *thread_sdbf_hashfile( void *task_param) ;
void sdbf_hash_files( char **filenames, uint32_t file_count, int32_t thread_cnt, sdbf_set *addto,index_info *info);
sdbf_set *sdbf_hash_stdin(index_info *info);
void sdbf_hash_files_dd( char **filenames, uint32_t file_count, uint32_t dd_block_size, uint64_t chunk_size, sdbf_set *addto, index_info *info);

int32_t hash_index_stringlist(const std::vector<std::string> & filenames, string output_name);

int read_bloom_vectors(std::vector<bloom_vector *> &set,string filename, bool details) ;
string compare_bloom_vectors(std::vector<bloom_vector *> &set,string sep,int threshold, bool quiet) ;
string compare_two_bloom_vectors(std::vector<bloom_vector *> &set, std::vector<bloom_vector *> &set2,string sep,int threshold, bool quiet);
#endif
