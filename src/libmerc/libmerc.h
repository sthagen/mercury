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
#include <time.h>

// defaults (if not set via ./configure)
//
#ifndef DEFAULT_RESOURCE_DIR
#define DEFAULT_RESOURCE_DIR "/usr/local/share/mercury"
#endif


// The LIBMERC_DLL_EXPORTED attribute can be applied to a function or
// variable to indicate that it should be exported from a shared
// object library even if the -fvisibility=hidden option is passed to
// the compiler; for background, see
// https://www.gnu.org/software/gnulib/manual/html_node/Exported-Symbols-of-Shared-Libraries.html
//
#define LIBMERC_DLL_EXPORTED __attribute__((__visibility__("default")))

enum enc_key_type {
    enc_key_type_none = 0,
    enc_key_type_aes_128,
    enc_key_type_aes_256
};

/**
 * @brief struct libmerc_config represents the complete configuration
 * of the libmerc library.
 *
 * To initialize libmerc, create a libmerc_config structure and pass
 * it to the mercury_init() function.  To create a libmerc_config
 * structure, you can use the #define libmerc_config_init(), which
 * represents a minimal, default configuration.
 */
struct libmerc_config {

#ifdef __cplusplus
    // constructor, for c++ only
    libmerc_config() :
        dns_json_output{false},
        certs_json_output{false},
        metadata_output{false},
        do_analysis{false},
        do_stats{false},
        report_os{false},
        output_tcp_initial_data{false},
        output_udp_initial_data{false},
        resources{NULL},
        enc_key{NULL},
        key_type{enc_key_type_none},
        packet_filter_cfg{NULL},
        fp_proc_threshold{0.0},
        proc_dst_threshold{0.0},
        max_stats_entries{0}
    {}
#endif

    bool dns_json_output;         /* output DNS as JSON           */
    bool certs_json_output;       /* output certificates as JSON  */
    bool metadata_output;         /* output lots of metadata      */
    bool do_analysis;             /* write analysys{} JSON object */
    bool do_stats;                /* gather src/fp/dst statistics */
    bool report_os;               /* report oses in analysis JSON */
    bool output_tcp_initial_data; /* write initial data field     */
    bool output_udp_initial_data; /* write initial data field     */

    char *resources;             /* archive containing resource files       */
    const uint8_t *enc_key;      /* (optional) decryption key for archive   */
    enum enc_key_type key_type;  /* key type (none=0 if key not present)    */

    char *packet_filter_cfg; /* packet filter configuration string             */

    float fp_proc_threshold;   /* remove processes with less than <var> weight    */
    float proc_dst_threshold;  /* remove destinations with less than <var> weight */
    size_t max_stats_entries;  /* max num entries in stats tables                 */
};

/**
 * libmerc_config_init() initializes a libmerc_config structure to a
 * minimal, default configuration.
 */
#ifndef __cplusplus
#define libmerc_config_init() {false,false,false,false,false,false,false,false,NULL,NULL,enc_key_type_none,NULL,0.0,0.0,0}
#endif


/**
 * mercury_context is an opaque pointer to an object that holds the
 * mercury state associated with a running instance
 */
typedef struct mercury *mercury_context;

/**
 * @brief initializes libmerc
 *
 * Initializes libmerc to use the configuration as specified with the
 * input parameters.  Returns a valid mercury_context handle on
 * success, and NULL otherwise.
 *
 * @param vars          libmerc_config
 * @param verbosity     higher values increase verbosity sent to stderr
 * @param resource_dir  directory of resource files to use in analysis
 *
 * @return a valid mercury_context handle on success, NULL on failure
 */
#ifdef __cplusplus
extern "C" LIBMERC_DLL_EXPORTED
#endif
mercury_context mercury_init(const struct libmerc_config *vars, int verbosity);

/**
 * @brief finalizes libmerc
 *
 * Finalizes the libmerc context associated with the handle, and frees
 * up resources allocated by mercury_init().  Returns zero on success.
 *
 * @return 0 on success, -1 on failure
 */
#ifdef __cplusplus
extern "C" LIBMERC_DLL_EXPORTED
#endif
int mercury_finalize(mercury_context mc);

/**
 * mercury_packet_processor is an opaque pointer to a threadsafe
 * packet processor.
 */
//#ifdef __cplusplus
typedef struct stateful_pkt_proc *mercury_packet_processor;
//#endif

/**
 * mercury_packet_processor_construct() allocates and initializes a
 * new mercury_packet_processor associated with the mercury_context
 * passed as input.
 *
 * @return a valid pointer on success, NULL otherwise.
 */
