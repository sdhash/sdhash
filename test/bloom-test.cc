
/**
 * Bloom-test:  Test harness for large bloom filters
 * accepts list of sha1
 * authors: Candice Quates
 */

#include "../sdbf/bloom_filter.h"
#include "../sdbf/bloom_vector.h"

#include "boost/filesystem.hpp"
#include "boost/program_options.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/math/special_functions/round.hpp"

#include "modp_b16.h"

#include <fstream>
#include <iomanip>
#include <vector>
#include <string>

#define SEP "|"

namespace fs = boost::filesystem;
namespace po = boost::program_options;

/** sdhash program main
*/
int main( int argc, char **argv) {
    int32_t f_size=32768;
    int32_t overlap=0;
    int32_t mn=16;
    vector<string> inputlist;
    po::variables_map vm;
    string config_file;
    po::options_description config("Configuration");
    try {
        // Declare a group of options that will be 
        // allowed both on command line and in
        // config file
        config.add_options()
                ("filter_size",po::value<int32_t>(&f_size)->default_value(32768),"size of filters")
                ("overlap",po::value<int32_t>(&overlap)->default_value(0),"level of overlap")
                ("mn",po::value<int32_t>(&mn)->default_value(16)," m/n ")
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
            cout << endl <<  "Usage: bloom-test <options> <file of sha1s>"<< endl;
            cout << config << endl;
            return 0;
        }

    }
    catch(exception& e)
    {
        cout << e.what() << "\n";
        return 0;
    }    

cout << "m/n= " << mn << " m = " <<f_size *8<< " n= " << (f_size*8)/mn << " " ;
    int elemct=boost::math::round((float)(f_size*8)/mn);
    //return 0;
    std::vector<string> hashes;
    if (inputlist.size()==1) {
        // load first set
        fstream fs(inputlist[0].c_str(),ios::in|ios::binary);
        if (!fs) {
            cerr << "File Fails to Open "<< endl;
            return -1;
        } 
        while (!fs.eof()) {
            string sha1_current;
            std::getline(fs,sha1_current);
            hashes.push_back(sha1_current);
        }
        fs.close();
        if (hashes.size() < elemct*2) {
             cerr << "Not enough source hashes, need "<< elemct*2<<endl;
             return 0;
        }
        int over=0;
        if (overlap > 0) 
            over=boost::math::round((float)elemct*(0.01*overlap));
cout << overlap ;
        bloom_filter *f1=new bloom_filter(f_size,5,0,0.01);
        bloom_filter *f2=new bloom_filter(f_size,5,0,0.01);
        
        char *data1=(char*)malloc(160);
        for (int i=0; i<elemct; i++) {
            int check=modp_b16_decode(data1,hashes[i].c_str(),hashes[i].length());
            // not checking for presence
            f1->insert_sha1((uint32_t*)data1);
        }
        for (int i=elemct-over; i<(elemct*2)-over; i++) {
            int check=modp_b16_decode(data1,hashes[i].c_str(),hashes[i].length());
            // not checking for presence
            f2->insert_sha1((uint32_t*)data1);
        }
        f1->compute_hamming();
        f2->compute_hamming();
        int score=f1->compare(f2,0.3);
        cout << score<< endl;
        delete f1;
        delete f2;
    } else {
        cout << endl <<  "Usage: bloom-test <options> <file of sha1s>"<< endl;
        cout << config << endl;
        return 0;
    }
    return 0;
}
