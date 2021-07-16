/*
 * udp.h
 *
 * UDP protocol processing
 *
 * Copyright (c) 2021 Cisco Systems, Inc. All rights reserved.  License at
 * https://github.com/cisco/mercury/blob/master/LICENSE
 */

#ifndef UDP_H
#define UDP_H

#include "extractor.h"

extern bool select_mdns;                    // defined in extractor.cc

struct udp_header {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__ ((__packed__));

struct udp_packet {
    const struct udp_header *header;

    udp_packet() : header{NULL} {};

    void parse(struct datum &d) {
        if (d.length() < (int)sizeof(struct udp_header)) {
            return;
        }
        header = (const struct udp_header *)d.data;
        d.skip(sizeof(struct udp_header));
    }

    void set_key(struct key &k) {
        if (header) {
            k.src_port = ntohs(header->src_port);
            k.dst_port = ntohs(header->dst_port);
            k.protocol = 17; // udp
        }
    }

    enum udp_msg_type estimate_msg_type_from_ports() {
        if (select_mdns && header && (header->src_port == htons(5353) || header->dst_port == htons(5353))) {
            return udp_msg_type_dns;
        }
        if (header && header->dst_port == htons(4789)) {
            return udp_msg_type_vxlan;
        }
        return udp_msg_type_unknown;
    }

};

enum udp_msg_type udp_get_message_type(const uint8_t *udp_data,
                                       unsigned int len);

//   From RFC 7348 (VXLAN)
//
//   #define VXLAN_PORT 4789
//
//   VXLAN Header:
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |R|R|R|R|I|R|R|R|            Reserved                           |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |                VXLAN Network Identifier (VNI) |   Reserved    |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
//

#define VXLAN_HDR_LEN 8

class vxlan : public datum {
    vxlan(datum &d) : datum{d} {
        if (datum_skip(&d, VXLAN_HDR_LEN) != status_ok) {
            d.set_empty();
        }
    }
    // note: we ignore the VXLAN Network Identifier for now, which
    // makes little difference as long as they are all identical
    //
};

#endif  // UDP_H

