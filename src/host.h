/******************************************************************************
 *  Copyright (c) 2015 Jamis Hoo
 *  Distributed under the MIT license 
 *  (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *  
 *  Project: Group-Share Filesystem
 *  Filename: host.h 
 *  Version: 1.0
 *  Author: Jamis Hoo
 *  E-mail: hoojamis@gmail.com
 *  Date: May 29, 2015
 *  Time: 23:16:35
 *  Description: 
 *****************************************************************************/
#ifndef HOST_H_
#define HOST_H_

class Hosts {
    struct Host {
        size_t id;
        std::string address;
        uint16_t tcp_port;
        uint16_t ssh_port;
        // TODO: authentication information
    };

private:
    std::vector<Host> _hosts;



};


#endif /* HOST_H_ */
