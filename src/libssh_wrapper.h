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
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>


class SSHSession {
public:
    SSHSession():_ssh_session(nullptr), _sftp_session(nullptr), _file_is_open(0) { }
    
    ~SSHSession() {
        if (_ssh_session) disconnect();
        sftp_free(_sftp_session);
        ssh_free(_ssh_session);
    }

    // returns true on error
    int initialize() {
        _ssh_session = ssh_new();
        if (!_ssh_session) return 1;
        
        _sftp_session = sftp_new(_ssh_session);
        if (!_sftp_session) return 1;

        if (sftp_init(_sftp_session) != SSH_OK) return 1;

        return 0;
    }

    int connect() {
        if (ssh_connect(_ssh_session) == SSH_OK) return 0;
        if (authPublickey()) { disconnect(); return 1; }
        return 1;
    }

    int disconnect() {
        if (close_file()) return 1;
        ssh_disconnect(_ssh_session);
        return 0;
    }

    int setHost(const std::string& host) {
        return ssh_options_set(_ssh_session, SSH_OPTIONS_HOST, host.c_str());
    }
    int setPort(const uint16_t port) {
        unsigned int port_ = port;
        return ssh_options_set(_ssh_session, SSH_OPTIONS_PORT, &port_);
    }

    intmax_t read(const std::string& path, const size_t offset, const size_t size, char* buff) {
        // not connected yet
        if (!ssh_is_connected(_ssh_session) && connect()) return -1;

        // file not open yet
        if (_open_file != path || _file_is_open) {
            _open_file = path;
            if (open_file()) return -1;
        }

        if (sftp_seek64(_file_handle, offset)) return -1;
        return sftp_read(_file_handle, buff, size);
    }

private:
    int authPublickey() {
         if (ssh_userauth_publickey_auto(_ssh_session, NULL, NULL) == SSH_AUTH_SUCCESS) return 0;
         return 1;
    }

    int open_file() {
        if (_file_is_open == 1) return 0;
        _file_handle = sftp_open(_sftp_session, _open_file.c_str(), O_RDONLY, 0);
        if (_file_handle) {
            _file_is_open = 1;
            return 0;
        }
        _file_is_open = 0;
        return 1;
    }

    int close_file() {
        if (_file_is_open == 0) return 0;
        _file_is_open = 0;
        if (sftp_close(_file_handle) == SSH_NO_ERROR) return 0;
        return 1;
    }

    ssh_session _ssh_session;
    sftp_session _sftp_session;
    sftp_file _file_handle;

    bool _file_is_open;
    // path of file that is opened
    std::string _open_file;


};

#endif /* LIBSSH_WRAPPER_H_ */
