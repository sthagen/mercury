/*
 * analysis.h
 *
 * Copyright (c) 2019 Cisco Systems, Inc. All rights reserved.
 * License at https://github.com/cisco/mercury/blob/master/LICENSE
 */

#ifndef ANALYSIS_H
#define ANALYSIS_H

#include <stdio.h>
#include <math.h>
#include <algorithm>
#include "packet.h"
#include "addr.h"
#include "json_object.h"
#include "result.h"
#include "stats.h"

#include <mutex>
#include <shared_mutex>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <zlib.h>
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "tls.h"
#include "archive.h"

// TBD - move flow_key_sprintf_src_addr() to the right file
//
void flow_key_sprintf_src_addr(const struct flow_key *key,
                               char *src_addr_str);

struct classifier *analysis_init_from_archive(int verbosity,
                               const char *archive_name,
                               const uint8_t *enc_key,
                               enum enc_key_type key_type,
                               float fp_proc_threshold,
                               float proc_dst_threshold,
                               bool report_os);

int analysis_finalize(classifier *c);


struct analysis_result analyze_client_hello_and_key(const struct tls_client_hello &hello,
                                                    const struct key &key);


// process and malware classifier classes
//

class process_info {
public:
    std::string name;
    bool malware;
    uint64_t count;
    std::unordered_map<uint32_t, uint64_t>    ip_as;
    std::unordered_map<std::string, uint64_t> hostname_domains;
    std::unordered_map<uint16_t, uint64_t>    portname_applications;
    std::unordered_map<std::string, uint64_t> ip_ip;
    std::unordered_map<std::string, uint64_t> hostname_sni;
    std::map<std::string, uint64_t> os_info;
    bool extended_fp_metadata = false;

    process_info(const std::string &proc_name,
                 bool is_malware,
                 uint64_t proc_count,
                 const std::unordered_map<uint32_t, uint64_t> &as,
                 const std::unordered_map<std::string, uint64_t> &domains,
                 const std::unordered_map<uint16_t, uint64_t> &ports,
                 const std::unordered_map<std::string, uint64_t> &ip,
                 const std::unordered_map<std::string, uint64_t> &sni,
                 const std::map<std::string, uint64_t> &oses) :
        name{proc_name},
        malware{is_malware},
        count{proc_count},
        ip_as{as},
        hostname_domains{domains},
        portname_applications{ports},
        ip_ip{ip},
        hostname_sni{sni},
        os_info{oses} {
            if (!ip.empty() && !sni.empty()) {
                extended_fp_metadata = true;
            }
        }

    void print(FILE *f) {
        fprintf(f, "{\"process\":\"%s\"", name.c_str());
        fprintf(f, ",\"count\":\"%" PRIu64 "\"", count);
        fprintf(f, ",\"classes_ip_as\":{");
        char comma = ' ';
        for (auto &x : ip_as) {
            fprintf(f, "%c\"%u\":%" PRIu64, comma, x.first, x.second);
            comma = ',';
        }
        fprintf(f, "}");
        fprintf(f, ",\"classes_hostname_domains\":{");
        comma = ' ';
        for (auto &x : hostname_domains) {
            fprintf(f, "%c\"%s\":%" PRIu64, comma, x.first.c_str(), x.second);
            comma = ',';
        }
        fprintf(f, "}");
        fprintf(f, ",\"classes_port_applications\":{");
        comma = ' ';
        for (auto &x : portname_applications) {
            fprintf(f, "%c\"%u\":%" PRIu64, comma, x.first, x.second);
            comma = ',';
        }
        fprintf(f, "}");

        if (!ip_ip.empty() && !hostname_sni.empty()) {
            fprintf(f, ",\"classes_ip_ip\":{");
            comma = ' ';
            for (auto &x : ip_ip) {
                fprintf(f, "%c\"%s\":%" PRIu64, comma, x.first.c_str(), x.second);
                comma = ',';
            }
            fprintf(f, "}");

            fprintf(f, ",\"classes_hostname_sni\":{");
            comma = ' ';
            for (auto &x : hostname_sni) {
                fprintf(f, "%c\"%s\":%" PRIu64, comma, x.first.c_str(), x.second);
                comma = ',';
            }
            fprintf(f, "}");
        }

        // TBD: print malware data

        fprintf(f, "}");
    }
};

