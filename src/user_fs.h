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
#include <boost/thread/shared_mutex.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp> 
#include "bytes_order.h"
#include "dir_tree.h"
#include "host.h"
#include "tcp_manager.h"

class UserFS {
public:
    UserFS(): _host_id(0), _max_host_id(2), _slave_wait_sem(0), _tcp_manager(this) { }
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
    // master node: setMaster -> initDirTree -> initHost -> initTCPNetwork
    // slave node: initDirTree -> initHost -> initTCPNetwork

    void setMaster() { _host_id = 1; }

    void initHost(const std::string& addr, const uint16_t tcp_port, const uint16_t ssh_port) {
        boost::unique_lock< boost::shared_mutex > lock(_access);

        // this functions should only be called when program initializes
        assert(_hosts.size() == 0);
        assert(_host_id == 0 || _host_id == 1);
        
        Hosts::Host host;
        host.id = _host_id;
        host.address = addr;
        host.tmpdir = _tmpdir;
        host.tcp_port = tcp_port;
        host.ssh_port = ssh_port;

        // push self's host at 0
        _hosts.push(host);
    }

    void initDirTree(const std::string& dir, const std::string& tmpdir) {
        using namespace boost::filesystem;

        boost::unique_lock< boost::shared_mutex > lock(_access);

        // TODO: may throw filesystem_error
        
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

        _dir_tree.initialize();
        _dir_tree.root()->type = DirTree::TreeNode::DIRECTORY;
        
        // It seems there's is a portable way to get dir size, just set it to 0
        // If anybody knows how to do that, please tell me. Thanks.
        _dir_tree.root()->size = 0;
        _dir_tree.root()->host_id = _host_id;

        std::function< void (const path&, const path&, const DirTree::TreeNode&) > traverseDirectory;
        traverseDirectory = [this, &traverseDirectory]
            (const path& dir, const path& tmpdir, const DirTree::TreeNode& parent)->void {
            for (auto& f: directory_iterator(dir)) {
                auto file_type = status(f).type();

                decltype(DirTree::TreeNode::type) treenode_type;

                switch (file_type) {
                    case file_type::directory_file: 
                        treenode_type = DirTree::TreeNode::DIRECTORY;
                        break;
                    case file_type::regular_file: 
                        treenode_type = DirTree::TreeNode::REGULAR;
                        break;
                    case file_type::character_file:
                        treenode_type = DirTree::TreeNode::CHRDEVICE;
                        break;
                    case file_type::block_file:
                        treenode_type = DirTree::TreeNode::BLKDEVICE;
                        break;
                    case file_type::fifo_file:
                        treenode_type = DirTree::TreeNode::FIFO;
                        break;
                    case file_type::symlink_file:
                        treenode_type = DirTree::TreeNode::SYMLINK;
                        break;
                    case file_type::socket_file:
                        treenode_type = DirTree::TreeNode::SOCKET;
                        break;
                    case file_type::type_unknown:
                    default:
                        treenode_type = DirTree::TreeNode::UNKNOWN;
                }
                
                // TODO: maintain root directory hard link number
                // TODO: add atime ctime mtime

                if (treenode_type == DirTree::TreeNode::UNKNOWN)
                    continue;
                else if (treenode_type == DirTree::TreeNode::DIRECTORY) {
                    create_directory(tmpdir / f);

                    DirTree::TreeNode dirnode;
                    dirnode.type = DirTree::TreeNode::DIRECTORY;
                    dirnode.name = f.path().filename().string();
                    dirnode.size = 0;
                    dirnode.host_id = _host_id;
                    dirnode.num_links = hard_link_count(f.path());

                    auto insert_rtv = parent.children.insert(dirnode);

                    traverseDirectory(f, tmpdir, *(insert_rtv.first));
                } else {
                    // create hard link (src, dst)
                    create_hard_link(f, tmpdir / f);

                    DirTree::TreeNode filenode;
                    filenode.type = DirTree::TreeNode::REGULAR;
                    filenode.name = f.path().filename().string();
                    filenode.size = file_size(f);
                    filenode.host_id = _host_id;
                    // GSFS create another hard link for files except directory
                    filenode.num_links = 1 + hard_link_count(f.path());

                    parent.children.insert(filenode);
                }
            }
        };

        traverseDirectory(dir, tmpdir, *_dir_tree.root());
    }

    // returns true on error
    bool initTCPNetwork(const std::string& addr, const uint16_t port) {
        bool is_master = _host_id == 1;

        bool init_rtv = _tcp_manager.initialize(is_master, addr, port);

        if (init_rtv) return 1;

        _tcp_manager.start();

        // push master's host
        if (is_master) 
            _hosts.push(_hosts[0]);
        // else wait for master's recognition or rejection
        else {
            std::string dir_tree_seq;
            std::string host_seq;

            {
                boost::shared_lock< boost::shared_mutex > lock(_access);
                dir_tree_seq = DirTree::serialize(_dir_tree);
                host_seq = Hosts::Host::serialize(_hosts[0]);
            }

            std::string dir_tree_seq_len = host_to_network_64(dir_tree_seq.length());
            std::string host_seq_len = host_to_network_64(host_seq.length());
            std::string protocol_type = host_to_network_64(0x00);

            std::string message = protocol_type +
                                  dir_tree_seq_len + dir_tree_seq +
                                  host_seq_len + host_seq;

            _tcp_manager.write(message);

            _slave_wait_sem.wait();
        }

        return 0;
    }

