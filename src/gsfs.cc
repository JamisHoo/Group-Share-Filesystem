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

    
    /*
    std::cout << "is_master: " << parser.is_master << "\n"
              << "is_standby: " << parser.is_standby << "\n"
              << "address: " << parser.address << "\n"
              << "tcp port: " << parser.tcp_port << "\n"
              << "ssh port: " << parser.ssh_port << "\n"
              << "mount point: " << parser.mount_point << "\n"
              << "tmp dir: " << parser.tmp_dir << "\n";
    */
    

    if (!parser.is_master && !parser.is_standby) return 0;

    // init user fs
    UserFS fs;
    if (parser.is_master) fs.setMaster();

    // init dir tree
    try {
        fs.initDirTree(parser.mount_point, parser.tmp_dir);
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
    std::unique_ptr<char[]> argv1(new char[parser.mount_point.length() + 1]);
    parser.mount_point.copy(argv1.get(), parser.mount_point.length());
    argv1.get()[parser.mount_point.length()] = 0;

    char* fuse_args[2] = { argv[0], argv1.get() };

    return fuse_main(2, fuse_args, fuse.get(), 0);
}