// data type used in floating point computations
//
using floating_point_type = long double;

// an instance of class update represents an update to a prior
// probability
//
class update {
public:
    update(unsigned int i, floating_point_type v) : index{i}, value{v} {}
    unsigned int index;  // index of probability to update
    floating_point_type value;   // value of update
};

class fingerprint_data {
    std::vector<std::string> process_name;
    std::vector<floating_point_type> process_prob;
    std::vector<bool>        malware;
    std::unordered_map<uint32_t, std::vector<class update>> as_number_updates;
    std::unordered_map<uint16_t, std::vector<class update>> port_updates;
    std::unordered_map<std::string, std::vector<class update>> hostname_domain_updates;
    std::unordered_map<std::string, std::vector<class update>> ip_ip_updates;
    std::unordered_map<std::string, std::vector<class update>> hostname_sni_updates;
    //std::vector<std::pair<os_information*,uint16_t>> os_info; // TODO: delete
    std::vector<std::vector<struct os_information>> process_os_info_vector;
    //std::vector<std::vector<std::string>> process_os_names;
    floating_point_type base_prior;

    static bool malware_db;
    static bool report_os;

    const subnet_data *subnet_data_ptr = nullptr;

public:
    uint64_t total_count;

    fingerprint_data() : total_count{0} { }

    fingerprint_data(uint64_t count,
                     const std::vector<class process_info> &processes,
                     ptr_dict &os_dictionary,
                     const subnet_data *subnets) :
        subnet_data_ptr{subnets},
        total_count{count} {

            //fprintf(stderr, "compiling fingerprint_data for %lu processes\n", processes.size());

            // initialize data structures
            //
            process_name.reserve(processes.size());
            process_prob.reserve(processes.size());
            malware.reserve(processes.size());

            base_prior = log(1.0 / total_count);
            size_t index = 0;
            for (const auto &p : processes) {
                process_name.push_back(p.name);
                malware.push_back(p.malware);
                if (p.malware) {
                    malware_db = true;
                }

                if (p.os_info.size() > 0) {

                    // create a vector of os_information structs, whose char * makes
                    // use of the os_dictionary
                    //
                    process_os_info_vector.push_back(std::vector<struct os_information>{});
                    std::vector<struct os_information> &os_info_vector = process_os_info_vector.back();
                    for (const auto &os_and_count : p.os_info) {
                        const char *os = os_dictionary.get(os_and_count.first);
                        struct os_information tmp{(char *)os, os_and_count.second};
                        os_info_vector.push_back(tmp);
                    }
                }

                constexpr floating_point_type as_weight = 0.13924;
                constexpr floating_point_type domain_weight = 0.15590;
                constexpr floating_point_type port_weight = 0.00528;
                constexpr floating_point_type ip_weight = 0.56735;
                constexpr floating_point_type sni_weight = 0.96941;

                //fprintf(stderr, "compiling process \"%s\"\n", p.name.c_str());

                floating_point_type proc_prior = log(.1);
                floating_point_type prob_process_given_fp = (floating_point_type)p.count / total_count;
                floating_point_type score = log(prob_process_given_fp);
                process_prob.push_back(fmax(score, proc_prior) + base_prior * (as_weight + domain_weight + port_weight + ip_weight + sni_weight));

                for (const auto &as_and_count : p.ip_as) {
                    const auto x = as_number_updates.find(as_and_count.first);
                    class update u{ index, (log((floating_point_type)as_and_count.second / total_count) - base_prior ) * as_weight };
                    if (x != as_number_updates.end()) {
                        x->second.push_back(u);
                    } else {
                        as_number_updates[as_and_count.first] = { u };
                    }
                }
                for (const auto &domains_and_count : p.hostname_domains) {
                    const auto x = hostname_domain_updates.find(domains_and_count.first);
                    class update u{ index, (log((floating_point_type)domains_and_count.second / total_count) - base_prior) * domain_weight };
                    if (x != hostname_domain_updates.end()) {
                        x->second.push_back(u);
                    } else {
                        hostname_domain_updates[domains_and_count.first] = { u };
                    }
                }
                for (const auto &port_and_count : p.portname_applications) {
                    const auto x = port_updates.find(port_and_count.first);
                    class update u{ index, (log((floating_point_type)port_and_count.second / total_count) - base_prior) * port_weight };
                    if (x != port_updates.end()) {
                        x->second.push_back(u);
                    } else {
                        port_updates[port_and_count.first] = { u };
                    }
                }
                for (const auto &ip_and_count : p.ip_ip) {
                    const auto x = ip_ip_updates.find(ip_and_count.first);
                    class update u{ index, (log((floating_point_type)ip_and_count.second / total_count) - base_prior) * ip_weight };
                    if (x != ip_ip_updates.end()) {
                        x->second.push_back(u);
                    } else {
                        ip_ip_updates[ip_and_count.first] = { u };
                    }
                }
                for (const auto &sni_and_count : p.hostname_sni) {
                    const auto x = hostname_sni_updates.find(sni_and_count.first);
                    class update u{ index, (log((floating_point_type)sni_and_count.second / total_count) - base_prior) * sni_weight };
                    if (x != hostname_sni_updates.end()) {
                        x->second.push_back(u);
                    } else {
                        hostname_sni_updates[sni_and_count.first] = { u };
                    }
                }

                ++index;
            }

    }

