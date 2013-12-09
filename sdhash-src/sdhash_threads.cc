// sdhash_threads.cc
// author: candice quates
// threading code for hashing individual files 
// and for hashing stdin.

#include "../sdbf/sdbf_class.h"
#include "../sdbf/sdbf_defines.h"
#include "../sdbf/bloom_vector.h"
#include "../sdbf/bloom_filter.h"
#include "../sdbf/blooms.pb.h"
#include "sdhash.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>

#include <boost/thread/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;
namespace fs = boost::filesystem;

// Global parameters

extern sdbf_parameters_t sdbf_sys;



int
read_bloom_vectors(std::vector<bloom_vector *> &set,string filename, bool details) {
    try {
        fstream fs(filename.c_str(),ios::in|ios::binary);
        if (!fs) {
            cerr << "Protobuf Fail Open "<< endl;
            return -1;
        }
        int size=0;
        blooms::BloomVector vect; // temporary buffers
        blooms::BloomFilter filter;
        while (!fs.eof()) {
            fs.read((char*)&size,sizeof(size));
            char *buf=(char*)malloc(size*sizeof(char));
            fs.read(buf,size);
            if (!fs)
                break; // eof
            string readme;
            readme.assign(buf,size);
            vect.ParseFromString(readme);
            free(buf);
            bloom_vector *tmp=new bloom_vector(&vect);
            for (int i=0;i<vect.filter_count();i++) {
                int fsize;
                fs.read((char*)&fsize,sizeof(fsize));
                char *filtbuf=(char*)malloc(fsize*sizeof(char));
                fs.read(filtbuf,fsize);
                if (!fs)
                    break; // eof
                // trickery with strings is necessary!!!
                string readstr;
                readstr.assign(filtbuf,fsize);
                filter.ParseFromString(readstr);
                tmp->add_filter(&filter);
                free(filtbuf);
                filter.Clear();
            }
            if (details) {
                cout << tmp->details() << endl;
            }
            set.push_back(tmp);
            vect.Clear();
        }
        fs.close();
    } catch (int e) {
        cerr << "sdhash: ERROR: Could not load SDBF-LG file "<< filename << ". Exiting"<< endl;
        return -1;
    }
    return 0;
}

string
compare_bloom_vectors(std::vector<bloom_vector *> &set,string sep,int threshold, bool quiet) {
    std::stringstream ss;
    int end=set.size();
    cout.fill(0);
    ss.fill(0);
    #pragma omp parallel for
    for (int i = 0; i < end ; i++) {
        for (int j = i; j < end ; j++) {
            if (i == j) continue;
               int32_t score = set.at(i)->compare(set.at(j),0);
               if (score >= threshold)  {
                   #pragma omp critical
                   {
                   if (!quiet)
                       cout << set.at(i)->items->at(0)->name() << sep << set.at(j)->items->at(0)->name() ;
                   ss << set.at(i)->items->at(0)->name() << sep << set.at(j)->items->at(0)->name() ;
                   if (score != -1) {
                       if (!quiet)
                           cout << sep << setw (3) << score << std::endl;
                       ss << sep << setw (3) << score << std::endl;
                   } else {
                       if (!quiet)
                           cout << sep <<  score << std::endl;
                       ss << sep <<  score << std::endl;
                   }
                   }
               }
          }
    }
    return ss.str();
}


string
compare_two_bloom_vectors(std::vector<bloom_vector *> &set, std::vector<bloom_vector *> &set2,string sep,int threshold, bool quiet) {
    std::stringstream ss;
    int tend=set.size();
    int qend=set2.size();
    cout.fill(0);
    ss.fill(0);
    #pragma omp parallel for
    for (int i = 0; i < tend ; i++) {
        for (int j = i; j < qend ; j++) {
               int32_t score = set.at(i)->compare(set2.at(j),0);
               if (score >= threshold)  {
                   #pragma omp critical
                   {
                   if (!quiet)
                       cout << set.at(i)->items->at(0)->name() << sep << set2.at(j)->items->at(0)->name() ;
                   ss << set.at(i)->items->at(0)->name() << sep << set2.at(j)->items->at(0)->name() ;
                   if (score != -1) {
                       if (!quiet)
                           cout << sep << setw (3) << score << std::endl;
                       ss << sep << setw (3) << score << std::endl;
                   } else {
                       if (!quiet)
                           cout << sep <<  score << std::endl;
                       ss << sep <<  score << std::endl;
                   }
                   }
               }
          }
    }
    return ss.str();
}
// NOT in sdbf class
/** 
    the actual processing task for the file-list hashing without block mode
*/
void 
*thread_sdbf_hashfile( void *task_param) {
    filehash_task_t *task = (filehash_task_t *)task_param;
    struct stat file_stat;
    ifstream *is = new ifstream();

    uint32_t i;
    for( i=task->tid; i<task->file_count; i+=task->tcount) {
        if (stat(task->filenames[i],&file_stat))
            continue;
        is->open(task->filenames[i], ios::binary);
        try {
        if (sdbf_sys.verbose) 
            cerr << "sdhash: digesting file " << task->filenames[i] << endl;
            if (!task->addset) {
                class sdbf *sdbfm = new sdbf(task->filenames[i],is,0,file_stat.st_size,task->info);
                cout << sdbfm;
                delete sdbfm;            
            } else  {
                class sdbf *sdbfm = new sdbf(task->filenames[i],is,0,file_stat.st_size,task->info);
                task->addset->add(sdbfm);
            }
            
        } catch (int e) {
            if (e==-2)
               exit(-2);
            if (e==-3 && sdbf_sys.warnings) {
                    cerr <<"Input file too small for processing: "<< task->filenames[i] << endl;
            }
       }
       is->close();
    }
    delete is;
    return 0;
}

