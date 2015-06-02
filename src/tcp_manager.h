/******************************************************************************
 *  Copyright (c) 2015 Jamis Hoo
 *  Distributed under the MIT license 
 *  (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *  
 *  Project: Group-Share Filesystem
 *  Filename: tcp_manager.h 
 *  Version: 1.0
 *  Author: Jamis Hoo
 *  E-mail: hoojamis@gmail.com
 *  Date: May 30, 2015
 *  Time: 11:29:46
 *  Description: 
 *****************************************************************************/
#ifndef TCP_MANAGER_H_
#define TCP_MANAGER_H_

#include <thread>
#include "tcp_messager.h"

class TCPManager {
public:
    TCPManager(UserFS* owner): 
        _is_master(0), _is_slave(0), 
        _master_messager(nullptr), _slave_messager(nullptr), _owner(owner) { }
    ~TCPManager() { 
        if (_thread.joinable()) 
            _thread.join(); 
        delete _master_messager;
        delete _slave_messager;
    }

    // returns true on error
    bool initialize(const bool is_master, const std::string& addr, const uint16_t port) {
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

    void start() {
        if (_is_master)
            _thread = std::thread([this]() { _master_messager->start(); });
        else if (_is_slave)
            _thread = std::thread([this]() { _slave_messager->start(); });
    }
    
    // write to all peers 
    void write(const std::string& message) {
        // construct packet
        Packet packet;
        packet.encodeData(message);

        if (_is_master)
            _master_messager->write(packet);
        else if (_is_slave)
            _slave_messager->write(packet);
    }


    // functions for slaves 

    // messager will call back this function when receiving a packet
    // packet size should just fit in with the protocol
    void read(const Packet& packet);


    // write to peer
    void writeTo(const std::string& message, const TCPMasterMessager::Connection::iterator handle) {
        assert(_is_master);
        // construct packet
        Packet packet;
        packet.encodeData(message);
        
        _master_messager->writeTo(packet, handle);
    }



    // functions for master

    // messager will call back this function when receiving a packet
    // packet size should just fit in with the protocol
    void read(const Packet& packet, const TCPMasterMessager::Connection::iterator handle);


    // close connection with one slave
    void close(const TCPMasterMessager::Connection::iterator handle) {
        _master_messager->close(handle);
    }



    /*
       For slaves,
       if fail when trying to connect or connection is interrupted
       the tcp thread will return.
       Then try inform the ssh thread to stop and then main thresd will return.

       For master,
       remove this slave from hosts and dir tree,
       then inform other slaves to update hosts and dir tree.
    */

    // TODO: how to handle disconnect?
    // disconnetion callback for slave
    // connect failed or slave disconnect from master
    void disconnect() const;

    // disconnection callback for master
    // slave disconnect from master
    // void disconnect(/* arg? */) { }

private:
    bool _is_master;
    bool _is_slave;
    TCPMasterMessager* _master_messager;
    TCPSlaveMessager* _slave_messager;
    UserFS* _owner;

    std::thread _thread;

};

#endif /* TCP_MANAGER_H_ */
