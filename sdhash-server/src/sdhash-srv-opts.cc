/** sdhash-srv-opts.cc 
 * author: Candice Quates
 * reading server defaults. 
 * 
 * small remaining portions of example code from boost-provided examples.
*/



#include <boost/program_options.hpp>
namespace po = boost::program_options;


#include <iostream>
#include <fstream>
#include <iterator>

#include <omp.h>

#include "sdhash-srv.h"
using namespace std;

// A helper function to simplify the main part.
template<class T>
ostream& operator<<(ostream& os, const vector<T>& v)
{
    copy(v.begin(), v.end(), ostream_iterator<T>(cout, " ")); 
    return os;
}

sdbf_parameters_t*
read_args (int ac, char* av[], int quiet)
{
    sdbf_parameters_t *options=(sdbf_parameters_t*)malloc(sizeof(sdbf_parameters_t));

    try {
        string config_file;
    
        // Declare a group of options that will be 
        // allowed only on command line
        po::options_description generic("Generic options");
        generic.add_options()
            ("version,v", "print version string")
            ("help,h", "produce help message")
            ("config,C", po::value<string>(&config_file)->default_value("sdhash-srv.cfg"),
                  "name of config file")
            ;
    
        // Declare a group of options that will be 
        // allowed both on command line and in
        // config file
        po::options_description config("Configuration");
        config.add_options()
            ("port,P",po::value<uint32_t>()->default_value(9090),"server port")
            //("host,H",po::value<string>()->default_value("localhost"),"server local address")
            ("threads,t",po::value<uint32_t>(),"set compute threads to use")
            ("connections,c",po::value<uint32_t>()->default_value(20),"server connections provided")
            ("hashdir,d",po::value<string>()->default_value("."),"server hash directory")
            ("sourcedir,s",po::value<string>()->default_value("."),"server sources directory")
            ;

        po::options_description cmdline_options;
        cmdline_options.add(generic).add(config);

        po::options_description config_file_options;
        config_file_options.add(config);

        po::options_description visible("Options");
        visible.add(generic).add(config);
        
        po::positional_options_description p;
        p.add("input-file", -1);
        
        po::variables_map vm;
        store(po::command_line_parser(ac, av).
              options(cmdline_options).positional(p).run(), vm);
        notify(vm);
        
        ifstream ifs(config_file.c_str());
        if (!ifs)
        {
            cout << "sdhash-srv cannot open config file: " << config_file << endl;
            cout << "sdhash-srv will use defaults." << endl;
            // return 0;
        }
        else
        {
            store(parse_config_file(ifs, config_file_options), vm);
            notify(vm);
        }
    
        if (vm.count("help")) {
            cout << visible << endl;
            return 0;
        }

        if (vm.count("version")) {
            cout << "sdhash-srv 2.1beta" << endl;
            return 0;
        }

        if (vm.count("threads") && !vm.count("auto-threads")) {
            if (!quiet)
                cout << "Processing threads available for use: " << vm["threads"].as< uint32_t >() << endl;
            options->thread_cnt=vm["threads"].as<uint32_t>();
            omp_set_num_threads(options->thread_cnt);
        } else {
            options->thread_cnt= omp_get_max_threads();
            if (!quiet)
                cout << "Processing threads available for use: " << options->thread_cnt << endl;
        }

        if (vm.count("connections")) {
            if (!quiet)
                cout << "Max connections allowed: " << vm["connections"].as< uint32_t >() << endl;
            options->maxconn=vm["connections"].as<uint32_t>();
        }
        if (vm.count("port")) {
            if (!quiet)
                cout << "Server port: " << vm["port"].as< uint32_t >() << endl;
            options->port=vm["port"].as<uint32_t>();
        }
        if (vm.count("hashdir")) {
            if (!quiet)
                cout << "Server home directory: " << vm["hashdir"].as< string >() << endl;
            options->home=new string(vm["hashdir"].as<string>());
        }
        if (vm.count("sourcedir")) {
            if (!quiet)
                cout << "Server sources directory: " << vm["sourcedir"].as< string >() << endl;
            options->sources=new string(vm["sourcedir"].as<string>());
        }

    }
    catch(exception& e)
    {
        cout << e.what() << endl;
        return NULL;
    }    
    return options;
}
