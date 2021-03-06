/******************************************************************************
 *  Copyright (c) 2015 Jamis Hoo
 *  Distributed under the MIT license 
 *  (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *  
 *  Project: Group-Share Filesystem
 *  Filename: tcp_manager.cc 
 *  Version: 1.0
 *  Author: Jamis Hoo
 *  E-mail: hoojamis@gmail.com
 *  Date: Jun  2, 2015
 *  Time: 09:00:30
 *  Description: 
 *****************************************************************************/
#include "bytes_order.h"
#include "tcp_manager.h"
#include "user_fs.h"

void TCPManager::disconnect() const {
    _owner->disconnect();
}

void TCPManager::disconnect(TCPMasterMessager::Connection::iterator handle) const {
    _owner->disconnect(handle);
}

// messager will call back this function when receiving a packet
// packet size should just fit in with the protocol
void TCPManager::read(const Packet& packet, const TCPMasterMessager::Connection::iterator handle) {
    /* 
       protocol:
       
       |   8 bytes   | packet.size() - 8 bytes |
       | packet type |       packet content    |

       packet type:
       0: slave sends self's dir tree and host info
       packet content:
       |     8 bytes     |     dir tree length     |   8 bytes   |     host length     | 
       | dir tree length | dir tree bytes sequence | host length | host bytes sequence |

       packet type:
       1: master sends recognition, slave id, merged dir tree, merged hosts info
       packet content:
       |  8 bytes |     8 bytes     |     dir tree length     |      8 bytes      |  hosts info length   |
       | slave id | dir tree length | dir tree bytes sequence | hosts info length | hosts bytes sequence |

       packet type:
       2: master sends updated dir tree and hosts info
       packet content:
       |     8 bytes     |     dir tree length     |      8 bytes      |  hosts info length   |
       | dir tree length | dir tree bytes sequence | hosts info length | hosts bytes sequence |



    */

    const char* data = packet.data();

    uint64_t protocol_type = network_to_host_64(data);
    data += sizeof(uint64_t);


    switch (protocol_type) {
        case 0: {
            uint64_t dir_tree_seq_len = network_to_host_64(data);
            data += sizeof(uint64_t);

            std::string dir_tree_seq = std::string(data, dir_tree_seq_len);
            data += dir_tree_seq_len;

            uint64_t hosts_seq_len = network_to_host_64(data);
            data += sizeof(uint64_t);

            std::string hosts_seq = std::string(data, hosts_seq_len);
            data += hosts_seq_len;

            return _owner->newConnection(dir_tree_seq, hosts_seq, handle);
        } 
    }

}

bool TCPManager::initialize(const bool is_master, const std::string& addr, const uint16_t port) {
    delete _master_messager;
    delete _slave_messager;

    if (is_master) {
        _is_master = 1;

        _master_messager = new TCPMasterMessager(this);
        _slave_messager = nullptr;

        if (_master_messager->init(addr, port))
            return 1;
    } else {
        _is_slave = 1;

        _master_messager = nullptr;
        _slave_messager = new TCPSlaveMessager(this);

        if (_slave_messager->init(addr, port))
            return 1;
    }
    return 0;
}

void TCPManager::read(const Packet& packet) {
    const char* data = packet.data();

    uint64_t protocol_type = network_to_host_64(data);
    data += sizeof(uint64_t);


    switch (protocol_type) {
        case 1: {
            uint64_t slave_id = network_to_host_64(data);
            data += sizeof(uint64_t);

            uint64_t dir_tree_seq_len = network_to_host_64(data);
            data += sizeof(uint64_t);

            std::string dir_tree_seq = std::string(data, dir_tree_seq_len);
            data += dir_tree_seq_len;

            uint64_t hosts_seq_len = network_to_host_64(data);
            data += sizeof(uint64_t);

            std::string hosts_seq = std::string(data, hosts_seq_len);
            data += hosts_seq_len;

            return _owner->slaveRecognized(slave_id, dir_tree_seq, hosts_seq);
        } case 2: {
            uint64_t dir_tree_seq_len = network_to_host_64(data);
            data += sizeof(uint64_t);

            std::string dir_tree_seq = std::string(data, dir_tree_seq_len);
            data += dir_tree_seq_len;

            uint64_t hosts_seq_len = network_to_host_64(data);
            data += sizeof(uint64_t);

            std::string hosts_seq = std::string(data, hosts_seq_len);
            data += hosts_seq_len;

            return _owner->updateInfo(dir_tree_seq, hosts_seq);
        }
    }
}

