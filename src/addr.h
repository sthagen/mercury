/*
 * addr.h
 *
 * interface into address processing functions, including longest
 * prefix matching
 */


#include <string>
#include "mercury.h"

uint32_t get_asn_info(char* dst_ip);

int addr_init(const char *resources_dir);

void addr_finalize();
