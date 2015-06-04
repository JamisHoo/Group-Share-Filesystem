/******************************************************************************
 *  Copyright (c) 2015 Jamis Hoo
 *  Distributed under the MIT license 
 *  (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *  
 *  Project: Group-Share Filesystem
 *  Filename: tcp_messager.h 
 *  Version: 1.0
 *  Author: Jamis Hoo
 *  E-mail: hoojamis@gmail.com
 *  Date: Jun  1, 2015
 *  Time: 10:41:50
 *  Description: messager is used to pass message between two peers
 *****************************************************************************/
#ifndef TCP_MESSAGER_H_
#define TCP_MESSAGER_H_

// DEBUG
#include <iostream>
#include <queue>
#include <list>
#include <boost/asio.hpp>
#include "bytes_order.h"

class TCPManager;
class UserFS;

class Packet {
    // sending packet:
    // _size : length of primary data + sizeof(_size)
    // _data : _size in big endian + primary data
    // received packet:
    // _size : length of primary data
    // _data : primary data
    uint64_t _size;
    std::string _data;
public: 
    Packet() { _size = 0; }

    void encodeData(const std::string& data) {
        _size = data.length();
        _data = host_to_network_64(_size);
        _size += sizeof(_size);
        _data += data;
    }

    void decodeSize(const char* size) {
        _size = network_to_host_64(size);
    }

    void setData(const char* data) { _data.assign(data, _size); }

    const char* data() const { return _data.data(); }
    size_t size() const { return _size; }
    static size_t sizeLength() { return sizeof(_size); }
};

class TCPSlaveMessager {
public:
    TCPSlaveMessager(TCPManager* owner): 
        _socket(_io_service), _resolver(_io_service), _owner(owner) { }

    // returns true on error
    bool init(const std::string& addr, const uint16_t port);

    // thread call this function will do connect, read, and write
    void start();

    void stop();

    // send packet to master
    // packet can be released when this function returns
    void write(const Packet& packet);
    
    // a packet has been read, pass it to owner
    void read(const Packet& packet) const;

    // connect master failed, or connection interrupted
    void disconnect() const;
    
private:
    void do_connect(boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
    void do_read_header();
    void do_read_body();
    void do_write();


    boost::asio::io_service _io_service;
    boost::asio::ip::tcp::socket _socket;
    boost::asio::ip::tcp::resolver _resolver;
    boost::asio::ip::tcp::resolver::iterator _endpoint_iterator;
    Packet _packet;
    std::vector<char> _buffer;
    std::queue<Packet> _write_packets;

    TCPManager* _owner;
};

class TCPMasterMessager {
public:
    typedef std::list< 
        std::tuple<
                   // socket of this connection
                   boost::asio::ip::tcp::socket, 
                   // receive buffer of this connection
                   std::vector<char>, 
                   // receive packet of this connection
                   Packet, 
                   // sending queue of this connection
                   std::queue< std::shared_ptr<Packet> >,
                   // slave id
                   uint64_t
                  > > Connection;

    TCPMasterMessager(TCPManager* owner): 
        _acceptor(_io_service), _socket(_io_service), _resolver(_io_service), _owner(owner) { }

    // returns true on error
    bool init(const std::string& addr, const uint16_t port);

    // thread call this function will do accept, read and write
    void start();

    void stop();

    // send packet to all slaves
    // packet can be released when this function returns
    void write(const Packet& packet);

    void writeTo(const Packet packet, const Connection::iterator iter);

    // a pakcet has been read, send it to owner
    void read(const Packet& packet, Connection::iterator iter) const;

    // close connection with a slave
    void close(Connection::iterator connect_iter);

private:
    void disconnect(Connection::iterator iter) const;

    void do_accept();

    void do_new_connection(Connection::iterator connect_iter);

    void do_read_header(Connection::iterator connect_iter);
    
    void do_read_body(Connection::iterator connect_iter);

    void do_write(Connection::iterator connect_iter);

    boost::asio::io_service _io_service;
    boost::asio::ip::tcp::acceptor _acceptor; 
    boost::asio::ip::tcp::socket _socket;
    boost::asio::ip::tcp::resolver _resolver;
    boost::asio::ip::tcp::resolver::iterator _endpoint_iterator;

    Connection _connections;

    TCPManager* _owner;
};

#endif /* TCP_MESSAGER_H_ */