sdbf_set
*sdbf_hash_stdin(index_info *info) {
    sdbf_set *myset = new sdbf_set();
    const char *default_name= "stdin";
    char *stream;
    int32_t j = 0;
    if (sdbf_sys.filename == NULL) {
        stream = (char*)default_name;
    } else {
        stream = sdbf_sys.filename;
    }
    
    char *readbuf = (char *)alloc_check(ALLOC_ONLY,sizeof(char)*sdbf_sys.segment_size, "sdbf_hash_stdin","read buffer",ERROR_EXIT);
    if (readbuf!=NULL) {
        int done = 0;
        while (!done) {
            std::stringstream namestr;
            namestr << stream;
            memset(readbuf,0,sdbf_sys.segment_size);
            size_t sz = fread(readbuf,1,sdbf_sys.segment_size,stdin);
            if (sz != sdbf_sys.segment_size) 
                done = 1;
            //if (j >0) { // always start at 0M
                namestr.fill('0');
                namestr << "." << setw(4) << j*sdbf_sys.segment_size/MB << "M" ;
         //}
            string myname = namestr.str();
            char *fname = (char*)alloc_check(ALLOC_ONLY,myname.length()+1, "sdbf_hash_stdin", "generated filename",ERROR_EXIT);
            strncpy(fname,myname.c_str(),myname.length()+1);
            if (sdbf_sys.verbose) 
            cerr << "sdhash: "<< stream << " segment begin "<< j*sdbf_sys.segment_size/MB << "M" << endl;
            try {
                class sdbf *sdbfm = new sdbf(fname,readbuf,sdbf_sys.dd_block_size*KB,sz, info);
                //myset->insert(sdbfm);
                myset->add(sdbfm);
            } catch (int e) {
                if (e==-2)
                   exit(-2);
            } 
            j++;
        }
    }
    if (sdbf_sys.verbose) 
      cerr << "sdhash: finished hashing stdin." << endl;
    return myset;
}

/**
 * Compute SD for a list of files & add them to a new set.
 * Not block-wise.
 \returns sdbf_set with the hashes in it
 */
