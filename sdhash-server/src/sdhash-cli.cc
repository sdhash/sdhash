
/**
 * sdhash-cli: Client for hashing server
 * author: Candice Quates
 */

#include <protocol/TBinaryProtocol.h>
#include <transport/TSocket.h>
#include <transport/TTransportUtils.h>

#include "boost/program_options.hpp"
#include "boost/lexical_cast.hpp"
#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>
#include "../sdhash-src/version.h"

#include "sdhashsrv.h"

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using namespace sdhash;

using namespace boost;

namespace po = boost::program_options;
namespace fs = boost::filesystem;


#include "sdhash-cli.h"

// Global parameters
sdhash_parameters_t sdbf_sys = {
    (char*)HOST,        // hostname
    PORT,        // port#
    0,                // dd block size off initially
    1,               // output_threshold
    FLAG_OFF,        // warnings
    0,              // sample size off
    0,        // default no options set
    0,        // default no options2 set
    -1,        // result ID off
    (char*)"default"        // default username
};

int main( int argc, char **argv) {
    uint32_t  i, j, k, file_cnt;
    int set1;
    char tempstring[]="/tmp/default-XXXXXX";
    boost::filesystem::path current_dir=boost::filesystem::initial_path();
    int hashsetID = -1;
    vector<string> inputlist;
    string input_name;
    string user_name;
    string hostname;
    string config_file;
    string segment_size;
    int32_t resultID;
    po::variables_map vm;
    po::options_description config("Configuration");

    try {
        // Declare a group of options that will be 
        // allowed both on command line and in
        // config file
        config.add_options()
                ("config-file,C", po::value<string>(&config_file)->default_value("sdhash-cli.cfg"), "")
               // ("hash-list,f",po::value<std::string>(&listingfile),"generate SDBFs from list of filenames")
               // ("deep,r", "generate SDBFs from directories and files")
                ("host,H",po::value<std::string>(&hostname)->default_value("localhost"),"server to connect to")
                ("port,P",po::value<uint32_t>(&sdbf_sys.port)->default_value(9090),"server port")
                ("list,l","list all resident sets of SDBFs")
                ("compare,c","compare all pairs in SDBF file, or compare two SDBF files to each other")
                ("import,i","load SDBFs from file")
                ("export,e","export SDBFs to file <N> <file>.")
                ("results,r",po::value<int32_t>(&resultID),"retrieve result by ID")
                ("results-list","retrieve results list")
                ("type",po::value<std::string>(&user_name),"result type, default|indexing|web")
                ("show-set-data,d",po::value<int32_t>(&resultID),"retrieve result by ID")
                ("show-set-names,n",po::value<int32_t>(&resultID),"retrieve result by ID")
                ("threshold,t",po::value<int32_t>(&sdbf_sys.output_threshold)->default_value(1),"only show results >=threshold")
                ("block-size,b",po::value<uint32_t>(&sdbf_sys.dd_block_size),"hashes input files in nKB blocks")
                ("sample-size,s",po::value<uint32_t>(&sdbf_sys.sample_size)->default_value(0),"sample N filters for comparisons")
                ("name",po::value<std::string>(&input_name),"declare set name")
                ("index","generate indexes while hashing")
                ("index-search","match indexes at set level")
                ("verbose","turn on warnings")
                ("version","show version info")
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

        std::ifstream ifs(config_file.c_str());
        if (vm.count("help")) {
            std::cout << VERSION_INFO << ", rev " << REVISION << std::endl;
            std::cout << "Usage: sdhash-cli <options> <source files>|<hash files>"<< std::endl;
            std::cout << config << std::endl;
            return 0;
        }

        if (vm.count("version")) {
            std::cout << VERSION_INFO << ", rev " << REVISION << std::endl;
            std::cout << "       http://sdhash.org, license Apache v2.0" << std::endl;
            return 0;
        }
        if (vm.count("warnings")) {
            sdbf_sys.warnings = 1;
        }
    if (vm.count("host")) {
        sdbf_sys.hostname=(char*)hostname.c_str();
    }
    if (vm.count("import"))
        sdbf_sys.options|=MODE_IMPORT;
    if (vm.count("export"))
        sdbf_sys.options|=MODE_EXPORT;
    if (vm.count("list"))
        sdbf_sys.options|=MODE_LIST;
    if (vm.count("show-set-names"))
        sdbf_sys.options|=MODE_DISP;
    if (vm.count("show-set-data"))
        sdbf_sys.options|=MODE_CONTENT;
    if (vm.count("compare"))
        sdbf_sys.options|=MODE_COMP;
    if (vm.count("results")) {
        sdbf_sys.options|=MODE_RESULT;
        sdbf_sys.resultid=resultID;
    }
    if (vm.count("results-list")) {
        sdbf_sys.options|=MODE_RESULT;
            if (vm.count("type"))
                sdbf_sys.username=(char*)user_name.c_str();
        sdbf_sys.resultid=0;
    }
    if (vm.count("type")) {
        sdbf_sys.username=(char*)user_name.c_str();
    }
    if (vm.count("index-search")) {
        sdbf_sys.options|=MODE_HASH;
        if (sdbf_sys.dd_block_size == 0);
            sdbf_sys.dd_block_size=16;
    }

    } catch (std::exception &e) {
        std::cout << e.what() << "\n";
        return 0;
    }

    int file_start=1;
    boost::shared_ptr<TTransport> socket(new TSocket(sdbf_sys.hostname, sdbf_sys.port));
    boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
    boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
    sdhashsrvClient client(protocol);
    if (sdbf_sys.options == 0 && vm.count("input-files")) {
        sdbf_sys.options|=MODE_HASH;
        if (vm.count("index-search")) {
            input_name = "searching"; 
        } else if (!vm.count("name")) {
            cerr << "SDHASH: hashset must be named with --name " << std::endl;
            return -1;
        }    
    }
    if (sdbf_sys.options == 0) {
        std::cout << VERSION_INFO << ", rev " << REVISION << std::endl;
        std::cout << "Usage: sdhash-cli <options> <source files>|<hash files>"<< std::endl;
        std::cout << config << std::endl;
    return 0;
    }
    try {
        transport->open();
        client.ping();
        int i,fd;
        FILE *workfile;
        std::string result;
        uint32_t resultvalue = -2;
        switch (sdbf_sys.options) {
            case MODE_LIST:
                client.setList(result,false);
                break;
            case MODE_DISP:
                client.displaySet(result,resultID);
                break;
            case MODE_CONTENT:
                client.displayContents(result,resultID);
                break;
            case MODE_COMP:
                if (inputlist.size()==1) {
                    int32_t newid=client.createResultID(sdbf_sys.username);
                    int32_t set1id=boost::lexical_cast<int32_t>(inputlist[0]);
                    client.compareAll(set1id,sdbf_sys.output_threshold,newid);
                    std::cout << "Request name " << sdbf_sys.username <<", id: "<< newid << std::endl;
                } else if (inputlist.size()==2) {
                    int32_t newid=client.createResultID(sdbf_sys.username);
                    int32_t set1id=boost::lexical_cast<int32_t>(inputlist[0]);
                    int32_t set2id=boost::lexical_cast<int32_t>(inputlist[1]);
                    client.compareTwo(set1id,set2id,sdbf_sys.output_threshold,sdbf_sys.sample_size,newid);
                    std::cout << "Request name " << sdbf_sys.username <<", id: "<< newid << std::endl;
                }
                break;
            case MODE_IMPORT:
                if (inputlist.size()==1) {
                            resultvalue=client.loadSet(inputlist[0].c_str(),-1);
                }
                break;
            case MODE_INFILE:
                hashsetID = client.createHashsetID();
                //    client.hashList(argv[file_start],sdbf_sys.dd_block_size,hashsetID,(sdbf_sys.options2 & MODE_INDEX) );
                result="Hashing in progress for "+(string)argv[file_start];
                break;
            case MODE_HASH:
                // change to hash_string to make windows compat
                hashsetID = client.createHashsetID();
                if (vm.count("index")) {
                    client.hashString(input_name,inputlist,sdbf_sys.dd_block_size,hashsetID,-1);
                    result="Hashing in progress for "+input_name+".sdbf";
                }
                else if (vm.count("index-search")) {
                    int resultID= client.createResultID("indexing");
cout << sdbf_sys.dd_block_size;
                    client.hashString(input_name,inputlist,sdbf_sys.dd_block_size,resultID,3);
                    result="Searching in progress, result id: "+boost::lexical_cast<string>(resultID);
                }
                else { // plain 
                    client.hashString(input_name,inputlist,sdbf_sys.dd_block_size,hashsetID,0);
                    result="Hashing in progress for "+input_name+".sdbf";
                }
                break;
            case MODE_EXPORT:
                if (file_cnt == 2) {
                    if (isdigit(argv[file_start][0])) {
                        resultvalue=client.saveSet(atoi(argv[file_start]),argv[file_start+1]);
                        if (resultvalue==0) {
                            result = "Set saved";
                         }    
                    }
                }
                break;
            case MODE_RESULT: 
                if (sdbf_sys.resultid == 0) {
                    client.displayResultsList(result,sdbf_sys.username,false);
                } else {
                    client.displayResult(result,sdbf_sys.resultid);
                }
                break;
        }
        transport->close();
        if (result.length()==0) {
             if (resultvalue==-1) {
                std::cout << "ERROR: Command failed" << std::endl;
            } else if (resultvalue==-2) {
                //std::cout << "No results found" << std::endl;
            } else{
                std::cout << "Set "<< resultvalue << " created" << std::endl;
            }
        } else {
            std::cout << result << std::endl;
        }
    } catch (TException &tx) {
        printf("Couldn't connect: %s\n", tx.what());
    }
    return 0;
}
