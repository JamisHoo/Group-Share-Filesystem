/******************************************************************************
 *  Copyright (c) 2015 Jamis Hoo
 *  Distributed under the MIT license 
 *  (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *  
 *  Project: Group-Share Filesystem
 *  Filename: option_parser.h 
 *  Version: 1.0
 *  Author: Jamis Hoo
 *  E-mail: hoojamis@gmail.com
 *  Date: Jun  4, 2015
 *  Time: 23:08:38
 *  Description: parse command-line options
 *****************************************************************************/
#ifndef OPTION_PARSER_H_
#define OPTION_PARSER_H_


#include <boost/program_options.hpp>

struct OptionParser {

    OptionParser() { initialize(); }

    void initialize();
    void parse(const int, char**);


    bool is_master;
    bool is_standby;
    // IP address
    // all nodes should be able to access each other
    std::string address;
    // master listens at this port
    // other nodes connect to this port
    uint16_t tcp_port;
    // port for ssh, default is 22
    uint16_t ssh_port;
    std::string mount_point;
    std::string working_dir;

private:
    boost::program_options::variables_map vm;
    boost::program_options::options_description all_options;  
};

#endif /* OPTION_PARSER_H_ */
