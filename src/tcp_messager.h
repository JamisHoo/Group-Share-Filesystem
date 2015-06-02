/******************************************************************************
 *  Copyright (c) 2015 Jamis Hoo
 *  Distributed under the MIT license 
 *  (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *  
 *  Project: Group-Sharing Filesystem
 *  Filename: tcp_messager.h 
 *  Version: 1.0
 *  Author: Jamis Hoo
 *  E-mail: hoojamis@gmail.com
 *  Date: Jun  1, 2015
 *  Time: 10:41:50
 *  Description: 
 *****************************************************************************/
#ifndef TCP_MESSAGER_H_
#define TCP_MESSAGER_H_

#include <iostream>
#include <string>
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
    uint32_t _size;
    std::string _data;
public: 
    Packet() { _size = 0; }

    void encodeData(const std::string& data) {
        _size = data.length();
        _data.resize(sizeof(_size));

        // TODO; replace with byte_order function
        for (size_t i = 0; i < sizeof(_size); ++i)
            _data[i] = _size >> i * 8 & 0xff;

        _size += sizeof(_size);

        _data += data;
    }

    void decodeSize(const char* size) {
        _size = network_to_host_32(size);
    }

    //void setData(const std::string& data) { _data = data; }

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
    bool init(const std::string& addr, const uint16_t port) {
        boost::system::error_code ec;
        _endpoint_iterator = _resolver.resolve({ addr, std::to_string(port) }, ec);

        return ec;
    }

    // thread call this function will do connect, read, and write
    void start() {
        do_connect(_endpoint_iterator);
        _io_service.run();
    }

    // send packet to master
    // packet can be released when this function returns
    void write(const Packet& packet) {
        // move packet to heap
        Packet* packet_in_heap = new Packet(packet);
        
        _io_service.post(
        // TODO: just pass by value
        [this, packet_in_heap]() {
            bool write_in_progress = _write_packets.size();

            _write_packets.push(*packet_in_heap);

            delete packet_in_heap;

            if (!write_in_progress) 
                do_write();
        });
    }
    
    // a packet has been read, pass it to owner
    void read(const Packet& packet) const;

    // TODO?
    // connect master failed, or connection interrupted
    void disconnect() const;
    
private:
    void do_connect(boost::asio::ip::tcp::resolver::iterator endpoint_iterator) {
        boost::asio::async_connect(_socket, endpoint_iterator, 
        [this](boost::system::error_code ec, boost::asio::ip::tcp::resolver::iterator) {
            if (!ec) do_read_header();
            else disconnect();
        });
    }

    void do_read_header() {
        _buffer.resize(_packet.sizeLength());
        boost::asio::async_read(_socket, 
            boost::asio::buffer(_buffer.data(), _packet.sizeLength()),
        [this](boost::system::error_code ec, std::size_t length) {
            if (!ec && length == _packet.sizeLength()) {
                _packet.decodeSize(_buffer.data());

                do_read_body();
            } else {
                _socket.close();
                disconnect();
            }
        });
    }

    void do_read_body() {
        _buffer.resize(_packet.size());
        boost::asio::async_read(_socket,
            boost::asio::buffer(_buffer.data(), _packet.size()),
        [this](boost::system::error_code ec, std::size_t length) {
            if (!ec && length == _packet.size()) {
                _packet.setData(_buffer.data());
                read(_packet);
                do_read_header();
            } else {
                _socket.close();
                disconnect();
            }
        });
    }

    void do_write() {
        boost::asio::async_write(_socket,
            boost::asio::buffer(_write_packets.front().data(), 
                                _write_packets.front().size()),
        [this](boost::system::error_code ec, std::size_t length) {
            if (!ec && length == _write_packets.front().size()) {
                _write_packets.pop();
                if (!_write_packets.empty()) do_write();
            } else {
                _socket.close();
                disconnect();
            }
        });
    }


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
                   std::queue< std::shared_ptr<Packet> > 
                  > > Connection;

    TCPMasterMessager(TCPManager* owner): 
        _acceptor(_io_service), _socket(_io_service), _resolver(_io_service), _owner(owner) { }

    // returns true on error
    bool init(const std::string& addr, const uint16_t port) {
        boost::system::error_code ec;
        _endpoint_iterator = _resolver.resolve({ addr, std::to_string(port) }, ec);

        auto i = _endpoint_iterator;
        if (i == boost::asio::ip::tcp::resolver::iterator())
            return true;
        if (++i != boost::asio::ip::tcp::resolver::iterator()) 
            return true;

        return ec;
    }

    // thread call this function will do accept, read and write
    void start() {
        _acceptor.open(_endpoint_iterator->endpoint().protocol());
        _acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        _acceptor.bind(_endpoint_iterator->endpoint());
        _acceptor.listen();

        do_accept();
        _io_service.run();
    }

    // send packet to all slaves
    // packet can be released when this function returns
    void write(const Packet& packet) {
        // we need to move packet to heap for this is async write
        Packet* packet_in_heap = new Packet(packet);

        _io_service.post(
        [this, packet_in_heap]() {
            std::shared_ptr<Packet> pointer(packet_in_heap);            
            
            for (auto connect_iter = _connections.begin(); connect_iter != _connections.end(); ++connect_iter) {
                std::queue< std::shared_ptr<Packet> >& queue = std::get<3>(*connect_iter);
                bool write_in_progress = queue.size();
                queue.push(pointer);
                if (!write_in_progress)
                    do_write(connect_iter);
            }
        });
    }

    void writeTo(const Packet packet, const Connection::iterator iter) {
        // move packet to heap
        Packet* packet_in_heap = new Packet(packet);

        _io_service.post(
        [this, packet_in_heap, iter]() {
            std::shared_ptr<Packet> pointer(packet_in_heap);

            std::queue< std::shared_ptr<Packet> >& queue = std::get<3>(*iter);
            bool write_in_progress = queue.size();
            queue.push(pointer);
            if (!write_in_progress)
                do_write(iter);
        });
    }

    // a pakcet has been read, send it to owner
    void read(const Packet& packet, Connection::iterator iter) const;

    // close connection with a slave
    void close(Connection::iterator connect_iter) {
        boost::asio::ip::tcp::socket& socket = std::get<0>(*connect_iter);


        boost::system::error_code ec;
        socket.close(ec);
        
        // if erase this element from list,
        // there's one scenario that the iterator is erased more than once 
        // because this function maybe interrupted by async I/O
        // _connections.erase(connect_iter);
    }

