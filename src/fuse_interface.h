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
        const DirTree::TreeNode* node = _user_fs->find(path);
        
        // not found
        if (!node) return -ENOENT;
        
        // TODO: support more file types and hard link
        
        if (node->type == DirTree::TreeNode::REGULAR) {
            stbuf->st_mode = S_IFREG | 0444;
            stbuf->st_nlink = 1;
            stbuf->st_size = node->size;
        } else if (node->type == DirTree::TreeNode::DIRECTORY) {
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
        } else {
            std::cerr << "read path error at " << path << "." << std::endl;
            return -ENOENT;
        }

        return 0;
    }

    static int readdir(const char* path, void* buf, fuse_fill_dir_t filler,
                       off_t /* offset */, struct fuse_file_info* /* fi */) {
        const DirTree::TreeNode* node = _user_fs->find(path);
        if (!node) return -ENOENT;

        if (node->type != DirTree::TreeNode::DIRECTORY) return -ENOTDIR;
        
        for (const auto& i: node->children) 
            filler(buf, i.name.c_str(), 0, 0);

        return 0;
    }

    static int open(const char* path, fuse_file_info* fi) {
        const DirTree::TreeNode* node = _user_fs->find(path);
        if (!node) return -ENOENT;
        
        if ((fi->flags & 3) != O_RDONLY)
            return -EACCES;

        return 0;
    }

    static int read(const char* path, char* buf, size_t size, off_t offset,
                    fuse_file_info* /* fi */) {
        
        const DirTree::TreeNode* node = _user_fs->find(path);
        if (!node) return -ENOENT;

        if (node->type == DirTree::TreeNode::DIRECTORY) return -EISDIR;

        assert(offset >= 0);

        size_t read_offset = offset;
        size_t read_size = size;
        size_t file_size = node->size;

        if (read_offset >= file_size) return 0;

        if (read_offset + read_size > file_size)
            read_size = file_size - read_offset;
        
        // TODO: read file from remote host
        for (size_t i = 0; i < read_size; ++i)
            buf[i] = i;

        return read_size;
    }

    static fuse_operations* get() {
        return &_gsfs_oper;
    }

private:
    static UserFS* _user_fs;

    static fuse_operations _gsfs_oper;

};


#endif /* FUSE_INTERFACE_H_ */
