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
#include <string>
#include <iostream>



class SSHSession {
public:
    SSHSession(const std::string& host, const uint16_t port):
        _address(host), _port(port) { }

    ~SSHSession() {
        disconnect();
    }

    // just keep it connected after reading.
    // Because many programs read a lot of times and each time a small chunk of data
    // SSH server will automatically close the connection if not hear anything 
    // for a while, and the connection will be recreated at next read.
    intmax_t read(const std::string& path, const size_t offset, const size_t size, char* buff) {
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

    bool do_connect(const std::string& path) {
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
