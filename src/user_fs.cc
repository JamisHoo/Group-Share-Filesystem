/******************************************************************************
 *  Copyright (c) 2015 Jamis Hoo
 *  Distributed under the MIT license 
 *  (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *  
 *  Project: 
 *  Filename: user_fs.cc 
 *  Version: 1.0
 *  Author: Jamis Hoo
 *  E-mail: hoojamis@gmail.com
 *  Date: Jun  4, 2015
 *  Time: 22:52:30
 *  Description: main class of this GSFS
 *****************************************************************************/
#include "user_fs.h"
#include <stdexcept>
#include "bytes_order.h"


// calling order of functions below:
// master node: setMaster -> initDirTree -> initHost -> initTCPNetwork
// slave node: initDirTree -> initHost -> initTCPNetwork

void UserFS::setMaster() { _host_id = 1; }

void UserFS::initHost(const std::string& addr, const uint16_t tcp_port, const uint16_t ssh_port) {
    boost::unique_lock< boost::shared_mutex > lock(_access);

    // this functions should only be called when program initializes
    
    Hosts::Host host;
    host.id = _host_id;
    host.address = addr;
    host.mountdir = _dir;
    host.tmpdir = _tmpdir;
    host.tcp_port = tcp_port;
    host.ssh_port = ssh_port;

    // push self's host at 0
    _hosts.push(host);
}

// this function may throw exceptions
void UserFS::initDirTree(const std::string& dir, const std::string& tmpdir) {
    using namespace boost::filesystem;

    boost::unique_lock< boost::shared_mutex > lock(_access);

    if (!exists(dir))
        throw std::invalid_argument(dir + " not exists. ");

    if (!is_directory(dir))
        throw std::invalid_argument(dir + " is not directory. ");
    
    if (exists(tmpdir / dir)) 
        throw std::invalid_argument(std::string("Assert ") + path(tmpdir / dir).string() + " not exists. ");

    if (!is_directory(dir))
        throw std::invalid_argument(dir + " is not directory. ");

    if (!create_directory(tmpdir / dir))
        throw std::invalid_argument(std::string("Failed creating directory ") + path(tmpdir / dir).string() + ".");
    
    _tmpdir = absolute(tmpdir).string();
    _dir = dir;

    _dir_tree.initialize();
    _dir_tree.root()->type = DirTree::TreeNode::DIRECTORY;
    // It seems there's is a portable way to get dir size, just set it to 0
    // If anybody knows how to do that, please tell me. Thanks.
    _dir_tree.root()->size = 0;
    _dir_tree.root()->host_id = _host_id;
    _dir_tree.root()->mtime = last_write_time(dir);
    _dir_tree.root()->num_links = hard_link_count(dir);

    std::function< void (const path&, const path&, const DirTree::TreeNode&) > traverseDirectory;
    traverseDirectory = [this, &traverseDirectory]
        (const path& dir, const path& _tmpdir, const DirTree::TreeNode& parent)->void {
        for (auto& f: directory_iterator(dir)) {
            auto file_type = symlink_status(f).type();

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
            

            if (treenode_type == DirTree::TreeNode::UNKNOWN)
                continue;
            else if (treenode_type == DirTree::TreeNode::DIRECTORY) {
                create_directory(_tmpdir / f.path());

                DirTree::TreeNode dirnode;
                dirnode.type = treenode_type;
                dirnode.name = f.path().filename().string();
                dirnode.size = 0;
                dirnode.mtime = last_write_time(f.path());
                dirnode.host_id = _host_id;
                dirnode.num_links = hard_link_count(f.path());

                auto insert_rtv = parent.children.insert(dirnode);

                traverseDirectory(f, _tmpdir, *(insert_rtv.first));
            } else {
                // create hard link (src, dst)
                create_hard_link(f, _tmpdir / f.path());

                DirTree::TreeNode filenode;
                filenode.type = treenode_type;
                filenode.name = f.path().filename().string();
                filenode.size = file_size(f);
                filenode.mtime = last_write_time(f.path());
                filenode.host_id = _host_id;
                // GSFS create another hard link for files except directory
                filenode.num_links = 1 + hard_link_count(f.path());

                parent.children.insert(filenode);
            }
        }
    };

    traverseDirectory(_dir, _tmpdir, *_dir_tree.root());
}

