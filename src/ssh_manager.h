/******************************************************************************
 *  Copyright (c) 2015 Jamis Hoo
 *  Distributed under the MIT license 
 *  (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *  
 *  Project: Group-Share Filesystem
 *  Filename: ssh_manager.h 
 *  Version: 1.0
 *  Author: Jamis Hoo
 *  E-mail: hoojamis@gmail.com
 *  Date: Jun  4, 2015
 *  Time: 10:46:58
 *  Description: 
 *****************************************************************************/
#ifndef SSH_MANAGER_H_
#define SSH_MANAGER_H_

#include <unordered_map>
#include "libssh_wrapper.h"


class SSHManager {
public:
    ~SSHManager() {
        for (auto i: _connections)
            delete i.second;
    }

    // returns true if found
    bool findHost(const uint64_t id) const {
        return _connections.find(id) != _connections.end();
    }

    int insertHost(const uint64_t id, const std::string& addr, const uint16_t port) {
        return !_connections.emplace(id, new SSHSession(addr, port)).second;
    }

    int removeHost(const uint64_t id) {
        return !_connections.erase(id);
    }

    intmax_t read(const uint64_t id, const std::string& path, const size_t offset,
                  const size_t size, char* buff) const {
        auto ite = _connections.find(id);
        if (ite == _connections.end()) return -1;

        return ite->second->read(path, offset, size, buff);
    }

private:
    std::unordered_map<uint64_t, SSHSession*> _connections;
};


#endif /* SSH_MANAGER_H_ */
