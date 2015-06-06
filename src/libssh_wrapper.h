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

#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <string>
#include <iostream>



class SSHSession {
public:
    SSHSession(const std::string& host, const uint16_t port):
        _address(host), _port(port) { }

    ~SSHSession() {
        disconnect();
    }

    // 1. just keep it connected after reading.
    //    Because many programs read a lot of times and each time a small chunk of data
    //    SSH server will automatically close the connection if not hear anything 
    //    for a while, and the connection will be recreated at next read.
    // 2. This implementation of read() is extremely slow when read big file.
    //    Use prefetch if u wanna speed up.
    intmax_t read(const std::string& path, const size_t offset, const size_t size, char* buff);

    intmax_t do_read(const size_t offset, const size_t size, char* buff) {
        if (sftp_seek64(_file_handle, offset) != 0) return -1;
        return sftp_read(_file_handle, buff, size);
    }

    bool connect(const std::string& path) {
        if (do_connect(path)) {
            disconnect();
            return 1;
        }
        return 0;
    }

    bool do_connect(const std::string& path);

    void disconnect() {
        _file_open.clear();
        if (_file_handle) sftp_close(_file_handle);
        _file_handle = nullptr;
        if (_sftp_session) sftp_free(_sftp_session);
        _sftp_session = nullptr;
        ssh_disconnect(_ssh_session);
        ssh_free(_ssh_session);
        _ssh_session = nullptr;
    }

private:
    std::string _address;
    unsigned int _port;

    ssh_session _ssh_session;
    sftp_session _sftp_session;
    sftp_file _file_handle;
    std::string _file_open;
};

#endif /* LIBSSH_WRAPPER_H_ */
