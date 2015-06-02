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


        static std::string serialize(const Host& host) {
            std::ostringstream ofs;
            boost::archive::text_oarchive oa(ofs);

            oa << host;

            return ofs.str();
        }

        static Host deserialize(const std::string& byte_sequence) {
            std::istringstream ifs(byte_sequence);

            boost::archive::text_iarchive ia(ifs);

            Host host;

            ia >> host;

            return host;
        }

        uint64_t id;
        std::string address;
        std::string tmpdir;
        uint16_t tcp_port;
        // TODO: is tcp port necessary?
        uint16_t ssh_port;
        // TODO: authentication information
    };

    size_t size() const { return _hosts.size(); }

    void push(const Host& host) { _hosts.push_back(host); }
    const Host& operator[](const size_t n) const { return _hosts[n]; }
    Host& operator[](const size_t n) { return _hosts[n]; }

    static std::string serialize(const Hosts& hosts) {
        std::ostringstream ofs;
        boost::archive::text_oarchive oa(ofs);

        oa << hosts;

        return ofs.str();
    }

    static Hosts deserialize(const std::string& byte_sequence) {
        std::istringstream ifs(byte_sequence);

        boost::archive::text_iarchive ia(ifs);

        Hosts hosts;

        ia >> hosts;

        return hosts;
    }

private:
    // 0 -- undefined
    // 1 -- master's host
    // others -- other nodes' hosts
    std::vector<Host> _hosts;
};


#endif /* HOST_H_ */