#ifdef __cplusplus
extern "C" LIBMERC_DLL_EXPORTED
#endif
mercury_packet_processor mercury_packet_processor_construct(mercury_context mc);

/**
 * mercury_packet_processor_destruct() deallocates all resources
 * associated with a mercury_packet_processor.
 */
#ifdef __cplusplus
extern "C" LIBMERC_DLL_EXPORTED
#endif
void mercury_packet_processor_destruct(mercury_packet_processor mpp);

/**
 * mercury_packet_processor_write_json() processes a packet and timestamp and
 * writes the resulting JSON into a buffer.
 *
 * @param processor (input) is a packet processor context to be used
 * @param buffer (output) - location to which JSON will be written
 * @param buffer_size (input) - length of buffer in bytes
 * @param packet (input) - location of packet, starting with ethernet header
 * @param ts (input) - pointer to timestamp associated with packet
 *
 * @return the number of bytes of JSON output written.
 */
#ifdef __cplusplus
extern "C" LIBMERC_DLL_EXPORTED
#endif
size_t mercury_packet_processor_write_json(mercury_packet_processor processor,
                                           void *buffer,
                                           size_t buffer_size,
                                           uint8_t *packet,
                                           size_t length,
                                           struct timespec* ts);

/**
 * mercury_packet_processor_ip_write_json() processes a packet and
 * timestamp and writes the resulting JSON into a buffer.
 *
 * @param processor (input) is a packet processor context to be used
 * @param buffer (output) - location to which JSON will be written
 * @param buffer_size (input) - length of buffer in bytes
 * @param packet (input) - location of packet, starting with IPv4 or IPv6 header
 * @param ts (input) - pointer to timestamp associated with packet
 *
 * @return the number of bytes of JSON output written.
 */
#ifdef __cplusplus
extern "C" LIBMERC_DLL_EXPORTED
#endif
size_t mercury_packet_processor_ip_write_json(mercury_packet_processor processor,
                                              void *buffer,
                                              size_t buffer_size,
                                              uint8_t *packet,
                                              size_t length,
                                              struct timespec* ts);
/**
 * enum fingerprint_status represents the status of a fingerprint
 * relative to the library's knowledge about fingerprints, based on
 * the data in its resources and the other fingerprints that it has
 * observed.
 */
enum fingerprint_status {
    fingerprint_status_no_info_available = 0,  /**< fingerprint status is unknown                */
    fingerprint_status_labeled           = 1,  /**< fingerprint is in FPDB                       */
    fingerprint_status_randomized        = 2,  /**< fingerprint is in randomized FP set          */
    fingerprint_status_unlabled          = 3   /**< fingerprint is not in FPDB or randomized set */
};

/**
 * mercury_packet_processor_ip_get_analysis_context() processes an IP
 * packet and timestamp and returns a pointer to an analysis context
 * if a fingerprint was found in the packet, and returns nothing
 * otherwise.
 *
 * @param processor (input) is a packet processor context to be used
 * @param buffer_size (input) - length of buffer in bytes
 * @param packet (input) - location of packet, starting with IPv4 or IPv6 header
 * @param ts (input) - pointer to timestamp associated with packet
 *
 * @return a pointer to an analysis_context, if a fingerprint was
 * found, otherwise NULL.
 */
#ifdef __cplusplus
extern "C" LIBMERC_DLL_EXPORTED
#endif
const struct analysis_context *mercury_packet_processor_ip_get_analysis_context(mercury_packet_processor processor,
                                                                                uint8_t *packet,
                                                                                size_t length,
                                                                                struct timespec* ts);

/**
 * mercury_packet_processor_get_analysis_context() processes an
 * ethernet packet and timestamp and returns a pointer to an analysis
 * context if a fingerprint was found in the packet, and returns
 * nothing otherwise.
 *
 * @param processor (input) is a packet processor context to be used
 * @param buffer_size (input) - length of buffer in bytes
 * @param packet (input) - location of packet, starting with the ethernet header
 * @param ts (input) - pointer to timestamp associated with packet
 *
 * @return a pointer to an analysis_context, if a fingerprint was
 * found, otherwise NULL.
 */
#ifdef __cplusplus
extern "C" LIBMERC_DLL_EXPORTED
#endif
const struct analysis_context *mercury_packet_processor_get_analysis_context(mercury_packet_processor processor,
                                                                             uint8_t *packet,
                                                                             size_t length,
                                                                             struct timespec* ts);

/**
 * analysis_context_get_fingerprint_status() returns the fingerprint_status
 * associated with an analysis_context.
 *
 * @param ac (input) is an analysis_context pointer.
 *
 * @return a fingerprint_status enumeration.
 */