    ~fingerprint_data() {
    }

    void print(FILE *f) {
        fprintf(f, ",\"total_count\":%" PRIu64, total_count);
        fprintf(f, ",\"process_info\":[");

        // TBD: fingerprint_data::print() output should be a JSON representation of object

        for (size_t i=0; i < process_name.size(); i++) {
            fprintf(stderr, "process: %s\tprob: %Le\n", process_name[i].c_str(), process_prob[i]);
        }
        fprintf(stderr, "as_number_updates:\n");
        for (const auto &asn_and_updates : as_number_updates) {
            fprintf(stderr, "\t%u:\n", asn_and_updates.first);
            for (const auto &update : asn_and_updates.second) {
                fprintf(stderr, "\t\t{ %u, %Le }\n", update.index, update.value);
            }
        }
        //std::unordered_map<std::string, std::vector<class update>> hostname_domain_updates;
        fprintf(stderr, "hostname_domain_updates:\n");
        for (const auto &domain_and_updates : hostname_domain_updates) {
            fprintf(stderr, "\t%s:\n", domain_and_updates.first.c_str());
            for (const auto &update : domain_and_updates.second) {
                fprintf(stderr, "\t\t{ %u, %Le }\n", update.index, update.value);
            }
        }
        fprintf(stderr, "port_updates:\n");
        for (const auto &port_and_updates : port_updates) {
            fprintf(stderr, "\t%u:\n", port_and_updates.first);
            for (const auto &update : port_and_updates.second) {
                fprintf(stderr, "\t\t{ %u, %Le }\n", update.index, update.value);
            }
        }
        fprintf(f, "]");
    }

