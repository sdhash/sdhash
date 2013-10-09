#include "sdbf_class.h"
#include "sdbf_defines.h"
#include "sdhash-srv.h"
#include "set_list.h"

#include <boost/filesystem.hpp>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>

#include <boost/thread/thread.hpp>

namespace fs = boost::filesystem;
using namespace std;
#define CHUNK_SIZE 128*MB
 

/** 
    thread "process" function for file-by-file hashing
*/

void
*thread_sdbf_hashfile( void *task_param) {
    filehash_task_t *task = (filehash_task_t *)task_param;
    struct stat file_stat;
    int32_t chunks = 0,csize = 0;
    ifstream *is = new ifstream();

    int i;
    for( i=task->tid; i<task->file_count; i+=task->tcount) {
        if (stat(task->filenames[i],&file_stat))
            continue;
        is->open(task->filenames[i], ios::binary);
        try {
            class sdbf *sdbfm = new sdbf(task->filenames[i],is,0,file_stat.st_size,task->info);
            task->addset->add(sdbfm);
        } catch (int e) {
            continue;
        }
        is->close();
    }
    delete is;
    return 0;
}

/**
 * Compute SD for a list of files & add them to a new set.
 * Not block-wise.
 */
void
sdbf_hash_files( char **filenames, uint32_t file_count, int32_t thread_cnt,sdbf_set *addto, index_info *info ) {
    int32_t i, t, j;
    //sdbf_set *myset = new sdbf_set();
    struct stat file_stat;
    int32_t chunks = 0,csize = 0;
    ifstream *is = new ifstream();

    // Sequential implementation
    if( thread_cnt == 1) {
        for( i=0; i<file_count; i++) {
            if (stat(filenames[i],&file_stat))
                continue;
            is->open(filenames[i], ios::binary);
            try {
                if (!addto) {
                    class sdbf *sdbfm = new sdbf(filenames[i],is,0,file_stat.st_size,info);
                    cout << sdbfm ;
                    delete sdbfm;
                } else  {
                    class sdbf *sdbfm = new sdbf(filenames[i],is,0,file_stat.st_size,info);
                    addto->add(sdbfm);
                }
            } catch (int e) {
                 continue; // failure is always an option.
            }
            is->close();
        }
    // Threaded implementation
    } else {
      boost::thread *thread_pooll[MAX_THREADS];
        filehash_task_t *tasks = (filehash_task_t *) alloc_check( ALLOC_ZERO, thread_cnt*sizeof( filehash_task_t), "sdbf_hash_files", "tasks", ERROR_EXIT);
        for( t=0; t<thread_cnt; t++) {
            tasks[t].tid = t;
            tasks[t].tcount = thread_cnt;
            tasks[t].filenames = filenames;
            tasks[t].file_count = file_count;
            tasks[t].addset = addto;
            tasks[t].info = info;
         thread_pooll[t] = new boost::thread(thread_sdbf_hashfile,tasks+t);
        }
        for( t=0; t<thread_cnt; t++) {
            thread_pooll[t]->join();
        }
        for( t=0; t<thread_cnt; t++) {
            delete thread_pooll[t];
        }
      free(tasks);
    // End threading
    }
    delete is;
}

/**
 * Compute SD for a vector containing a list of files & add them to sets
 */
sdbf_set*
hash_stringlist(const std::vector<std::string> & filenames, uint32_t dd_block_size,uint32_t thread_cnt, index_info *info) {
    //bloom_filter *index1=new bloom_filter(512*MB,5,0,0.01);
    sdbf_set *set1 = new sdbf_set(info->index);
    int i;
    std::vector<string> small;
    std::vector<string> large;
    for (vector<string>::const_iterator it=filenames.begin(); it < filenames.end(); it++) {
        if (fs::is_regular_file(*it)) {
            if (fs::file_size(*it) < 16*MB) {
                    small.push_back(*it);
            } else {
                large.push_back(*it);
            }
        }
    }
    int smallct=small.size();
    int largect=large.size();

    if (smallct > 0) {
        char **smalllist=(char **)alloc_check(ALLOC_ONLY,smallct*sizeof(char*),"main", "filename list", ERROR_EXIT);
        for (i=0; i < smallct ; i++) {
            smalllist[i]=(char*)alloc_check(ALLOC_ONLY,small[i].length()+1, "main", "filename", ERROR_EXIT);
            strncpy(smalllist[i],small[i].c_str(),small[i].length()+1);
        }
        if (dd_block_size < 1 )  {
            sdbf_hash_files( smalllist, smallct,thread_cnt,set1,info);
        } else {
            sdbf_hash_files_dd( smalllist, smallct,dd_block_size*KB,128*MB,set1,info);
        }
    }
    if (largect > 0) {
        char **largelist=(char **)alloc_check(ALLOC_ONLY,largect*sizeof(char*),"main", "filename list", ERROR_EXIT);
        for (i=0; i < largect ; i++) {
            largelist[i]=(char*)alloc_check(ALLOC_ONLY,large[i].length()+1, "main", "filename", ERROR_EXIT);
            strncpy(largelist[i],large[i].c_str(),large[i].length()+1);
        }
        if (dd_block_size < 1) {
            sdbf_hash_files_dd(largelist, largect, 16*KB, 128*MB,set1,info);
        } else {
            sdbf_hash_files_dd(largelist, largect, dd_block_size*KB, 128*MB,set1,info);
        }
       
    }
    return set1;
}

void
sdbf_hash_files_dd( char **filenames, uint32_t file_count, uint32_t dd_block_size, uint64_t chunk_size, sdbf_set *addto, index_info *info) {
    int32_t i,j, result = 0;
    struct stat file_stat;
    uint64_t chunks = 0,csize = 0;
    ifstream *is = new ifstream();
    int tailflag = 0;
    uint64_t filesize;
    for( i=0; i<file_count; i++) {
        tailflag=0;
    filesize=fs::file_size(filenames[i]);
    is->open(filenames[i], ios::binary);
    if (filesize > chunk_size && chunk_size > 0) {
        chunks = filesize / (chunk_size) ;
        // adjusting for too small of a last fragment
        // on a single segment piece
        if (filesize - chunk_size <= 512) {
            chunks=0;
        }
        // last segment adjustment
        if (filesize - chunks*chunk_size <= 512) {
            chunks--;
            tailflag=1;
        }
        for (j=0;j<=chunks;j++) {
            std::stringstream namestr;
            namestr << filenames[i];
            if (j > 0 || (chunks>0)) { // assuming MB sizing.
                namestr.fill('0');
                namestr << "." << setw(4) << j*chunk_size/MB << "M" ;
                }
                string myname = namestr.str();
                char *fname = (char*)alloc_check(ALLOC_ONLY,myname.length()+1,"hash_files_dd","filename",ERROR_EXIT);
                strncpy(fname,myname.c_str(),myname.length()+1);
                if (j==chunks) {
                    csize=filesize-(chunks*chunk_size) ;
                    if (tailflag)
                        csize=csize+(filesize-((chunks+1)*chunk_size)) ;
                } else {
                    csize=chunk_size;
                }
                try {
                    class sdbf *sdbfm = new sdbf(fname,is,dd_block_size,csize,info);
                    addto->add(sdbfm);
                } catch (int e) {
            continue; // failure allowed
                }
            }
        } else {
            try {
                class sdbf *sdbfm = new sdbf(filenames[i],is,dd_block_size,filesize,info);
                addto->add(sdbfm);
            } catch (int e) {
                continue; // failure allowed
            }
        }
        is->close();
    }
    delete is;
}

