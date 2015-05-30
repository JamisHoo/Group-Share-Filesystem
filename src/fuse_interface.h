/******************************************************************************
 *  Copyright (c) 2015 Jamis Hoo
 *  Distributed under the MIT license 
 *  (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *  
 *  Project: 
 *  Filename: fuse_interface.h 
 *  Version: 1.0
 *  Author: Jamis Hoo
 *  E-mail: hoojamis@gmail.com
 *  Date: May 30, 2015
 *  Time: 17:45:00
 *  Description: 
 *****************************************************************************/
#ifndef FUSE_INTERFACE_H_
#define FUSE_INTERFACE_H_

#include <errno.h>
#include <fcntl.h>

#define FUSE_USE_VERSION 26
#include <osxfuse/fuse.h>
#undef FUSE_USE_VERSION

#include "user_fs.h"


class FUSEInterface {
public:

    static void initialize(UserFS* userfs) {
        _gsfs_oper.getattr = FUSEInterface::getattr;
        _gsfs_oper.readdir = FUSEInterface::readdir;
        _gsfs_oper.open = FUSEInterface::open;
        _gsfs_oper.read = FUSEInterface::read;
        _user_fs = userfs;
    }

    static int getattr(const char* path, struct stat* stbuf) {
        memset(stbuf, 0, sizeof(struct stat));

        // find in dir tree
        const DirTree::TreeNode* node = _user_fs->dir_tree.find(path);
        
        // not found
        if (!node) return -ENOENT;
        
        
        if (node->type == DirTree::TreeNode::REGULAR) {
            stbuf->st_mode = S_IFREG | 0444;
            stbuf->st_nlink = 1;
            stbuf->st_size = node->size;
        } else if (node->type == DirTree::TreeNode::DIRECTORY) {
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
        } else assert(0);

        return 0;
    }

    static int readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi) {

        (void) offset;
        (void) fi;

        if (strcmp(path, "/") != 0)
            return -ENOENT;

        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
        filler(buf, "a", NULL, 0);
        filler(buf, "b", NULL, 0);

        return 0;
    }

    static int open(const char *path, fuse_file_info *fi) {
        if (strcmp(path, "/a") != 0 && strcmp(path, "/b") != 0)
            return -ENOENT;

        if ((fi->flags & 3) != O_RDONLY)
            return -EACCES;

        return 0;
    }

    static int read(const char *path, char *buf, size_t size, off_t offset,
                    fuse_file_info *fi) {
        (void) fi;
        if(strcmp(path, "/a") != 0 && strcmp(path, "/b") != 0)
            return -ENOENT;

        char hello_str[] = " Under implementation\n";

        off_t len = strlen(hello_str);
        off_t ssize = size;
        if (offset < len) {
            if (offset + ssize > len)
                ssize = len - offset;
            memcpy(buf, hello_str + offset, ssize);
        } else
            ssize = 0;

        return ssize;
    }

    static fuse_operations* get() {
        return &_gsfs_oper;
    }

private:
    static UserFS* _user_fs;

    static fuse_operations _gsfs_oper;

};


#endif /* FUSE_INTERFACE_H_ */
