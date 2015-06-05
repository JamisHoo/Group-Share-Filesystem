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
 *  Description: main class of this GSFS
 *****************************************************************************/
#ifndef USER_FS_H_
#define USER_FS_H_

#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp> 
#include "dir_tree.h"
#include "host.h"
#include "tcp_manager.h"
#include "ssh_manager.h"

class UserFS {
public:
    UserFS(): _host_id(0), _max_host_id(2), _slave_wait_sem(0), 
              _main_thread_is_waiting(1), _tcp_manager(this) { }
    ~UserFS() {
        using namespace boost::filesystem;
        // if delete failed, just ignore it
        try {
            remove_all(path(_tmpdir) / path(_dir));
        } catch (...) { }
    }

    // calling order of functions below:
    // master node: setMaster -> initDirTree -> initHost -> initTCPNetwork
    // slave node: initDirTree -> initHost -> initTCPNetwork

    void setMaster();

    void initHost(const std::string& addr, const uint16_t tcp_port, 
                  const uint16_t ssh_port);

    // this function may throw exceptions
    void initDirTree(const std::string& dir, const std::string& tmpdir);

    // returns true on error
    bool initTCPNetwork(const std::string& addr, const uint16_t port);

    const DirTree::TreeNode* find(const std::string& path);

    // returns num of bytes read on success
    // returns < 0 on error
    intmax_t read(const uint64_t node_id, const std::string path, 
                  const size_t offset, const size_t size, char* buff);

    // send update packet to all slaves
    void sendUpdate();

    void sendUpdate(const std::string& dir_tree_seq, const std::string& hosts_seq);

    // Callback Functions for slaves' tcp manager:

    // slave get recognized from master
    void slaveRecognized(const uint64_t slave_id, const std::string& dir_tree_seq, 
                         const std::string& hosts_seq);

    // master sent a update packet, update dirtree and hosts
    void updateInfo(const std::string& dir_tree_seq, const std::string& hosts_seq);

    // connect failed or slave disconnect from master
    void disconnect();


    // Callback Functions for master' tcp manager:

    // slave disconnect from master
    void disconnect(const TCPMasterMessager::Connection::iterator handle);

    // new slave coming, chekc dirtree, allocate host_id, merge dir_tree and hosts
    void newConnection(const std::string& dir_tree_seq, 
                       const std::string& host_seq, 
                       const TCPMasterMessager::Connection::iterator handle);

    size_t hostID() const { return _host_id; }

private:
    
    // This sem is used to block slave node until it's get master's recognization and dir tree
    bool _main_thread_is_waiting;
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

    SSHManager _ssh_manager;
};


#endif /* USER_FS_H_ */
