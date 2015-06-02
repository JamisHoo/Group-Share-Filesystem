/******************************************************************************
 *  Copyright (c) 2015 Jamis Hoo
 *  Distributed under the MIT license 
 *  (See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT)
 *  
 *  Project: 
 *  Filename: bytes_order.h 
 *  Version: 1.0
 *  Author: Jamis Hoo
 *  E-mail: hoojamis@gmail.com
 *  Date: Jun  1, 2015
 *  Time: 23:25:19
 *  Description: 
 *****************************************************************************/
#ifndef BYTES_ORDER_H_
#define BYTES_ORDER_H_

#include <cinttypes>
#include <string>

inline void host_to_network_16(void* dst, void* from) {
    uint16_t* host = (uint16_t*)(from);
    uint8_t* network = (uint8_t*)(dst);

    network[0] = *host >> 0 & 0xff;
    network[1] = *host >> 8 & 0xff;
}

inline std::string host_to_network_16(uint16_t host) {
    std::string network(sizeof(uint16_t), 0x00);

    network[0] = host >> 0 & 0xff;
    network[1] = host >> 8 & 0xff;

    return network;
}

inline void network_to_host_16(void* dst, void* from) {
    uint16_t* host = (uint16_t*)(dst);
    uint8_t* network = (uint8_t*)(from);

    *host = network[0] | network[1] << 8;
}

inline uint16_t network_to_host_16(void* from) {
    uint16_t host;
    network_to_host_16(&host, from);
    return host;
}

inline void host_to_network_32(void* dst, void* from) {
    uint32_t* host = (uint32_t*)(from);
    uint8_t* network = (uint8_t*)(dst);

    network[0] = *host >>  0 & 0xff;
    network[1] = *host >>  8 & 0xff;
    network[2] = *host >> 16 & 0xff;
    network[3] = *host >> 24 & 0xff;
}

inline std::string host_to_network_32(uint32_t host) {
    std::string network(sizeof(uint32_t), 0x00);

    network[0] = host >> 0 & 0xff;
    network[1] = host >> 8 & 0xff;
    network[2] = host >> 16 & 0xff;
    network[3] = host >> 24 & 0xff;

    return network;
}

inline void network_to_host_32(void* dst, void* from) {
    uint32_t* host = (uint32_t*)(dst);
    uint8_t* network = (uint8_t*)(from);

    *host = network[0] <<  0 | network[1] <<  8 |
            network[2] << 16 | network[3] << 24;
}

inline uint32_t network_to_host_32(void* from) {
    uint32_t host;
    network_to_host_32(&host, from);
    return host;
}

inline void host_to_network_64(void* dst, void* from) {
    uint64_t* host = (uint64_t*)(from);
    uint8_t* network = (uint8_t*)(dst);

    network[0] = *host >>  0 & 0xff;
    network[1] = *host >>  8 & 0xff;
    network[2] = *host >> 16 & 0xff;
    network[3] = *host >> 24 & 0xff;
    network[4] = *host >> 32 & 0xff;
    network[5] = *host >> 40 & 0xff;
    network[6] = *host >> 48 & 0xff;
    network[7] = *host >> 56 & 0xff;
}

inline std::string host_to_network_64(uint64_t host) {
    std::string network(sizeof(uint64_t), 0x00);

    network[0] = host >> 0 & 0xff;
    network[1] = host >> 8 & 0xff;
    network[2] = host >> 16 & 0xff;
    network[3] = host >> 24 & 0xff;
    network[4] = host >> 32 & 0xff;
    network[5] = host >> 40 & 0xff;
    network[6] = host >> 48 & 0xff;
    network[7] = host >> 56 & 0xff;

    return network;
}

inline void network_to_host_64(void* dst, void* from) {
    uint64_t* host = (uint64_t*)(dst);
    uint8_t* network = (uint8_t*)(from);

    *host = uint64_t(network[0]) <<  0 | uint64_t(network[1]) <<  8 |
            uint64_t(network[2]) << 16 | uint64_t(network[3]) << 24 |
            uint64_t(network[4]) << 32 | uint64_t(network[5]) << 40 |
            uint64_t(network[6]) << 48 | uint64_t(network[7]) << 56;
}

inline uint64_t network_to_host_64(void* from) {
    uint64_t host;
    network_to_host_64(&host, from);
    return host;
}

#endif /* BYTES_ORDER_H_ */