    // get_tld_domain_name() returns the string containing the top two
    // domains of the input string; that is, given "s3.amazonaws.com",
    // it returns "amazonaws.com".  If there is only one name, it is
    // returned.
    //
    std::string get_tld_domain_name(const char* server_name) {

        const char *separator = NULL;
        const char *previous_separator = NULL;
        const char *c = server_name;
        while (*c) {
            if (*c == '.') {
                if (separator) {
                    previous_separator = separator;
                }
                separator = c;
            }
            c++;
        }
        if (previous_separator) {
            previous_separator++;  // increment past '.'
            return previous_separator;
        }
        return server_name;
    }

    struct analysis_result perform_analysis(const char *server_name, const char *dst_ip, uint16_t dst_port) {
        uint32_t asn_int = subnet_data_ptr->get_asn_info(dst_ip);
        uint16_t port_app = remap_port(dst_port);
        std::string domain = get_tld_domain_name(server_name);
        std::string server_name_str(server_name);
        std::string dst_ip_str(dst_ip);

        std::vector<floating_point_type> process_score = process_prob;  // working copy of probability vector

        auto asn_update = as_number_updates.find(asn_int);
        if (asn_update != as_number_updates.end()) {
            for (const auto &x : asn_update->second) {
                process_score[x.index] += x.value;
            }
        }
        auto port_update = port_updates.find(port_app);
        if (port_update != port_updates.end()) {
            for (const auto &x : port_update->second) {
                process_score[x.index] += x.value;
            }
        }
        auto hostname_domain_update = hostname_domain_updates.find(domain);
        if (hostname_domain_update != hostname_domain_updates.end()) {
            for (const auto &x : hostname_domain_update->second) {
                process_score[x.index] += x.value;
            }
        }
        auto ip_ip_update = ip_ip_updates.find(dst_ip_str);
        if (ip_ip_update != ip_ip_updates.end()) {
            for (const auto &x : ip_ip_update->second) {
                process_score[x.index] += x.value;
            }
        }
        auto hostname_sni_update = hostname_sni_updates.find(server_name_str);
        if (hostname_sni_update != hostname_sni_updates.end()) {
            for (const auto &x : hostname_sni_update->second) {
                process_score[x.index] += x.value;
            }
        }

        floating_point_type max_score = std::numeric_limits<floating_point_type>::lowest();
        floating_point_type sec_score = std::numeric_limits<floating_point_type>::lowest();
        uint64_t index_max = 0;
        uint64_t index_sec = 0;
        for (uint64_t i=0; i < process_score.size(); i++) {
            if (process_score[i] > max_score) {
                sec_score = max_score;
                index_sec = index_max;
                max_score = process_score[i];
                index_max = i;
            } else if (process_score[i] > sec_score) {
                sec_score = process_score[i];
                index_sec = i;
            }
        }

        floating_point_type score_sum = 0.0;
        floating_point_type malware_prob = 0.0;
        for (uint64_t i=0; i < process_score.size(); i++) {
            process_score[i] = exp((float)process_score[i]);
            score_sum += process_score[i];
            if (malware[i]) {
                malware_prob += process_score[i];
            }
        }
        max_score = process_score[index_max];
        sec_score = process_score[index_sec];

        if (malware_db && process_name[index_max] == "generic dmz process" && malware[index_sec] == false) {
            // the most probable process is unlabeled, so choose the
            // next most probable one if it isn't malware, and adjust
            // the normalization sum as appropriate

            index_max = index_sec;
            score_sum -= max_score;
            max_score = sec_score;
        }
        if (score_sum > 0.0) {
            max_score /= score_sum;
            if (malware_db) {
                malware_prob /= score_sum;
            }
        }

        // set os_info (to NULL if unavailable)
        //
        os_information *os_info_data = NULL;
        uint16_t os_info_size = 0;
        if (process_os_info_vector.size() > 0) {
            os_info_data = process_os_info_vector[index_max].data();
            os_info_size = process_os_info_vector[index_max].size();
        }
        if (malware_db) {
            return analysis_result(process_name[index_max].c_str(), max_score, os_info_data, os_info_size,
                                   malware[index_max], malware_prob);
        }
        return analysis_result(process_name[index_max].c_str(), max_score, os_info_data, os_info_size);
    }

