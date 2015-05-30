/******************************************************************************
 *  Copyright (c) 2015 Jamis Hoo
 *  Distributed under the MIT license 
 *  (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *  
 *  Project: Group-Share Filesystem
 *  Filename: host.h 
 *  Version: 1.0
 *  Author: Jamis Hoo
 *  E-mail: hoojamis@gmail.com
 *  Date: May 29, 2015
 *  Time: 23:16:35
 *  Description: 
 *****************************************************************************/
#ifndef HOST_H_
#define HOST_H_

#include <string>
#include <vector>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>

class Hosts {
private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int /* version */) {
        ar & _hosts;
    }
public:
    struct Host {
        friend class boost::serialization::access;
        template <class Archive>
        void serialize(Archive& ar, const unsigned int /* version */) {
            ar & id;
            ar & address;
            ar & tmpdir;
            ar & tcp_port;
            ar & ssh_port;
        }

        size_t id;
        // TODO: asio address
        std::string address;
        std::string tmpdir;
        uint16_t tcp_port;
        uint16_t ssh_port;
        // TODO: authentication information
    };

    size_t size() const { return _hosts.size(); }

private:
    std::vector<Host> _hosts;
};


#endif /* HOST_H_ */
