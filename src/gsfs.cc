/******************************************************************************
 *  Copyright (c) 2015 Jamis Hoo
 *  Distributed under the MIT license 
 *  (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *  
 *  Project: Group-Share Filesystem
 *  Filename: gsfs.cc 
 *  Version: 1.0
 *  Author: Jamis Hoo
 *  E-mail: hoojamis@gmail.com
 *  Date: Jun  4, 2015
 *  Time: 23:07:43
 *  Description: main function
 *****************************************************************************/

#include <iostream>
#include "option_parser.h"
#include "user_fs.h"
#include "fuse_interface.h"

int main(int argc, char** argv) {


    // parser options
    OptionParser parser;
    try {
        parser.parse(argc, argv);
    } catch (std::exception& err) {
        std::cerr << err.what() << std::endl;
        return 1;
    }

    if (!parser.is_master && !parser.is_standby) return 0;

    // fork a new process to run as a daemon
    if (fork()) return 0;

    // init user fs
    UserFS fs;
    if (parser.is_master) fs.setMaster();

    // init dir tree
    try {
        fs.initDirTree(parser.working_dir);
    } catch (std::exception& err) {
        std::cerr << err.what() << std::endl;
        return 1;
    }

    // init host
    fs.initHost(parser.address, parser.tcp_port, parser.ssh_port);

    // init tcp network
    if (fs.initTCPNetwork(parser.address, parser.tcp_port)) {
        std::cerr << "Error when initializing TCP network. " << std::endl;
        return 1;
    }

    // init fuse interface
    FUSEInterface fuse;
    fuse.initialize(&fs);

    // copy arguments
    std::string arg_tmp = argv[0];
    std::vector<char> arg0(arg_tmp.begin(), arg_tmp.end());
    arg0.push_back(0);

    arg_tmp = parser.mount_point;
    std::vector<char> arg1(arg_tmp.begin(), arg_tmp.end());
    arg1.push_back(0);

    // We must run fuse foreground.
    // If not, fuse will fork() and the other threads in this process won't work.
    arg_tmp = "-f";
    std::vector<char> arg2(arg_tmp.begin(), arg_tmp.end());
    arg2.push_back(0);

    char* fuse_args[3] = { arg0.data(), arg1.data(), arg2.data() };

    return fuse_main(3, fuse_args, fuse.get(), 0);
}
