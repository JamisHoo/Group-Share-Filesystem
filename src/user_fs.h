/******************************************************************************
 *  Copyright (c) 2015 Jamis Hoo
 *  Distributed under the MIT license 
 *  (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *  
 *  Project: Group-Share Filesystem
 *  Filename: user_fs.h 
 *  Version: 1.0
 *  Author: Jamis Hoo
 *  E-mail: hoojamis@gmail.com
 *  Date: May 30, 2015
 *  Time: 09:15:51
 *  Description: 
 *****************************************************************************/
#ifndef USER_FS_H_
#define USER_FS_H_

#include <stdexcept>
#include <boost/filesystem.hpp>
#include "dir_tree.h"
#include "host.h"

class UserFS {
public:
    UserFS(): _host_id(0) { }
    ~UserFS() {
        // TODO: when to remove tmp dir?
        /*
        using namespace boost::filesystem;
        // if delete failed, just ignore it
        try {
            remove_all(path(_tmpdir) / path(_dir));
        } catch (...) { }
        */
    }

    // calling order of functions below:
    // master node: setServer -> initDirTree -> initHost
    // normal node: initDirTree -> initHost

    void setMaster() { _host_id = 1; }

    void initHost(const std::string& addr, const uint16_t tcp_port, const uint16_t ssh_port) {
        // this functions should only be called when program initializes
        assert(hosts.size() == 0);
        assert(_host_id == 0 || _host_id == 1);
        
        // TODO: asio check addr validity

        Hosts::Host host;
        host.id = _host_id;
        host.address = addr;
        host.tmpdir = _tmpdir;
        host.tcp_port = tcp_port;
        host.ssh_port = ssh_port;
    }

    void initDirTree(const std::string& dir, const std::string& tmpdir) {
        using namespace boost::filesystem;

        // may throw filesystem_error
        
        if (!is_directory(dir))
            throw std::invalid_argument(dir + " is not directory. ");
        
        if (!exists(dir))
            throw std::invalid_argument(dir + " not exists. ");
        
        if (!is_directory(dir))
            throw std::invalid_argument(dir + " is not directory. ");

        if (exists(tmpdir / dir)) 
            throw std::invalid_argument(std::string("Assert ") + path(tmpdir / dir).string() + " not exists. ");

        if (!create_directory(tmpdir / dir))
            throw std::invalid_argument(std::string("Failed creating directory ") + path(tmpdir / dir).string() + ".");
        
        _tmpdir = tmpdir;
        _dir = dir;

        dir_tree.initialize();
        dir_tree.root()->type = DirTree::TreeNode::DIRECTORY;
        // TODO: get dir inode size
        dir_tree.root()->size = 0;
        dir_tree.root()->host_id = _host_id;

        std::function<void (const path&, const path&, const DirTree::TreeNode&) > traverseDirectory;
        traverseDirectory = [this, &traverseDirectory]
            (const path& dir, const path& tmpdir, const DirTree::TreeNode& parent)->void {
            for (auto& f: directory_iterator(dir)) {
                if (is_directory(f)) {
                    create_directory(tmpdir / f);

                    DirTree::TreeNode dirnode;
                    dirnode.type = DirTree::TreeNode::DIRECTORY;
                    dirnode.name = f.path().filename().string();
                    // TODO: get dir inode size
                    dirnode.size = 0;
                    dirnode.host_id = _host_id;

                    auto insert_rtv = parent.children.insert(dirnode);

                    traverseDirectory(f, tmpdir, *(insert_rtv.first));
                } else if (is_regular_file(f)) {
                    // create hard link (src, dst)
                    create_hard_link(f, tmpdir / f);

                    DirTree::TreeNode filenode;
                    filenode.type = DirTree::TreeNode::REGULAR;
                    filenode.name = f.path().filename().string();
                    filenode.size = file_size(f);
                    filenode.host_id = _host_id;

                    parent.children.insert(filenode);
                }
            }
        };

        traverseDirectory(dir, tmpdir, *dir_tree.root());
    }

    size_t hostID() const { return _host_id; }

    DirTree dir_tree;
    Hosts hosts;

private:
// DEBUG
public:
    // host id 0 -- uninitialized, 1 -- master, >= 2 -- other nodes
    size_t _host_id;
    std::string _tmpdir;
    std::string _dir;

};


#endif /* USER_FS_H_ */