#ifdef __cplusplus
extern "C" LIBMERC_DLL_EXPORTED
#endif
enum fingerprint_status analysis_context_get_fingerprint_status(const struct analysis_context *ac);

/**
 * enum fingerprint_type identifies a type of fingerprint for the
 * struct fingerprint.
 */
enum fingerprint_type {
     fingerprint_type_unknown = 0,     /**< The fingerprint type is not known. */
     fingerprint_type_tls = 1,         /**< TLS client fingerprint             */
     fingerprint_type_tls_server = 2,  /**< TLS server fingerprint             */
     fingerprint_type_http = 3,        /**< HTTP client fingerprint            */
     fingerprint_type_http_server = 4, /**< HTTP server fingerprint            */
     fingerprint_type_ssh = 5,         /**< SSH init fingerprint               */
     fingerprint_type_ssh_kex = 6,     /**< SSH kex fingerprint                */
     fingerprint_type_tcp = 7,         /**< TCP SYN fingerprint                */
     fingerprint_type_dhcp = 8,        /**< DHCP client fingerprint            */
     fingerprint_type_smtp_server = 9, /**< SMTP server fingerprint            */
};

/**
 * analysis_context_get_fingerprint_type() returns the fingerprint_status
 * associated with an analysis_context.
 *
 * @param ac (input) is an analysis_context pointer.
 *
 * @return a fingerprint_type enumeration.
 */
#ifdef __cplusplus
extern "C" LIBMERC_DLL_EXPORTED
#endif
enum fingerprint_type analysis_context_get_fingerprint_type(const struct analysis_context *ac);

/**
 * analysis_context_get_fingerprint_string() returns the printable,
 * null-terminated string for the fingerprint associated with an
 * analysis_context, if there is one.
 *
 * @param ac (input) is an analysis_context pointer.
 *
 * @return a null-terminated, printable character string, if a
 * fingerprint was found by the library; otherwise, NULL.
 */
#ifdef __cplusplus
extern "C" LIBMERC_DLL_EXPORTED
#endif
const char *analysis_context_get_fingerprint_string(const struct analysis_context *ac);

/**
 * analysis_context_get_server_name() returns the printable,
 * null-terminated string for the TLS client hello server name
 * associated with an analysis_context, if there is one.
 *
 * @param ac (input) is an analysis_context pointer.
 *
 * @return a null-terminated, printable character string, if a TLS
 * client hello server name was found by the library; otherwise, NULL.
 */
#ifdef __cplusplus
extern "C" LIBMERC_DLL_EXPORTED
#endif
const char *analysis_context_get_server_name(const struct analysis_context *ac);


/**
 * analysis_context_get_process_info() writes the probable process and
 * its corresdponing probability score into the locations provided,
 * given an analysis_context.
 *
 * @param ac (input) is an analysis_context pointer.
 *
 * @param probable_process (output) is the location to write the
 * probable_process string.
 *
 * @param probability_score (output) is the location to write the
 * probability score.
 *
 * @return true if the probable_process and probabiltiy_score are
 * valid after the function returns, and false otherwise.
 */
#ifdef __cplusplus
extern "C" LIBMERC_DLL_EXPORTED
#endif
bool analysis_context_get_process_info(const struct analysis_context *ac, // input
                                       const char **probable_process,     // output
                                       double *probability_score          // output
                                       );

/**
 * analysis_context_get_malware_info() writes the
 * probable_process_is_malware boolean and the probability_malware
 * value into the locations provided, for a given analysis_context.
 *
 * @param ac (input) is an analysis_context pointer.
 *
 * @param probable_process_is_malware (output) is the location to write the
 * boolean.
 *
 * @param probability_malware (output) is the location to write the
 * probability that the process is malware.
 *
 * @return true if the probable_process_is_malware and
 * probabiltiy_malware values are valid after the function returns,
 * and false otherwise.
 */
#ifdef __cplusplus
extern "C" LIBMERC_DLL_EXPORTED
#endif
bool analysis_context_get_malware_info(const struct analysis_context *ac, // input
                                       bool *probable_process_is_malware, // output
                                       double *probability_malware        // output
                                       );

/**
 * os_information holds the name of an operating system and the
 * prevalence with which it has been observed with a particular
 * fingerprint.
 */
struct os_information {
    char *os_name;           /**< printable, null-termated string holding OS name */
    uint64_t os_prevalence;  /**< prevalence with which this OS is associated with fingerprint */
};