    static uint16_t remap_port(uint16_t dst_port) {
        std::unordered_map<uint16_t, uint16_t> port_remapping =
            {
             { 443, 443 },   // https
             { 448, 448 },   // database
             { 465, 465 },   // email
             { 563, 563 },   // nntp
             { 585, 465 },   // email
             { 614, 614 },   // shell
             { 636, 636 },   // ldap
             { 989, 989 },   // ftp
             { 990, 989 },   // ftp
             { 991, 991 },   // nas
             { 992, 992 },   // telnet
             { 993, 465 },   // email
             { 994, 994 },   // irc
             { 995, 465 },   // email
             { 1443, 1443 }, // alt-https
             { 2376, 2376 }, // docker
             { 8001, 8001 }, // tor
             { 8443, 1443 }, // alt-https
             { 9000, 8001 }, // tor
             { 9001, 8001 }, // tor
             { 9002, 8001 }, // tor
             { 9101, 8001 }, // tor
            };

        const auto port_it = port_remapping.find(dst_port);
        if (port_it != port_remapping.end()) {
            return port_it->second;
        }
        return 0;  // unknown
    }


};

// static const char* kTypeNames[] = { "Null", "False", "True", "Object", "Array", "String", "Number" };
// fprintf(stderr, "Type of member %s is %s\n", "str_repr", kTypeNames[fp["str_repr"].GetType()]);


class fingerprint_prevalence {
public:
    fingerprint_prevalence(uint32_t max_cache_size) : mutex_{}, list_{}, set_{}, known_set_{}, max_cache_size_{max_cache_size} {}

    // first check if known fingerprints contains fingerprint, then check adaptive set
    bool contains(std::string fp_str) const {
        if (known_set_.find(fp_str) != known_set_.end()) {
            return true;
        }

        std::shared_lock lock(mutex_);
        if (set_.find(fp_str) != set_.end()) {
            return true;
        }
        return false;
    }

    // seed known set of fingerprints
    void initial_add(std::string fp_str) {
        known_set_.insert(fp_str);
    }

    // update fingerprint LRU cache if needed
    void update(std::string fp_str) {
        if (known_set_.find(fp_str) != known_set_.end()) {
            return ;
        }

        std::unique_lock lock(mutex_);

        if (set_.find(fp_str) != set_.end()) {
            list_.remove(fp_str);
            list_.push_back(fp_str);
        } else {
            list_.push_back(fp_str);
            set_.insert(fp_str);
        }

        if (set_.size() > max_cache_size_) {
            set_.erase(list_.front());
            list_.pop_front();
        }
    }

    void print(FILE *f) {
        for (auto &entry : known_set_) {
            fprintf(f, "%s\n", entry.c_str());
        }
    }

private:
    mutable std::shared_mutex mutex_;
    std::list<std::string> list_;
    std::unordered_set<std::string> set_;
    std::unordered_set<std::string> known_set_;
    uint32_t max_cache_size_;
};


class classifier {
    bool MALWARE_DB = false;
    bool EXTENDED_FP_METADATA = false;

    ptr_dict os_dictionary;  // used to hold/compact OS CPE strings

    subnet_data subnets;     // holds ASN/subnet information

    std::unordered_map<std::string, class fingerprint_data> fpdb;
    fingerprint_prevalence fp_prevalence{100000};

    std::string resource_version;  // as reported by VERSION file in resource archive

public:

    void process_fp_prevalence_line(std::string &line_str) {
        if (!line_str.empty() && line_str[line_str.length()-1] == '\n') {
            line_str.erase(line_str.length()-1);
        }
        //fprintf(stderr, "loading fp_prevalence_line '%s'\n", line_str.c_str());
        fp_prevalence.initial_add(line_str);
    }

