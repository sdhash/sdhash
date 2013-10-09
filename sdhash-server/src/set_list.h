
#ifndef __SET_LIST_H
#define __SET_LIST_H

sdbf_set* hash_stringlist(const std::vector<std::string> & filenames, uint32_t dd_block_size,uint32_t thread_cnt, index_info *info);
void sdbf_hash_files( char **filenames, uint32_t file_count, int32_t thread_cnt,sdbf_set *addto, index_info *info ) ;
void sdbf_hash_files_dd( char **filenames, uint32_t file_count, uint32_t dd_block_size, uint64_t chunk_size, sdbf_set *addto, index_info *info);

void *thread_sdbf_hashfile( void *task_param) ;


#endif