void
sdbf_hash_files( char **filenames, uint32_t file_count, int32_t thread_cnt,sdbf_set *addto, index_info *info ) {
    uint32_t i;
    int32_t t;
    //sdbf_set *myset = new sdbf_set();
    struct stat file_stat;
    ifstream *is = new ifstream();

    // Sequential implementation
    if( thread_cnt == 1) {
        for( i=0; i<file_count; i++) {
            if (stat(filenames[i],&file_stat))
                continue;
            is->open(filenames[i], ios::binary);
            try {
            if (sdbf_sys.verbose) 
               cerr << "sdhash: digesting file " << filenames[i] << endl;
                if (!addto) {
                    class sdbf *sdbfm = new sdbf(filenames[i],is,0,file_stat.st_size,info);
                    cout << sdbfm ;
                    delete sdbfm;
                } else  {
                    class sdbf *sdbfm = new sdbf(filenames[i],is,0,file_stat.st_size,info);
                    addto->add(sdbfm);
                }
            } catch (int e) {
                if (e==-2)
                   exit(-2);
                if (e==-3 && sdbf_sys.warnings) {
               cerr << "Input file too small for processing: "<< filenames[i]<< endl;
                }
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

void
sdbf_hash_files_dd( char **filenames, uint32_t file_count, uint32_t dd_block_size, uint64_t chunk_size, sdbf_set *addto, index_info *info) {
    uint32_t i,j;
    uint64_t chunks = 0,csize = 0;
    ifstream *is = new ifstream();
    int tailflag = 0;
    uint64_t filesize;
    for( i=0; i<file_count; i++) {
        tailflag=0;
        filesize=fs::file_size(filenames[i]);
        if (sdbf_sys.verbose) 
            cerr << "sdhash: digesting file " << filenames[i] << endl;
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
                    if (addto!=NULL) {
                        class sdbf *sdbfm = new sdbf(fname,is,dd_block_size,csize,info);
                        addto->add(sdbfm);
                    } else {
                        class sdbf *sdbfm = new sdbf(fname,is,dd_block_size,csize,info);
                        std::cout <<sdbfm;
                        delete sdbfm;
                    }
                } catch (int e) {
                    if (e==-2)
                       exit(-2);
                }
            }
        } else {
            try {
                if (addto!=NULL) {
                class sdbf *sdbfm = new sdbf(filenames[i],is,dd_block_size,filesize,info);
                    addto->add(sdbfm);
                } else {
                class sdbf *sdbfm = new sdbf(filenames[i],is,dd_block_size,filesize,info);
                    std::cout <<sdbfm;
                    delete sdbfm;
                }
            } catch (int e) {
                if (e==-2)
                   exit(-2);
            }
        }
        is->close();
    }
    delete is;
}

int32_t 
hash_index_stringlist(const std::vector<std::string> & filenames, string output_name) {
    std::vector<string> small;
    std::vector<string> large;
    sdbf_set *set1;
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
    int hashfilecount=0;
    int i;
    index_info *info=(index_info*)malloc(sizeof(index_info));
    info->setlist=NULL;
    info->indexlist=NULL;
    info->search_deep=false;
    info->search_first=false;
    if (smallct > 0) {
        if (sdbf_sys.verbose)
            cerr << "sdhash: hashing+indexing small files"<< endl;
        char **smalllist=(char **)alloc_check(ALLOC_ONLY,smallct*sizeof(char*),"main", "filename list", ERROR_EXIT);
        int filect=0;
        int sizetotal=0;
        for (i=0; i < smallct ; filect++,i++) {
            smalllist[filect]=(char*)alloc_check(ALLOC_ONLY,small[i].length()+1, "main", "filename", ERROR_EXIT);
            strncpy(smalllist[filect],small[i].c_str(),small[i].length()+1);
            sizetotal+=fs::file_size(small[i]);
            //if (sizetotal > 16*MB || i==smallct-1) {
            if (sizetotal > 640*MB || i==smallct-1) {
               //if (sdbf_sys.verbose)
                    //cerr << "hash "<<hashfilecount<< " numf "<<filect<< endl;
               // set up new index, set, hash them..
               //bloom_filter *index1=new bloom_filter(4*MB,5,0,0.01);
               bloom_filter *index1=new bloom_filter(64*MB,5,0,0.01);
               info->index=index1;
               set1=new sdbf_set(index1);
               sdbf_hash_files( smalllist, filect+1, sdbf_sys.thread_cnt,set1, info);
               string output_nm= output_name+boost::lexical_cast<string>(hashfilecount)+".sdbf";
               sizetotal=filect=0;
               hashfilecount++;
               // print set1 && index to files with counter
               std::filebuf fb; 
               fb.open (output_nm.c_str(),ios::out|ios::binary);
               if (fb.is_open()) {
                   std::ostream os(&fb);
                   os << set1;
                   fb.close();
               } else {
                   cerr << "sdhash: ERROR cannot write to file " << output_name<< endl;
                   return -1;
               }
               set1->index->set_name(output_nm);
               string output_index = output_nm + ".idx";
               int output_result = set1->index->write_out(output_index);
               if (output_result == -2) {
                   cerr << "sdhash: ERROR cannot write to file " << output_index<< endl;
                   return -1;
               }
               delete set1;
               delete index1;
            }
        }
    }
    if (largect > 0) {
        int filect=0;
        int sizetotal=0;
        if (sdbf_sys.verbose)
            cerr << "sdhash: hashing+indexing large files"<< endl;
        char **largelist=(char **)alloc_check(ALLOC_ONLY,largect*sizeof(char*),"main", "filename list", ERROR_EXIT);
        for (i=0; i < largect ; filect++, i++) {
            largelist[filect]=(char*)alloc_check(ALLOC_ONLY,large[i].length()+1, "main", "filename", ERROR_EXIT);
            strncpy(largelist[filect],large[i].c_str(),large[i].length()+1);
            sizetotal+=fs::file_size(large[i]);
            //if (sizetotal > 16*MB || i==smallct-1) {
            if (sizetotal > 640*MB || i==largect-1) {
               //if (sdbf_sys.verbose)
                    //cerr << "hash "<<hashfilecount<< " numf "<<filect<< endl;
               // set up new index, set, hash them..
               //bloom_filter *index1=new bloom_filter(4*MB,5,0,0.01);
               bloom_filter *index1=new bloom_filter(64*MB,5,0,0.01);
               info->index=index1;
               set1=new sdbf_set(index1);
               // no options allowed.  this is for auto-mode.
               sdbf_hash_files_dd( largelist, filect+1, 16*KB, 128*MB,set1, info);
               string output_nm= output_name+boost::lexical_cast<string>(hashfilecount)+".sdbf";
               sizetotal=filect=0;
               hashfilecount++;
               // print set1 && index to files with counter
               std::filebuf fb; 
               fb.open (output_nm.c_str(),ios::out|ios::binary);
               if (fb.is_open()) {
                   std::ostream os(&fb);
                   os << set1;
                   fb.close();
               } else {
                   cerr << "sdhash: ERROR cannot write to file " << output_name<< endl;
                   return -1;
               }
               set1->index->set_name(output_nm);
               string output_index = output_nm + ".idx";
               int output_result = set1->index->write_out(output_index);
               if (output_result == -2) {
                   cerr << "sdhash: ERROR cannot write to file " << output_index<< endl;
                   return -1;
               }
               delete set1;
               delete index1;
            }
        }
    }
    return 0;
}