/**
 * analysis_context_get_os_info() sets a pointer to an array of
 * os_information structures and the length of that array, for a given
 * analysis_context.
 *
 * @param ac (input) is an analysis_context pointer.
 *
 * @param os_info (output) is the location to which the os_information
 * array pointer will be written.
 *
 * @param os_info_len (output) is the location to write the
 * length of the os_info array.
 *
 * @return true if the os_info and os_info_len locations point to
 * valid data after the function returns, and false otherwise.
 */
#ifdef __cplusplus
extern "C" LIBMERC_DLL_EXPORTED
#endif
bool analysis_context_get_os_info(const struct analysis_context *ac,     // input
                                  const struct os_information **os_info, // output
                                  size_t *os_info_len                    // output
                                  );


/**
 * mercury_get_stats_data()
 *
 * @param data_ptr (output) is a pointer to a pointer to a buffer that
 * contains the stats data; that is, the function sets the pointer at
 * the location provided as input to be that buffer.  The pointer MAY
 * be NULL, if no data was gathered or if some other error occured.
 *
 * This function may process a lot of data, and it may take a very
 * long time to return, so the caller MUST be prepared to wait for
 * seconds or minutes.
 *
 * This function SHOULD be called periodically, e.g. every hour or
 * every day.  Mercury's stats engine accumulates data between calls
 * to this function, and each call flushes all of the data maintained
 * by that engine.  The stats engine uses a large but fixed amount of
 * RAM for data storage; if it runs out of storage, it will stop
 * accumulating data.
 *
 * @param num_bytes_ptr (output) is a pointer to a size_t that
 * contains the number of bytes of stats data in the data buffer; that
 * is, the function sets the size_t at the location provided as input
 * to be the size of that buffer.  The value of num_bytes MAY be zero,
 * if no data was gathered or if some other error occured.
 *
 * @return true on success, false otherwise.  In particular, if either
 * data_ptr or num_bytes_ptr are NULL, then false will be returned; in
 * the latter case, you SHOULD NOT dereference either data_ptr or
 * num_bytes_ptr.
 */
#ifdef __cplusplus
extern "C" LIBMERC_DLL_EXPORTED
#endif
bool mercury_get_stats_data(void **data_ptr, size_t *num_bytes_ptr);


/**
 * mercury_write_stats_data()
 *
 * @param mercury_context is the context associated with the stats
 * data to be written out.
 *
 * @param stats_data_file_path (input) is a pointer to an ASCII
 * character string holding the path to the file to which stats data
 * is to be written.
 *
 * This function may process a lot of data, and it may take a very
 * long time to return, so the caller MUST be prepared to wait for
 * seconds or minutes.
 *
 * This function SHOULD be called periodically, e.g. every hour or
 * every day.  Mercury's stats engine accumulates data between calls
 * to this function, and each call flushes all of the data maintained
 * by that engine.  The stats engine uses a large but fixed amount of
 * RAM for data storage; if it runs out of storage, it will stop
 * accumulating data.
 *
 * @return true on success, false otherwise.
 */
#ifdef __cplusplus
extern "C" LIBMERC_DLL_EXPORTED
#endif
bool mercury_write_stats_data(mercury_context mc, const char *stats_data_file_path);


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
extern "C" LIBMERC_DLL_EXPORTED
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
extern "C" LIBMERC_DLL_EXPORTED
#endif
void mercury_print_version_string(FILE *f);

/**
 * @brief returns the mercury semantic version
 *
 * Returns the semantic version of mercury/libmerc as a uint32_t, in
 * which the major version is the most significant byte, the minor
 * version is the second most significant byte, the patchlevel is the
 * third most significant byte, and the least significant byte may be
 * zero.  That is, the format looks like
 *
 *    major | minor | patchlevel | 0
 *
 * @return an unsigned integer that encodes the semantic version
 *
 */
#ifdef __cplusplus
extern "C" LIBMERC_DLL_EXPORTED
#endif
uint32_t mercury_get_version_number();

/**
 * @brief returns the resource archive VERSION
 *
 * Returns a pointer to a NULL-terminated string containing the
 * entirety of the VERSION file in the resource archive provided to
 * the mercury library.  If there was no VERSION file in the archive,
 * then a zero-length string will be returned.
 *
 * @warning this function should only be called after libmerc is
 * initialized, and it should not be called after it is
 * de-initialized.
 *
 * @return a pointer to a null-terminated string containing the
 * resource archive VERSION file
 *
 */
#ifdef __cplusplus
extern "C" LIBMERC_DLL_EXPORTED
#endif
const char *mercury_get_resource_version(mercury_context mc);

// OTHER FUNCTIONS
//
enum status proto_ident_config(const char *config_string);

enum status static_data_config(const char *config_string);


#endif /* LIBMERC_H */
