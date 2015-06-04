/******************************************************************************
 *  Copyright (c) 2015 Jamis Hoo
 *  Distributed under the MIT license 
 *  (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *  
 *  Project: Group-Share Filesystem
 *  Filename: fuse_interface.cc 
 *  Version: 1.0
 *  Author: Jamis Hoo
 *  E-mail: hoojamis@gmail.com
 *  Date: Jun  4, 2015
 *  Time: 22:21:23
 *  Description: FUSE interface
 *****************************************************************************/

#include "fuse_interface.h"
#include <errno.h>
#include <fcntl.h>

#define FUSE_USE_VERSION 26
#include <osxfuse/fuse.h>
#undef FUSE_USE_VERSION

#include "user_fs.h"

fuse_operations FUSEInterface::_gsfs_oper;
UserFS* FUSEInterface::_user_fs = nullptr;

int FUSEInterface::getattr(const char* path, struct stat* stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    // find in dir tree
    const DirTree::TreeNode* node = _user_fs->find(path);
    
    // not found
    if (!node) return -ENOENT;
    
    if (node->type == DirTree::TreeNode::REGULAR) {
        stbuf->st_mode = S_IFREG | 0444;
    } else if (node->type == DirTree::TreeNode::DIRECTORY) {
        stbuf->st_mode = S_IFDIR | 0755;
    } else if (node->type == DirTree::TreeNode::SYMLINK) {
        stbuf->st_mode = S_IFLNK | 0444;
    } else if (node->type == DirTree::TreeNode::CHRDEVICE) {
        stbuf->st_mode = S_IFCHR | 0444;
    } else if (node->type == DirTree:: TreeNode::BLKDEVICE) {
        stbuf->st_mode = S_IFBLK | 0444;
    } else if (node->type == DirTree::TreeNode::FIFO) {
        stbuf->st_mode = S_IFIFO | 0444;
    } else if (node->type == DirTree::TreeNode::SOCKET) {
        stbuf->st_mode = S_IFSOCK | 0444;
    } else {
        std::cerr << "read path error at " << path << "." << std::endl;
        return -ENOENT;
    }
    stbuf->st_nlink = node->num_links;
    stbuf->st_size = node->size;
    stbuf->st_mtime = node->mtime;

    return 0;
}

int FUSEInterface::readdir(const char* path, void* buf, fuse_fill_dir_t filler,
                   off_t /* offset */, struct fuse_file_info* /* fi */) {
    const DirTree::TreeNode* node = _user_fs->find(path);
    if (!node) return -ENOENT;

    if (node->type != DirTree::TreeNode::DIRECTORY) return -ENOTDIR;
    
    for (const auto& i: node->children) 
        filler(buf, i.name.c_str(), 0, 0);

    return 0;
}

int FUSEInterface::open(const char* path, fuse_file_info* fi) {
    const DirTree::TreeNode* node = _user_fs->find(path);
    if (!node) return -ENOENT;
    
    if ((fi->flags & 3) != O_RDONLY)
        return -EACCES;

    return 0;
}

int FUSEInterface::read(const char* path, char* buf, size_t size, off_t offset,
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
    
    intmax_t bytes_read = _user_fs->read(node->host_id, path, offset, size, buf);

    if (bytes_read < 0) return -EIO;

    return bytes_read;

}



