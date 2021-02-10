/*
 * @file libmerc.h
 *
 * @brief interface to mercury packet metadata capture and analysis library
 */

#ifndef LIBMERC_H
#define LIBMERC_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*
 * struct libmerc_config represents the complete configuration of
 * the libmerc library
 */
struct libmerc_config {

#ifdef __cplusplus
    // constructor, for c++ only
    libmerc_config() :
        dns_json_output{false},
        certs_json_output{false},
        metadata_output{false},
        do_analysis{false},
        report_os{false},
        output_tcp_initial_data{false},
        output_udp_initial_data{false},
        resources{NULL},
        packet_filter_cfg{NULL},
        fp_proc_threshold{0.0},
        proc_dst_threshold{0.0}
    {}
#endif

    bool dns_json_output;         /* output DNS as JSON           */
    bool certs_json_output;       /* output certificates as JSON  */
    bool metadata_output;         /* output lots of metadata      */
    bool do_analysis;             /* write analysys{} JSON object */
    bool report_os;               /* report oses in analysis JSON */
    bool output_tcp_initial_data; /* write initial data field     */
    bool output_udp_initial_data; /* write initial data field     */

    char *resources;         /* directory containing (analysis) resource files */
    char *packet_filter_cfg; /* packet filter configuration string             */

    float fp_proc_threshold;   /* remove processes with less than <var> weight    */
    float proc_dst_threshold;  /* remove destinations with less than <var> weight */
};


#ifndef __cplusplus
#define libmerc_config_init() {false,false,false,false,false,false,false,NULL,NULL,0.0,0.0}
#endif

/**
 * @brief initializes libmerc
 *
 * Initializes libmerc to use the configuration as specified with the
 * input parameters.  Returns zero on success.
 *
 * @param vars          libmerc_config
 * @param verbosity     higher values increase verbosity sent to stderr
 * @param resource_dir  directory of resource files to use in analysis
 *
 */
#ifdef __cplusplus
extern "C"
#endif
int mercury_init(const struct libmerc_config *vars, int verbosity);

/**
 * @brief finalizes libmerc
 *
 * Finalizes the libmerc library, and frees up resources allocated by
 * mercury_init().   Returns zero on success.
 *
 */
#ifdef __cplusplus
extern "C"
#endif
int mercury_finalize();

/*
 * mercury_packet_processor is an opaque pointer to a threadsafe
 * packet processor
 */
//#ifdef __cplusplus
typedef struct stateful_pkt_proc *mercury_packet_processor;
//#endif

/*
 * mercury_packet_processor_construct() allocates and initializes a
 * new mercury_packet_processor.  Returns a valid pointer on success,
 * and NULL otherwise.
 */
#ifdef __cplusplus
extern "C"
#endif
mercury_packet_processor mercury_packet_processor_construct();

/*
 * mercury_packet_processor_destruct() deallocates all resources
 * associated with a mercury_packet_processor.
 */
#ifdef __cplusplus
extern "C"
#endif
void mercury_packet_processor_destruct(mercury_packet_processor mpp);

/*
 * mercury_packet_processor_write_json() processes a packet and timestamp and
 * writes the resulting JSON into a buffer.
 *
 * processor (input) - packet processor context to be used
 * buffer (output) - location to which JSON will be written
 * buffer_size (input) - length of buffer in bytes
 * packet (input) - location of packet, starting with ethernet header
 * ts (input) - pointer to timestamp associated with packet
 */
#ifdef __cplusplus
extern "C"
#endif
size_t mercury_packet_processor_write_json(mercury_packet_processor processor,
                                           void *buffer,
                                           size_t buffer_size,
                                           uint8_t *packet,
                                           size_t length,
                                           struct timespec* ts);

/*
 * same as above, but packet points to an IP header (v4 or v6)
 */
#ifdef __cplusplus
extern "C"
#endif
size_t mercury_packet_processor_ip_write_json(mercury_packet_processor processor,
                                              void *buffer,
                                              size_t buffer_size,
                                              uint8_t *packet,
                                              size_t length,
                                              struct timespec* ts);

typedef struct analysis_result *result;

#ifdef __cplusplus
extern "C"
#endif
bool mercury_packet_processor_ip_set_analysis_result(mercury_packet_processor processor,
                                                     result res,
                                                     uint8_t *packet,
                                                     size_t length,
                                                     struct timespec* ts);

#ifdef __cplusplus
extern "C"
#endif
const struct analysis_context *mercury_packet_processor_ip_get_analysis_context(mercury_packet_processor processor,
                                                                                uint8_t *packet,
                                                                                size_t length,
                                                                                struct timespec* ts);


#ifdef __cplusplus
extern "C"
#endif
const char *result_get_process_name(const result r);

enum status {
    status_ok = 0,
    status_err = 1,
    status_err_no_more_data = 2
};
// enum status : bool { ok = 0, err = 1 };

/**
 * @brief returns the mercury license string
 *
 * Returns a printable string containing the license for mercury and
 * libmerc.
 *
 */
#ifdef __cplusplus
extern "C"
#endif
const char *mercury_get_license_string();

/**
 * @brief prints the mercury semantic version
 *
 * Prints the semantic version of mercury/libmerc to the FILE provided
 * as input.
 *
 * @param [in] file to print semantic version on.
 *
 */
#ifdef __cplusplus
extern "C"
#endif
void mercury_print_version_string(FILE *f);

// OTHER FUNCTIONS
//
enum status proto_ident_config(const char *config_string);

enum status static_data_config(const char *config_string);


#define MAX_FILENAME 256

#endif /* LIBMERC_H */
