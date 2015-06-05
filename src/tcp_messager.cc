/******************************************************************************
 *  Copyright (c) 2015 Jamis Hoo
 *  Distributed under the MIT license 
 *  (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *  
 *  Project: Group-Share Filesystem
 *  Filename: tcp_messager.cc 
 *  Version: 1.0
 *  Author: Jamis Hoo
 *  E-mail: hoojamis@gmail.com
 *  Date: Jun  1, 2015
 *  Time: 16:15:59
 *  Description: messager is used to pass messages between two peers
 *****************************************************************************/
#include "tcp_messager.h"
#include <iostream>
#include "tcp_manager.h"

void TCPSlaveMessager::read(const Packet& packet) const {
    _owner->read(packet);
}

void TCPSlaveMessager::disconnect() const {
    _owner->disconnect();
}

// returns true on error
bool TCPSlaveMessager::init(const std::string& addr, const uint16_t port) {
    boost::system::error_code ec;
    _endpoint_iterator = _resolver.resolve({ addr, std::to_string(port) }, ec);

    return ec;
}

// thread call this function will do connect, read, and write
void TCPSlaveMessager::start() {
    do_connect(_endpoint_iterator);
    _io_service.run();
}

void TCPSlaveMessager::stop() {
    _io_service.post(
    [this]() {
        _io_service.stop();
    });
}

// send packet to master
// packet can be released when this function returns
void TCPSlaveMessager::write(const Packet& packet) {
    _io_service.post(
    [this, packet]() {
        bool write_in_progress = _write_packets.size();

        _write_packets.push(packet);

        if (!write_in_progress) 
            do_write();
    });
}

void TCPSlaveMessager::do_connect(boost::asio::ip::tcp::resolver::iterator endpoint_iterator) {
    boost::asio::async_connect(_socket, endpoint_iterator, 
    [this](boost::system::error_code ec, boost::asio::ip::tcp::resolver::iterator) {
        if (!ec) do_read_header();
        else {
            std::cerr << "Cannot connect to master. But filesystem containing only local files is still mounted. ";
            disconnect();
        }
    });
}

void TCPSlaveMessager::do_read_header() {
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

void TCPSlaveMessager::do_read_body() {
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

void TCPSlaveMessager::do_write() {
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

void TCPMasterMessager::read(const Packet& packet, Connection::iterator iter) const {
    _owner->read(packet, iter);
}

void TCPMasterMessager::disconnect(Connection::iterator iter) const {
    _owner->disconnect(iter);
}

// returns true on error
bool TCPMasterMessager::init(const std::string& addr, const uint16_t port) {
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
void TCPMasterMessager::start() {
    boost::system::error_code ec;

    _acceptor.open(_endpoint_iterator->endpoint().protocol(), ec);

    if (!ec)
        _acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
    if (!ec)
        _acceptor.bind(_endpoint_iterator->endpoint(), ec);
    if (!ec)
        _acceptor.listen(boost::asio::socket_base::max_connections, ec);

    if (ec) {
        std::cerr << "TCP accept error. " << std::endl;
        return;
    }

    do_accept();
    _io_service.run();
}

void TCPMasterMessager::stop() {
    _io_service.post(
    [this]() {
        _io_service.stop();
    });
}

// send packet to all slaves
// packet can be released when this function returns
void TCPMasterMessager::write(const Packet& packet) {

    _io_service.post(
    [this, packet]() {
        std::shared_ptr<Packet> pointer(new Packet(packet));
        
        for (auto connect_iter = _connections.begin(); connect_iter != _connections.end(); ++connect_iter) {
            boost::asio::ip::tcp::socket& socket = std::get<0>(*connect_iter);
            std::queue< std::shared_ptr<Packet> >& queue = std::get<3>(*connect_iter);

            if (!socket.is_open()) continue;
            
            bool write_in_progress = queue.size();
            queue.push(pointer);
            if (!write_in_progress)
                do_write(connect_iter);
        }
    });
}

void TCPMasterMessager::writeTo(const Packet packet, const Connection::iterator iter) {

    _io_service.post(
    [this, packet, iter]() {
        std::shared_ptr<Packet> pointer(new Packet(packet));

        boost::asio::ip::tcp::socket& socket = std::get<0>(*iter);
        std::queue< std::shared_ptr<Packet> >& queue = std::get<3>(*iter);
        
        bool write_in_progress = queue.size();
        queue.push(pointer);
        if (!write_in_progress)
            do_write(iter);
    });
}

// close connection with a slave
void TCPMasterMessager::close(Connection::iterator connect_iter) {
    boost::asio::ip::tcp::socket& socket = std::get<0>(*connect_iter);


    if (socket.is_open()) {
        boost::system::error_code ec;
        socket.close(ec);

        // clear sending queue
        std::queue< std::shared_ptr<Packet> >& queue = std::get<3>(*connect_iter);
        std::queue< std::shared_ptr<Packet> > empty_queue;
        queue.swap(empty_queue);
    
        // if erase this element from list,
        // there's one scenario that the iterator is erased more than once 
        // because this function maybe interrupted by async I/O
        // _connections.erase(connect_iter);

        disconnect(connect_iter);
    }
}

void TCPMasterMessager::do_accept() {
    _acceptor.async_accept(_socket, 
    [this](boost::system::error_code ec) {
        if (!ec) {
            _connections.push_front(
                std::make_tuple(
                                std::move(_socket), 
                                std::vector<char>(), 
                                Packet(), 
                                std::queue< std::shared_ptr<Packet> >(),
                                0
                               )
                                   );

            Connection::iterator connect_iter = _connections.begin();

            do_new_connection(connect_iter); 
        }
        do_accept();
    });
}

void TCPMasterMessager::do_new_connection(Connection::iterator connect_iter) {
    boost::asio::ip::tcp::socket& socket = std::get<0>(*connect_iter);
    
    do_read_header(connect_iter);
}

void TCPMasterMessager::do_read_header(Connection::iterator connect_iter) {
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

void TCPMasterMessager::do_read_body(Connection::iterator connect_iter) {
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


void TCPMasterMessager::do_write(Connection::iterator connect_iter) {
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


