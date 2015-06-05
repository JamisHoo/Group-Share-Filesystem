/******************************************************************************
 *  Copyright (c) 2015 Jamis Hoo
 *  Distributed under the MIT license 
 *  (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *  
 *  Project: Group-Share Filesystem
 *  Filename: fuse_interface.h 
 *  Version: 1.0
 *  Author: Jamis Hoo
 *  E-mail: hoojamis@gmail.com
 *  Date: May 30, 2015
 *  Time: 17:45:00
 *  Description: FUSE interface
 *****************************************************************************/
#ifndef FUSE_INTERFACE_H_
#define FUSE_INTERFACE_H_


#define FUSE_USE_VERSION 26
#include <fuse.h>
#undef FUSE_USE_VERSION

class UserFS;

class FUSEInterface {
public:

    static void initialize(UserFS* userfs) {
        _gsfs_oper.getattr = FUSEInterface::getattr;
        _gsfs_oper.readdir = FUSEInterface::readdir;
        _gsfs_oper.open = FUSEInterface::open;
        _gsfs_oper.read = FUSEInterface::read;
        _gsfs_oper.destroy = FUSEInterface::destroy;
        _user_fs = userfs;
    }

    static int getattr(const char* path, struct stat* stbuf);

    static int readdir(const char* path, void* buf, fuse_fill_dir_t filler,
                       off_t /* offset */, struct fuse_file_info* /* fi */);

    static int open(const char* path, fuse_file_info* fi);

    static int read(const char* path, char* buf, size_t size, off_t offset,
                    fuse_file_info* /* fi */);
        

    static void destroy(void*) { _user_fs = nullptr; }

    static fuse_operations* get() { return &_gsfs_oper; }

private:
    static UserFS* _user_fs;

    static fuse_operations _gsfs_oper;

};


#endif /* FUSE_INTERFACE_H_ */
