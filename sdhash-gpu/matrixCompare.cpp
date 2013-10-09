#include "matrixCompare.h"

// includes, system
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
//
#include <sdbf_class.h>
#include <sdbf_set.h>
#include <bloom_filter.h>

#include "boost/program_options.hpp"
#include "boost/filesystem.hpp"
namespace po = boost::program_options;
namespace fs = boost::filesystem;


//helper function.
void constantInit(uint64_t *data, int size, uint64_t val)
{
    for (int i = 0; i < size; ++i)
    {
        data[i] = val;
    }
}

void constantInit(uint16_t *data, int size, uint64_t val)
{
    for (int i = 0; i < size; ++i)
    {
        data[i] = val;
    }
}

void emptySet(sdbf_set *to_empty) {
	for (int n=0;n< to_empty->size(); n++) 
        delete to_empty->at(n);
}

// doesn't compile  in the header please don't ask me why.
int sdbfsetCompare(sdbf_set *refset, sdbf_set *target, bool quiet, int confidence);

/**
 * launcher. 
 */
int main(int argc, char **argv)
{
    string refset;
    string targetset;
    int deviceNumber=0;
	int confidence=0;
    po::variables_map vm;
    po::options_description config("Configuration");
	bool quiet=true;
    try {
        config.add_options()
            ("device,d",po::value<int>(&deviceNumber)->default_value(0),"CUDA device to use (0,1,2 etc)")
            ("reference-set,r",po::value<std::string>(&refset),"File or directory of fixed-size reference set(s) (data size 640MB ideal, 32MB minimum)")
            ("target-set,t",po::value<std::string>(&targetset),"File or directory of variable-sized search target set(s)")
			
            ("confidence,c",po::value<int>(&confidence)->default_value(1),"confidence level of results")
            ("verbose","debugging and progress output")
            ("version","show version info")
            ("help,h","produce help message")
        ;
        store(po::parse_command_line(argc, argv,config), vm);
        notify(vm);
        if (vm.count("help")) {
            cout << "sdhash-gpu 3.4 BETA by Candice Quates, September 2013" << endl;
            cout << "Usage: sdhash-gpu -d dev -r ref.sdbf -t targ.sdbf" << endl;
			cout << "  Each reference set is processed as a whole entity."<<endl;
			cout << "  Target set processing compares each object to the current reference set." << endl; 
            cout << config << endl;
            return 0;
        }
        if (vm.count("version")) {
            cout << "sdhash-gpu 3.4 BETA by Candice Quates, September 2013"<< endl; 
            return 0;
        }
        if (!vm.count("reference-set") || !(vm.count("target-set"))) {
            cout << "sdhash-gpu 3.4 BETA by Candice Quates, September 2013"<< endl;
            cout << "Usage: sdhash-gpu -d dev -r ref.sdbf -t targ.sdbf" << endl;
			cout << "  Each reference set is processed as a whole entity."<<endl;
			cout << "  Target set processing compares each object to the current reference set." << endl; 
            cout << config << endl;
            return 0;
        }
		if (vm.count("verbose")) {
			quiet=false;
		} else {
			quiet=true;
		}
    } catch(exception& e) {
        cout << e.what() << endl;
        return 0;
    }
	int notready=deviceSetup(deviceNumber);
    if (notready) 
        exit(1);
	cerr<<"[sdhash-gpu 3.4 BETA comparison using CUDA] - Starting...\n" << endl;
	if (fs::is_regular_file(refset) && fs::is_regular_file(targetset)) { 
		sdbf_set *set1=new sdbf_set(refset.c_str());
		sdbf_set *set2=new sdbf_set(targetset.c_str());		
		if (set1->filter_count() > MAX_REF_SIZE) {
			cerr << "Reference set " << set1->name() << " too large, skipping. " << endl;
		}	else if (set1->filter_count() < MIN_REF_SIZE) {					
			cerr << "Reference set " << set1->name() << " too small, min bf ct "<<MIN_REF_SIZE << ", skipping. " << endl;
		} else {
			if (!quiet)
				std::cout<< set1->name() << " " << set2->name() << std::endl;
			sdbfsetCompare(set1,set2,quiet,confidence);
		}
		delete set1;
		delete set2;
	} else if (fs::is_directory(refset) && fs::is_regular_file(targetset)) {
		sdbf_set *set2=new sdbf_set(targetset.c_str());
        for (fs::directory_iterator itr(refset.c_str()); itr!=fs::directory_iterator(); ++itr) {
            if (fs::is_regular_file(itr->status()) && (itr->path().extension().string() == ".sdbf")) {
                sdbf_set *ref=new sdbf_set(itr->path().string().c_str());
				if (ref->filter_count() > MAX_REF_SIZE) {
					cerr << "Reference set " << ref->name() << " too large, skipping. " << endl;
					continue;
				}	
				if (ref->filter_count() < MIN_REF_SIZE) {					
					cerr << "Reference set " << ref->name() << " too small, min bf ct"<<MIN_REF_SIZE << ", skipping. " << endl;
					continue;
				}
                if (!quiet) {
					cout << "compare "<< ref->name() << " to " << set2->name() << endl;
				}
				sdbfsetCompare(ref,set2,quiet,confidence);
				emptySet(ref);
				delete ref;
            }
        }
		delete set2;
	} else if (fs::is_regular_file(refset) && fs::is_directory(targetset)) {
		sdbf_set *set1=new sdbf_set(refset.c_str());
		if (set1->filter_count() > MAX_REF_SIZE) {
			cerr << "Reference set " << set1->name() << " too large, skipping. " << endl;
		} else	if (set1->filter_count() < MIN_REF_SIZE) {					
			cerr << "Reference set " << set1->name() << " too small, min bf ct"<<MIN_REF_SIZE << ", skipping. " << endl;
		}
		else {
			for (fs::directory_iterator itr(targetset.c_str()); itr!=fs::directory_iterator(); ++itr) {
				if (fs::is_regular_file(itr->status()) && (itr->path().extension().string() == ".sdbf")) {
					sdbf_set *targ=new sdbf_set(itr->path().string().c_str());
					if (!quiet) {
						cout << "compare "<< set1->name() << " to " << targ->name() << endl;
					}
					sdbfsetCompare(set1,targ,quiet,confidence);
					emptySet(targ);
					delete targ;
				}
			}
		}
		delete set1;
	} else { // assume two directories
		for (fs::directory_iterator itr(refset.c_str()); itr!=fs::directory_iterator(); ++itr) {
            if (fs::is_regular_file(itr->status()) && (itr->path().extension().string() == ".sdbf")) {
                sdbf_set *ref=new sdbf_set(itr->path().string().c_str());
				if (ref->filter_count() > MAX_REF_SIZE) {
					cerr << "Reference set " << ref->name() << " too large, max "<< MAX_REF_SIZE << "skipping. " << endl;
					continue;
				}
				if (ref->filter_count() < MIN_REF_SIZE) {					
					cerr << "Reference set " << ref->name() << " too small, min bf ct"<<MIN_REF_SIZE << ", skipping. " << endl;
					continue;
				}
				for (fs::directory_iterator itr(targetset.c_str()); itr!=fs::directory_iterator(); ++itr) {
					if (fs::is_regular_file(itr->status()) && (itr->path().extension().string() == ".sdbf")) {
						sdbf_set *targ=new sdbf_set(itr->path().string().c_str());
						if (!quiet) {
							cout << "compare "<< ref->name() << " to " << targ->name() << endl;
						}
						sdbfsetCompare(ref,targ,quiet,confidence);
						emptySet(targ);
						delete targ;
					}	
				}
				emptySet(ref);
				delete ref;
			}
		}
	}
    deviceTeardown();
    return(0);
}
