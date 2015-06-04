/******************************************************************************
 *  Copyright (c) 2015 Jamis Hoo
 *  Distributed under the MIT license 
 *  (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *  
 *  Project: Group-Share Filesystem
 *  Filename: libssh_wrapper.h 
 *  Version: 1.0
 *  Author: Jamis Hoo
 *  E-mail: hoojamis@gmail.com
 *  Date: Jun  4, 2015
 *  Time: 09:53:38
 *  Description: wapper of libssh functions
 *****************************************************************************/
#ifndef LIBSSH_WRAPPER_H_
#define LIBSSH_WRAPPER_H_

#include <string>

class SSHSession {
public:
    SSHSession(const std::string& host, const uint16_t port):
        _address(host), _port(port) { }

    intmax_t read(const std::string& path, const size_t offset, const size_t size, char* buff);

private:
    std::string _address;
    unsigned int _port;

};

#endif /* LIBSSH_WRAPPER_H_ */
