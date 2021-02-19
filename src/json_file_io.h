/*
 * json_file_io.h
 * 
 * Copyright (c) 2019 Cisco Systems, Inc. All rights reserved.  License at 
 * https://github.com/cisco/mercury/blob/master/LICENSE 
 */


#ifndef JSON_FILE_IO_H
#define JSON_FILE_IO_H

#include <stdio.h>
#include <stdint.h>
#include "mercury.h"

struct json_file {
    FILE *file;
    int64_t record_countdown;
    int64_t max_records;
    uint32_t file_num;
    char outfile_name[MAX_FILENAME];
    const char *mode;
};

void json_file_write(struct json_file *jf,
		     uint8_t *packet,
		     size_t length,
		     unsigned int sec,
		     unsigned int usec);

void json_queue_write(struct ll_queue *llq,
                      uint8_t *packet,
                      size_t length,
                      unsigned int sec,
                      unsigned int usec,
                      struct tcp_reassembler *reassembler,
                      bool blocking,
                      struct flow_table &flows,
                      struct flow_table_tcp &tcp_flows);

enum status json_file_init(struct json_file *js,
			   const char *outfile_name,
			   const char *mode,
			   uint64_t max_records);

size_t append_packet_json(void *buffer,
                          size_t buffer_size,
                          uint8_t *packet,
                          size_t length,
                          struct timespec *ts,
                          struct tcp_reassembler *reassembler,
                          struct flow_table &flows,
                          struct flow_table_tcp &tcp_flows);

#endif /* JSON_FILE_IO_H */
