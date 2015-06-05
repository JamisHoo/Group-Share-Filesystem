/******************************************************************************
 *  Copyright (c) 2015 Jamis Hoo
 *  Distributed under the MIT license 
 *  (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *  
 *  Project: 
 *  Filename: option_parser.cc 
 *  Version: 1.0
 *  Author: Jamis Hoo
 *  E-mail: hoojamis@gmail.com
 *  Date: Jun  5, 2015
 *  Time: 09:24:42
 *  Description: parse command-line options
 *****************************************************************************/

#include "option_parser.h"
#include <iostream>
#include <stdexcept>
#include <boost/filesystem.hpp>
#include <boost/asio/ip/address.hpp>

void OptionParser::initialize() {
    using namespace std;
    using namespace boost::program_options;

    options_description master_options("Master Node options");
        
    master_options.add_options()
        ("listen,l", value<string>(), "Run as a master node and listen at this IP address. Both IPv4 and IPv6 are supported. ");

    options_description standby_options("Stand By Node options");

    standby_options.add_options()
        ("connect,c", value<string>(), "Run as a stand by node and connect to this IP address. Both IPv4 and IPv6 are supported.");

    options_description generic_options("Generic options");

    generic_options.add_options()
        ("tcp-port,t", value<uint16_t>(), "Specify TCP port. ")
        ("ssh-port,s", value<uint16_t>(), "Specify SSH port. Default value is 22 ")
        ("mount-point,m", value<boost::filesystem::path>(), "Specify filesysem mount point. ")
        ("working-directory,w", value<boost::filesystem::path>(), "Specify temporary directory used when program running. ")
        ("help,h", "Display this help message. ")
        ("version,v", "Display version number of this program. ");
        
    all_options.add(master_options).add(standby_options).add(generic_options);
}

void OptionParser::parse(int argc, char** argv) {
    using namespace std;
    using namespace boost::program_options;

    is_master = is_standby = 0;

    store(parse_command_line(argc, argv, all_options), vm);
    notify(vm);

    // --version
    if (vm.size() == 1 && vm.count("version")) {
        cout << "Group-Share Filesystem (GSFS), Version 1.0 \n"
                "Copyright (c) 2015 Jamis Hoo  \n"
                "Distributed under the MIT license\n"
             << std::flush;
        return;
    }
    
    // --help
    if ((vm.size() == 1 && vm.count("help")) || vm.size() == 0) {
        cout << all_options << std::endl;
        return;
    }

    // check --listen and --connect conflict
    if (vm.count("listen") && vm.count("connect")) 
        throw invalid_argument("Conflict options --listen and --connect. ");
    
    // redundant --help
    if (vm.count("help"))
        throw invalid_argument("Invalid option --help. ");

    // redundata --version
    if (vm.count("version"))
        throw invalid_argument("Invalid option --version. ");

    // either --listen or --connect
    if (vm.count("listen")) 
        is_master = 1, address = vm["listen"].as<string>();
    else if (vm.count("connect")) 
        is_standby = 1, address = vm["connect"].as<string>();
    else 
        throw invalid_argument("Invalid option(s). You must specify an IP address with either --listen or --connect. ");

    boost::system::error_code ec;
    boost::asio::ip::address::from_string(address, ec);
    if (ec) 
        throw invalid_argument("Invalid IP address (" + address + "). ");

    // --tcp-port
    if (vm.count("tcp-port")) 
        tcp_port = vm["tcp-port"].as<uint16_t>();
    else 
        throw invalid_argument("Invalid option(s). Yout must specify a port for TCP connection with --tcp-port. ");

    // --ssh-port
    if (vm.count("ssh-port"))
        ssh_port = vm["ssh-port"].as<uint16_t>();
    else
        ssh_port = 22;

    // --mount-point
    if (vm.count("mount-point"))
        mount_point = vm["mount-point"].as<boost::filesystem::path>().string();
    else
        throw invalid_argument("Invalid option(s). You must specify a mount point for this filesystem. ");

    // --working-directory
    if (vm.count("working-directory"))
        tmp_dir = vm["working-directory"].as<boost::filesystem::path>().string();
    else 
        throw invalid_argument("Invalid option(s). You must specify a temporary working directory for this filesystem. ");
}