    void process_fp_db_line(std::string &line_str, float fp_proc_threshold, float proc_dst_threshold, bool report_os) {
        rapidjson::Document fp;
        fp.Parse(line_str.c_str());

        std::string fp_string;
        if (fp.HasMember("str_repr") && fp["str_repr"].IsString()) {
            fp_string = fp["str_repr"].GetString();
            //fprintf(stderr, "%s\n", fp_string.c_str());
        }

        uint64_t total_count = 0;
        if (fp.HasMember("total_count") && fp["total_count"].IsUint64()) {
            total_count = fp["total_count"].GetUint64();
        }

        std::vector<class process_info> process_vector;

        if (fp.HasMember("process_info") && fp["process_info"].IsArray()) {
            //fprintf(stderr, "process_info[]\n");

            unsigned int process_number = 0;
            for (auto &x : fp["process_info"].GetArray()) {
                uint64_t count = 0;
                bool malware = false;

                if (x.HasMember("count") && x["count"].IsUint64()) {
                    count = x["count"].GetUint64();
                    //fprintf(stderr, "\tcount: %lu\n", x["count"].GetUint64());
                }
                if (x.HasMember("malware") && x["malware"].IsBool()) {
                    if (MALWARE_DB == false && process_number > 1) {
                        throw "error: malware data expected, but not present";
                    }
                    MALWARE_DB = true;
                    malware = x["malware"].GetBool();
                }
                /* do not load process into memory if prevalence is below threshold */
                if ((process_number > 1) && ((float)count/total_count < fp_proc_threshold) && (malware != true)) {
                    continue;
                }

                process_number++;
                //fprintf(stderr, "%s\n", "process_info");

                std::unordered_map<uint32_t, uint64_t>    ip_as;
                std::unordered_map<std::string, uint64_t> hostname_domains;
                std::unordered_map<uint16_t, uint64_t>    portname_applications;
                std::unordered_map<std::string, uint64_t> ip_ip;
                std::unordered_map<std::string, uint64_t> hostname_sni;
                std::map<std::string, uint64_t> os_info;

                std::string name;
                if (x.HasMember("process") && x["process"].IsString()) {
                    name = x["process"].GetString();
                    //fprintf(stderr, "\tname: %s\n", x["process"].GetString());
                }
                if (x.HasMember("classes_hostname_domains") && x["classes_hostname_domains"].IsObject()) {
                    //fprintf(stderr, "\tclasses_hostname_domains\n");
                    for (auto &y : x["classes_hostname_domains"].GetObject()) {
                        if (y.value.IsUint64() && ((float)y.value.GetUint64()/count > proc_dst_threshold)) {
                            //fprintf(stderr, "\t\t%s: %lu\n", y.name.GetString(), y.value.GetUint64());
                            hostname_domains[y.name.GetString()] = y.value.GetUint64();
                        }
                    }
                }
                if (x.HasMember("classes_ip_as") && x["classes_ip_as"].IsObject()) {
                    //fprintf(stderr, "\tclasses_ip_as\n");
                    for (auto &y : x["classes_ip_as"].GetObject()) {
                        if (y.value.IsUint64() && ((float)y.value.GetUint64()/count > proc_dst_threshold)) {
                            //fprintf(stderr, "\t\t%s: %lu\n", y.name.GetString(), y.value.GetUint64());

                            if (strcmp(y.name.GetString(), "unknown") != 0) {

                                errno = 0;
                                unsigned long as_number = strtol(y.name.GetString(), NULL, 10);
                                if (errno) {
                                    as_number = 0; // "unknown"
                                    fprintf(stderr, "note: found string \"%s\" in ip_as\n", y.name.GetString());
                                }
                                if (as_number > 0xffffffff) {
                                    throw "error: as number too high";
                                }
                                ip_as[as_number] = y.value.GetUint64();

                            }

                        }
                    }
                }
                if (x.HasMember("classes_port_applications") && x["classes_port_applications"].IsObject()) {
                    //fprintf(stderr, "\tclasses_port_applications\n");
                    for (auto &y : x["classes_port_applications"].GetObject()) {
                        if (y.value.IsUint64() && ((float)y.value.GetUint64()/count > proc_dst_threshold)) {
                            uint16_t tmp_port = 0;
                            auto port_it = string_to_port.find(y.name.GetString());
                            if (port_it == string_to_port.end()) {
                                // throw "error: unexpected string in classes_port_applications";
                                fprintf(stderr, "error: unexpected string \"%s\" in classes_port_applications\n", y.name.GetString());
                            } else {
                                tmp_port = port_it->second;
                            }
                            portname_applications[tmp_port] = y.value.GetUint64();
                        }
                    }
                }
                if (x.HasMember("classes_ip_ip") && x["classes_ip_ip"].IsObject()) {
                    if (EXTENDED_FP_METADATA == false && process_number > 1) {
                        throw "error: extended fingerprint metadata expected, but not present";
                    }
                    EXTENDED_FP_METADATA = true;
                    //fprintf(stderr, "\tclasses_ip_ip\n");
                    for (auto &y : x["classes_ip_ip"].GetObject()) {
                        if (!y.value.IsUint64() && ((float)y.value.GetUint64()/count > proc_dst_threshold)) {
                            fprintf(stderr, "warning: classes_ip_ip object element %s is not a Uint64\n", y.name.GetString());
                            //fprintf(stderr, "\t\t%s: %lu\n", y.name.GetString(), y.value.GetUint64());
                            ip_ip[y.name.GetString()] = y.value.GetUint64();
                        }
                    }
                }
                if (x.HasMember("classes_hostname_sni") && x["classes_hostname_sni"].IsObject()) {
                    if (EXTENDED_FP_METADATA == false && process_number > 1) {
                        throw "error: extended fingerprint metadata expected, but not present";
                    }
                    EXTENDED_FP_METADATA = true;
                    //fprintf(stderr, "\tclasses_hostname_sni\n");
                    for (auto &y : x["classes_hostname_sni"].GetObject()) {
                        if (y.value.IsUint64() && ((float)y.value.GetUint64()/count > proc_dst_threshold)) {
                            //fprintf(stderr, "\t\t%s: %lu\n", y.name.GetString(), y.value.GetUint64());
                            hostname_sni[y.name.GetString()] = y.value.GetUint64();
                        }
                    }
                }
                if (report_os && x.HasMember("os_info") && x["os_info"].IsObject()) {
                    for (auto &y : x["os_info"].GetObject()) {
                        if (std::string(y.name.GetString()) != "") {
                            os_info[y.name.GetString()] = y.value.GetUint64();
                        }
                    }
                }

                class process_info process(name, malware, count, ip_as, hostname_domains, portname_applications, ip_ip, hostname_sni, os_info);
                process_vector.push_back(process);
            }
            class fingerprint_data fp_data(total_count, process_vector, os_dictionary, &subnets);
            // fp_data.print(stderr);

            if (fpdb.find(fp_string) != fpdb.end()) {
                fprintf(stderr, "warning: fingerprint database has duplicate entry for fingerprint %s\n", fp_string.c_str());
            }
            fpdb[fp_string] = fp_data;
        }
    }