// returns true on error
bool UserFS::initTCPNetwork(const std::string& addr, const uint16_t port) {
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

const DirTree::TreeNode* UserFS::find(const std::string& path) {
    boost::shared_lock< boost::shared_mutex > lock(_access);
    return _dir_tree.find(path);
}

// returns num of bytes read on success
// returns < 0 on error
intmax_t UserFS::read(const uint64_t node_id, const std::string path, 
              const size_t offset, const size_t size, char* buff) {
    // remote node isn't inserted into ssh manager
    if (!_ssh_manager.findHost(node_id)) {
        bool rtv = _ssh_manager.insertHost(node_id, _hosts[node_id].address, _hosts[node_id].ssh_port);
        if (rtv) return -1;
    }
    
    boost::filesystem::path remote_path = _hosts[node_id].tmpdir;
    remote_path /= _hosts[node_id].mountdir;
    remote_path /= path;

    std::string remote_path_string = remote_path.string();

    // read local file
    if (node_id == _host_id) {
        std::ifstream fin(remote_path_string);
        fin.seekg(offset);
        fin.read(buff, size);
        if (fin.fail() || fin.bad()) return -1;
        size_t bytes_read = fin.gcount();
        return bytes_read;
    }

    // read remote file
    return _ssh_manager.read(node_id, remote_path_string, offset, size, buff);
}

// send update packet to all slaves
void UserFS::sendUpdate() {
    std::string dir_tree_seq;
    std::string hosts_seq;

    {
        boost::shared_lock< boost::shared_mutex > lock(_access);
        dir_tree_seq = DirTree::serialize(_dir_tree);
        hosts_seq = Hosts::serialize(_hosts);
    }

    sendUpdate(dir_tree_seq, hosts_seq);
}

void UserFS::sendUpdate(const std::string& dir_tree_seq, const std::string& hosts_seq) {
    std::string message = host_to_network_64(0x02);
    message += host_to_network_64(dir_tree_seq.length());
    message += dir_tree_seq;
    message += host_to_network_64(hosts_seq.length());
    message += hosts_seq;

    // send to all slaves
    _tcp_manager.write(message);
}

// Callback Functions for slaves' tcp manager:

// slave get recognized from master
void UserFS::slaveRecognized(const uint64_t slave_id, const std::string& dir_tree_seq, const std::string& hosts_seq) {
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
    _main_thread_is_waiting = 0;
    _slave_wait_sem.post();
}

// master sent a update packet, update dirtree and hosts
void UserFS::updateInfo(const std::string& dir_tree_seq, const std::string& hosts_seq) {
    DirTree new_tree = DirTree::deserialize(dir_tree_seq);
    Hosts merged_hosts = Hosts::deserialize(hosts_seq);

    {
        boost::unique_lock< boost::shared_mutex > lock(_access);
        _dir_tree.root()->children = new_tree.root()->children;
        _hosts = merged_hosts;
    }
}


// connect failed or slave disconnect from master
void UserFS::disconnect() {
    // clear dir tree
    { 
        boost::unique_lock< boost::shared_mutex > lock(_access);
        _dir_tree.removeNotOf(_host_id);
    }
    // if the first attempt to connect to master is failed,
    // recognition message will never come, and main thread will forever wait.
    // wake up the waiting main thread
    if (_main_thread_is_waiting) {
        std::cerr << "Cannot connect to master. But filesystem with only local files is still mounted. " << std::endl;
        _main_thread_is_waiting = 0;
        _slave_wait_sem.post();
    }
}


// Callback Functions for master' tcp manager:

// slave disconnect from master
void UserFS::disconnect(const TCPMasterMessager::Connection::iterator handle) {
    uint64_t& slave_id = std::get<4>(*handle);

    // slave is initializting
    if (slave_id == 0) return;

    // remove this slave's node in dir tree
    {
        boost::unique_lock< boost::shared_mutex > lock(_access);
        _dir_tree.removeOf(slave_id);
    }

    slave_id = 0;
    sendUpdate();
}

// new slave coming, chekc dirtree, allocate host_id, merge dir_tree and hosts
void UserFS::newConnection(const std::string& dir_tree_seq, 
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
    slave_dir_tree.root()->setHostID(slave_id);

    std::get<4>(*handle) = slave_id;
    
    {
        boost::unique_lock< boost::shared_mutex > lock(_access); 

        // merge dir tree
        _dir_tree.merge(slave_dir_tree);
       
        // merge host 
        _hosts.push(slave_host);
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