private:
    void do_accept() {
        _acceptor.async_accept(_socket, 
        [this](boost::system::error_code ec) {
            if (!ec) {
                _connections.push_front({ std::move(_socket), 
                                          std::vector<char>(), 
                                          Packet(), 
                                          std::queue< std::shared_ptr<Packet> >() });

                Connection::iterator connect_iter = _connections.begin();

                do_new_connection(connect_iter); 
            }
            do_accept();
        });
    }

    void do_new_connection(Connection::iterator connect_iter) {
        boost::asio::ip::tcp::socket& socket = std::get<0>(*connect_iter);
        
        std::cout << "New connection from " 
                  << socket.remote_endpoint().address().to_string() << ":"
                  << socket.remote_endpoint().port() << std::endl;
        
        do_read_header(connect_iter);
    }

    void do_read_header(Connection::iterator connect_iter) {
        boost::asio::ip::tcp::socket& socket = std::get<0>(*connect_iter);
        std::vector<char>& buffer = std::get<1>(*connect_iter);
        Packet& packet = std::get<2>(*connect_iter);

        buffer.resize(Packet::sizeLength());

        boost::asio::async_read(socket, 
            boost::asio::buffer(buffer.data(), Packet::sizeLength()),

        [this, connect_iter, &socket, &buffer, &packet](boost::system::error_code ec, std::size_t length) {
            if (!ec && length == Packet::sizeLength()) {
                
                packet.decodeSize(buffer.data());

                do_read_body(connect_iter);
            } else {
                close(connect_iter);
            }
        });
    }
    
    void do_read_body(Connection::iterator connect_iter) {
        boost::asio::ip::tcp::socket& socket = std::get<0>(*connect_iter);
        std::vector<char>& buffer = std::get<1>(*connect_iter);
        Packet& packet = std::get<2>(*connect_iter);

        buffer.resize(packet.size());

        boost::asio::async_read(socket,
            boost::asio::buffer(buffer.data(), packet.size()),

        [this, connect_iter, &socket, &buffer, &packet](boost::system::error_code ec, std::size_t length) {
            if (!ec && length == packet.size()) {
                packet.setData(buffer.data());

                read(packet, connect_iter);

                do_read_header(connect_iter);
            } else {
                close(connect_iter);
            }
        });
    }

    
    void do_write(Connection::iterator connect_iter) {
        boost::asio::ip::tcp::socket& socket = std::get<0>(*connect_iter);
        std::queue< std::shared_ptr<Packet> >& send_queue = std::get<3>(*connect_iter);

        boost::asio::async_write(socket,
            boost::asio::buffer(send_queue.front()->data(), 
                                send_queue.front()->size()),

        [this, connect_iter, &socket, &send_queue](boost::system::error_code ec, std::size_t length) {
            if (!ec && length == send_queue.front()->size()) {
                send_queue.pop();
                if (!send_queue.empty()) do_write(connect_iter);
            } else {
                close(connect_iter);
            }
        });
    }

    

    boost::asio::io_service _io_service;
    boost::asio::ip::tcp::acceptor _acceptor; 
    boost::asio::ip::tcp::socket _socket;
    boost::asio::ip::tcp::resolver _resolver;
    boost::asio::ip::tcp::resolver::iterator _endpoint_iterator;

    Connection _connections;

    TCPManager* _owner;
};

#endif /* TCP_MESSAGER_H_ */