    classifier(class encrypted_compressed_archive &archive,
               float fp_proc_threshold,
               float proc_dst_threshold,
               bool report_os) : os_dictionary{}, subnets{}, fpdb{}, resource_version{} {

        bool got_fp_prevalence = false;
        bool got_fp_db = false;
        bool got_version = false;
        //        class compressed_archive archive{resource_archive_file};
        const class archive_node *entry = archive.get_next_entry();
        if (entry == nullptr) {
            throw "error: could not read any entries from resource archive file";
        }
        while (entry != nullptr) {
            if (entry->is_regular_file()) {
                std::string line_str;

                std::string name = entry->get_name();
                if (name == "fp_prevalence_tls.txt") {
                    while (archive.getline(line_str)) {
                        process_fp_prevalence_line(line_str);
                    }
                    got_fp_prevalence = true;

                } else if (name == "fingerprint_db.json") {
                    while (archive.getline(line_str)) {
                        process_fp_db_line(line_str, fp_proc_threshold, proc_dst_threshold, report_os);
                    }
                    got_fp_db = true;

                } else if (name == "VERSION") {
                    while (archive.getline(line_str)) {
                        resource_version += line_str;
                    }
                    got_version = true;

                } else if (name == "pyasn.db") {
                    while (archive.getline(line_str)) {
                        subnets.process_line(line_str);
                    }
                    got_version = true;
                }
            }
            if (got_fp_db && got_fp_prevalence && got_version) {   // TODO: Do we want to require a VERSION file?
                break; // got all data, we're done here
            }
            entry = archive.get_next_entry();
        }

        subnets.process_final();
    }

