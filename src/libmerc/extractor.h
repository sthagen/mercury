/*
 * extractor.h
 *
 * Copyright (c) 2019 Cisco Systems, Inc. All rights reserved.
 * License at https://github.com/cisco/mercury/blob/master/LICENSE
 */

#ifndef EXTRACTOR_H
#define EXTRACTOR_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>      /* for FILE */
#include <list>
#include "datum.h"
#include "tcp.h"
#include "proto_identify.h"



enum tcp_msg_type get_message_type(const uint8_t *tcp_data,
                                   unsigned int len);

#endif /* EXTRACTOR_H */
