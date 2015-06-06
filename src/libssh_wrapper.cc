/******************************************************************************
 *  Copyright (c) 2015 Jamis Hoo
 *  Distributed under the MIT license 
 *  (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *  
 *  Project: Group-Share Filesystem
 *  Filename: libssh_wrapper.cc 
 *  Version: 1.0
 *  Author: Jamis Hoo
 *  E-mail: hoojamis@gmail.com
 *  Date: Jun  6, 2015
 *  Time: 16:45:39
 *  Description: wapper of libssh functions
 *****************************************************************************/

#include "libssh_wrapper.h"
#include <sys/stat.h>
#include <fcntl.h>


intmax_t SSHSession::read(const std::string& path, const size_t offset, const size_t size, char* buff) {
    // if connection already exists
    if (_file_open == path && _sftp_session) {
        // read
        intmax_t bytes_read = do_read(offset, size, buff);
        // read success
        if (bytes_read >= 0) return bytes_read;
        // read failed
        disconnect();
    }

    // reconnect failed
    if (connect(path)) return -1;

    // read
    intmax_t bytes_read = do_read(offset, size, buff);

    // read success
    if (bytes_read >= 0) return bytes_read;
    
    std::cerr << "SFTP read file failed. " << std::endl;

    // read failed
    disconnect();

    return bytes_read;
}


bool SSHSession::do_connect(const std::string& path) {
    // create ssh session
    _ssh_session = ssh_new();

    if (_ssh_session == nullptr) {
        std::cerr << "Create SSH session failed. " << std::endl;
        return 1;
    } 

    // set host and port
    if (ssh_options_set(_ssh_session, SSH_OPTIONS_HOST, _address.c_str()) ||
        ssh_options_set(_ssh_session, SSH_OPTIONS_PORT, &_port)) {
        std::cerr << "Set SSH host and port failed. " << std::endl;
        return 1;
    }

    // ssh connect
    if (ssh_connect(_ssh_session) != SSH_OK) {
        std::cerr << "SSH connection failed. " << std::endl;
        return 1;
    }

    // authentication
    if (ssh_userauth_publickey_auto(_ssh_session, 0, 0) != SSH_AUTH_SUCCESS) {
        std::cerr << "Authentication failed. " << std::endl;
        return 1;
    }

    // create sftp session
    _sftp_session = sftp_new(_ssh_session);

    if (_sftp_session == nullptr) {
        std::cerr << "Create SFTP session failed. " << std::endl;
        return 1;
    }

    // sftp init
    if (sftp_init(_sftp_session) != SSH_OK) {
        std::cerr << "SFTP init failed. " << std::endl;
        return 1;
    }

    // open file
    _file_handle = sftp_open(_sftp_session, path.c_str(), O_RDONLY, 0);
    if (_file_handle == nullptr) {
        std::cerr << "SFTP open file failed. " << std::endl;
        return 1;
    }
    _file_open = path;

    return 0;
}

