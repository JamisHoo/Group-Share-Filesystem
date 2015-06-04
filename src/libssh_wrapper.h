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
    SSHSession(const std::string& host, const uint16_t port):
        _address(host), _port(port) { }

    intmax_t read(const std::string& path, const size_t offset, const size_t size, char* buff) {
        ssh_session ssh_session;
        sftp_session sftp_session;
        sftp_file file_handle;
        
        bool rtv = 
                   // alloc ssh session
                   bool(ssh_session = ssh_new()) &&
                   // set host
                   ssh_options_set(ssh_session, SSH_OPTIONS_HOST, _address.c_str()) == 0 &&
                   // set port
                   ssh_options_set(ssh_session, SSH_OPTIONS_PORT, &_port) == 0 &&
                   // connect ssh
                   ssh_connect(ssh_session) == SSH_OK && 
                   // ssh authentication
                   ssh_userauth_publickey_auto(ssh_session, 0, 0) == SSH_AUTH_SUCCESS &&
                   // create sftp session
                   bool(sftp_session = sftp_new(ssh_session)) &&
                   // sftp init
                   sftp_init(sftp_session) == SSH_OK &&
                   // open file
                   bool(file_handle = sftp_open(sftp_session, path.c_str(), O_RDONLY, 0)) &&
                   // seek
                   sftp_seek64(file_handle, offset) == 0;
        
        intmax_t bytes_read = -1;

        if (rtv) 
            bytes_read = sftp_read(file_handle, buff, size);

        if (file_handle) sftp_close(file_handle);
        file_handle = nullptr;
        if (sftp_session) sftp_free(sftp_session);
        sftp_session = nullptr;
        ssh_disconnect(ssh_session);
        ssh_free(ssh_session);

        return bytes_read;
    }

private:
    std::string _address;
    unsigned int _port;

};

#endif /* LIBSSH_WRAPPER_H_ */
