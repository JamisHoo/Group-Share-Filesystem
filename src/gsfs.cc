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
//#include "user_fs.h"
//#include "fuse_interface.h"

int main(int argc, char** argv) {
    OptionParser parser;
    parser.parse(argc, argv);

    std::cout << "is_master: " << parser.is_master << "\n"
              << "is_standby: " << parser.is_standby << "\n"
              << "address: " << parser.address << "\n"
              << "tcp port: " << parser.tcp_port << "\n"
              << "ssh port: " << parser.ssh_port << "\n"
              << "mount point: " << parser.mount_point << "\n"
              << "tmp dir: " << parser.tmp_dir << "\n";


}
