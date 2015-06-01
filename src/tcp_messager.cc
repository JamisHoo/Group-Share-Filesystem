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
 *  Description: 
 *****************************************************************************/
#include "tcp_messager.h"
#include "tcp_manager.h"

void TCPSlaveMessager::read(const Packet& packet) const {
    _owner->read(packet);
}

void TCPMasterMessager::read(const Packet& packet) const {
    _owner->read(packet);
}

void TCPSlaveMessager::disconnect() const {
    _owner->disconnect();
}
