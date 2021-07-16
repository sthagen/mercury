/*
 * config.h
 *
 * mercury configuration structures and functions
 *
 * Copyright (c) 2021 Cisco Systems, Inc.  All rights reserved.  License at
 * https://github.com/cisco/mercury/blob/master/LICENSE
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "mercury.h"
#include "libmerc/libmerc.h"

enum status mercury_config_read_from_file(struct mercury_config &cfg,
                                          struct libmerc_config &global_vars,
                                          const char *filename);

#endif /* CONFIG_H */