    void print(FILE *f) {
        for (auto &fpdb_entry : fpdb) {
            fprintf(f, "{\"str_repr\":\"%s\"", fpdb_entry.first.c_str());
            fpdb_entry.second.print(f);
            fprintf(f, "}\n");
        }
        fp_prevalence.print(f);
    }

    std::unordered_map<std::string, uint16_t> string_to_port =
        {
         { "unknown",      0 },
         { "https",      443 },
         { "database",   448 },
         { "email",      465 },
         { "nntp",       563 },
         { "shell",      614 },
         { "ldap",       636 },
         { "ftp",        989 },
         { "nas",        991 },
         { "telnet",     992 },
         { "irc",        994 },
         { "alt-https", 1443 },
         { "docker",    2376 },
         { "tor",       8001 },
        };
    std::string port_to_app(uint16_t dst_port) {
        std::unordered_map<uint16_t, std::string> port_app = {
             {443, "https"},  {448,"database"}, {465,"email"},
             {563,"nntp"},    {585,"email"},    {614,"shell"},
             {636,"ldap"},    {989,"ftp"},      {990,"ftp"},
             {991,"nas"},     {992,"telnet"},   {993,"email"},
             {994,"irc"},     {995,"email"},    {1443,"alt-https"},
             {2376,"docker"}, {8001,"tor"},     {8443,"alt-https"},
             {9000,"tor"},    {9001,"tor"},     {9002,"tor"},
             {9101,"tor"}
        };
        auto it = port_app.find(dst_port);
        if (it != port_app.end()) {
            return it->second;
        }
        return "unknown";

    }

    struct analysis_result perform_analysis(const char *fp_str, const char *server_name, const char *dst_ip, uint16_t dst_port) {

        // fp_stats.observe(fp_str, server_name, dst_ip, dst_port); // TBD - decide where this call should go

        const auto fpdb_entry = fpdb.find(fp_str);
        if (fpdb_entry == fpdb.end()) {
            if (fp_prevalence.contains(fp_str)) {
                fp_prevalence.update(fp_str);
                return analysis_result();
            } else {
                fp_prevalence.update(fp_str);
                return analysis_result(true);
            }
        }
        class fingerprint_data &fp_data = fpdb_entry->second;

        return fp_data.perform_analysis(server_name, dst_ip, dst_port);
    }

    bool analyze_fingerprint_and_destination_context(const struct fingerprint &fp,
                                                    const struct destination_context &dc,
                                                    struct analysis_result &result) {

        if (fp.type != fingerprint_type_tls) {
            return false;  // cannot perform analysis
        }
        result = this->perform_analysis(fp.fp_str, dc.sn_str, dc.dst_ip_str, dc.dst_port);
        return true;
    }

    const char *get_resource_version() {
        return resource_version.c_str();
    }
};


#endif /* ANALYSIS_H */