    size_t hostID() const { return _host_id; }

    const DirTree::TreeNode* find(const std::string& path) {
        boost::shared_lock< boost::shared_mutex > lock(_access);
        return _dir_tree.find(path);
    }

    // functions for slaves' tcp manager:

    // slave get recognized from master
    void slaveRecognized(const uint64_t slave_id, const std::string& dir_tree_seq, const std::string& hosts_seq) {
        // deploy dir tree
        DirTree merged_tree = DirTree::deserialize(dir_tree_seq);
        
        // deploy hosts
        _host_id = slave_id;
        Hosts merged_hosts = Hosts::deserialize(hosts_seq);
        
        {
            boost::unique_lock< boost::shared_mutex > lock(_access);
            _dir_tree.root()->children = merged_tree.root()->children;
            _hosts = merged_hosts;
        }
        // wake up main thread
        _slave_wait_sem.post();
    }

    // master sent a update packet, update dirtree and hosts
    void updateInfo(const std::string& dir_tree_seq, const std::string& hosts_seq) {
        DirTree new_tree = DirTree::deserialize(dir_tree_seq);
        Hosts merged_hosts = Hosts::deserialize(hosts_seq);

        {
            boost::unique_lock< boost::shared_mutex > lock(_access);
            _dir_tree.root()->children = new_tree.root()->children;
            _hosts = merged_hosts;
        }
    }


    // TODO: how to handle disconnection?
    // void disconnect() { }


    // functions for master' tcp manager:

    // new slave coming, chekc dirtree, allocate host_id, merge dir_tree and hosts
    void newConnection(const std::string& dir_tree_seq, 
                       const std::string& host_seq, 
                       const TCPMasterMessager::Connection::iterator handle) {
        // deserialize
        DirTree slave_dir_tree = DirTree::deserialize(dir_tree_seq);
        Hosts::Host slave_host = Hosts::Host::deserialize(host_seq);

        // check conflicts
        std::vector<std::string> conflicts;
        
        {
           boost::shared_lock< boost::shared_mutex > lock(_access);
           conflicts = _dir_tree.hasConflict(slave_dir_tree);
        }
    
        // there's conflict, close connection
        if (conflicts.size()) return _tcp_manager.close(handle);
        // else

        // alloc slave id
        uint64_t slave_id = _max_host_id++;
        slave_host.id = slave_id;
        
        {
            boost::unique_lock< boost::shared_mutex > lock(_access); 

            // merge dir tree
            _dir_tree.merge(slave_dir_tree, slave_id);
           
            // merge host 
            _hosts.push(slave_host);

            assert(_hosts.size() == slave_id + 1);
        }


        // construct response message

        std::string merged_dir_tree_seq;
        std::string merged_hosts_seq;

        { 
            boost::shared_lock< boost::shared_mutex > lock(_access);
            merged_dir_tree_seq = DirTree::serialize(_dir_tree);
            merged_hosts_seq = Hosts::serialize(_hosts);
        }

        std::string merged_dir_tree_seq_len = host_to_network_64(merged_dir_tree_seq.length());
        std::string merged_hosts_seq_len = host_to_network_64(merged_hosts_seq.length());
        std::string protocol_type = host_to_network_64(0x01);
        std::string slave_id_seq = host_to_network_64(slave_id);

        std::string message = protocol_type + slave_id_seq +
                              merged_dir_tree_seq_len + merged_dir_tree_seq +
                              merged_hosts_seq_len + merged_hosts_seq;

        // send to slave
        _tcp_manager.writeTo(message, handle);
        sendUpdate(merged_dir_tree_seq, merged_hosts_seq);
    }

    // send update packet to all slaves
    void sendUpdate() {
        std::string dir_tree_seq;
        std::string hosts_seq;

        {
            boost::shared_lock< boost::shared_mutex > lock(_access);
            dir_tree_seq = DirTree::serialize(_dir_tree);
            hosts_seq = Hosts::serialize(_hosts);
        }

        sendUpdate(dir_tree_seq, hosts_seq);
    }
    void sendUpdate(const std::string& dir_tree_seq, const std::string& hosts_seq) {
        std::string message = host_to_network_64(0x02);
        message += host_to_network_64(dir_tree_seq.length());
        message += dir_tree_seq;
        message += host_to_network_64(hosts_seq.length());
        message += hosts_seq;

        // send to all slaves
        _tcp_manager.write(message);
    }

private:
    
    // This sem is used to block slave node until it's get master's recognization and dir tree
    boost::interprocess::interprocess_semaphore _slave_wait_sem;

    // host id 0 -- uninitialized, 1 -- master, >= 2 -- other nodes
    uint64_t _host_id;
    std::string _tmpdir;
    std::string _dir;

    // for master, alloc host id for new connected slave
    uint64_t _max_host_id;


    // lock for _dir_tree and _hosts
    boost::shared_mutex _access;

    // access to dir tree and hosts should be controlled with lock or sem or condition variable
    DirTree _dir_tree;
    Hosts _hosts;

    TCPManager _tcp_manager;

};


#endif /* USER_FS_H_ */
