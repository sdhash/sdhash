
/**
 * sdhash-mr: Command-line interface for file compare MR testing
 * authors: Candice Quates
 */

#include "../sdbf/bloom_filter.h"
#include "../sdbf/bloom_vector.h"

#include "boost/filesystem.hpp"
#include "boost/program_options.hpp"
#include "boost/lexical_cast.hpp"

#include <fstream>
#include <iomanip>
//#include <omp.h>
#include <vector>
#include <string>

#define SEP "|"

namespace fs = boost::filesystem;
namespace po = boost::program_options;

int 
results_to_file(string results, string resfilename) {
    std::filebuf fb;
    fb.open (resfilename.c_str(),ios::out|ios::binary);
    if (fb.is_open()) {
        std::ostream os(&fb);
        os << results;
        fb.close();
    } else {
        cerr << "sdhash: ERROR cannot write to file " << resfilename << endl;
        return -1;
    }
    return 0;
}

/** sdhash program main
*/
int main( int argc, char **argv) {
    time_t hash_start;
    time_t hash_end; 
    string config_file;
    string listingfile;
    string input_name;
    string output_name;
    string segment_size;
    string idx_size;
    string idx_dir; // where to find indexes
    string logfile;
    string separator;
    int32_t level=0;
    double scale=0.3;
    int32_t threshold=1;
    vector<string> inputlist;
    po::variables_map vm;
    po::options_description config("Configuration");
    try {
        // Declare a group of options that will be 
        // allowed both on command line and in
        // config file
        config.add_options()
                //#("compare,c","compare SDBF-MRs in file, or two SDBF files to each other")
                ("level",po::value<int32_t>(&level)->default_value(0), "resolution level 0, 1, 2 (#folds)")
                ("scale",po::value<double>(&scale)->default_value((double)0.300), "score scale default 0.3 (sdbf)")
                ("threshold,t",po::value<int32_t>(&threshold)->default_value(1),"only show results >=threshold")
                //("output,o",po::value<std::string>(&output_name),"send output to files")
                ("help,h","produce help message")
            ;

        // options that do not need to be in help message
        po::options_description hidden("Hidden");
        hidden.add_options()
            ("input-files", po::value<vector<std::string> >(&inputlist), "input file") 
            ;

        po::options_description cmdline_options;
        cmdline_options.add(config).add(hidden);

        po::options_description config_file_options;
        config_file_options.add(config);

        // setup for list of files on command line
        po::positional_options_description p;
        p.add("input-files", -1);
        
        store(po::command_line_parser(argc, argv).
              options(cmdline_options).positional(p).run(), vm);
        notify(vm);
        
        ifstream ifs(config_file.c_str());
        if (ifs)
        {
            store(parse_config_file(ifs, config_file_options), vm);
            notify(vm);
        }
        if (vm.count("help")) {
            //cout << VERSION_INFO << endl;
            cout << endl <<  "Usage: sdhash-mr <options> <files>"<< endl;
            cout << config << endl;
            return 0;
        }

    }
    catch(exception& e)
    {
        cout << e.what() << "\n";
        return 0;
    }    
    // Perform all-pairs comparison
    std::vector<bloom_vector*> set1;
    //std::vector<bloom_filter *> set2;
        if (inputlist.size()==1) {
            // load first set
            int sizes=0;
            try {
                fstream fs(inputlist[0].c_str(),ios::in|ios::binary);
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
                    vect.ParseFromString(buf);
                    free(buf);
                    bloom_vector *tmp=new bloom_vector(&vect);
                    for (int i=0;i<vect.filter_count();i++) {
                        int fsize;
                        fs.read((char*)&fsize,sizeof(fsize));
                        char *filtbuf=(char*)malloc(fsize*sizeof(char));
                        fs.read(filtbuf,fsize);
                        if (!fs) 
                            break; // eof
                        // trickery with strings is necessary
                        string readstr;
                        readstr.assign(filtbuf,fsize);
                        filter.ParseFromString(readstr);
                        tmp->add_filter(&filter,level);
                        free(filtbuf);
                        filter.Clear();
                    }
                    set1.push_back(tmp);
                    vect.Clear();
                //    sizes=tmp->items->at(0)->bf_size;
                }
                fs.close();
                //return 0;
            } catch (int e) {
                cerr << "sdhash: ERROR: Could not load SDBF-MR file "<< inputlist[0] << ". Exiting"<< endl;
                return -1;
            }
cout << "folding level " << level << " size " << sizes << " scale " << scale << endl;
            int end=set1.size();
            for (int i = 0; i < end ; i++) {
                for (int j = i; j < end ; j++) {
                    if (i == j) continue;
                    int32_t score = set1.at(i)->compare(set1.at(j),scale);
                    if (score >= threshold)  {
                       cout << set1.at(i)->items->at(0)->name() << SEP << set1.at(j)->items->at(0)->name() ;
                       if (score != -1)
                           cout << SEP << setw (3) << score << std::endl;
                       else
                           cout << SEP <<  score << std::endl;
                    }
                }
             }

        } 
    return 0;
}
