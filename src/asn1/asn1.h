/*
 * asn1.h
 */

#ifndef ASN1_H
#define ASN1_H

#include "../datum.h"
#include "../json_object.h"

namespace std {
    template <>  struct hash<struct datum>  {
        std::size_t operator()(const struct datum& p) const {
            size_t x = 5381;
            const uint8_t *tmp = p.data;
            while (tmp < p.data_end) {
                x = (33 * x) + *tmp;
            }
            return x;
        }
    };
}


/*
 * utility functions
 */

#if 0 // currently unused functions
void fprint_as_ascii_with_dots(FILE *f, const void *string, size_t len) {
    const char *s = (const char *)string;
    for (size_t i=0; i < len; i++) {
        if (isprint(s[i])) {
            fprintf(f, "%c", s[i]);
        } else {
            fprintf(f, ".");
        }
    }
    printf("\n");
}

void fprintf_parser_as_string(FILE *f, struct parser *p) {
    fprintf(f, "%.*s", (int) (p->data_end - p->data), p->data);
}
#endif

static void utc_to_generalized_time(uint8_t gt[15], const uint8_t utc[13]) {
    if (utc[0] < '5') {
        gt[0] = '2';
        gt[1] = '0';
    } else {
        gt[0] = '1';
        gt[1] = '9';
    }
    memcpy(gt + 2, utc, 13);
}


#ifndef UTILS_H

void fprintf_raw_as_hex(FILE *f, const void *data, unsigned int len) {
    if (data == NULL) {
        return;
    }
    const unsigned char *x = (const unsigned char *)data;
    const unsigned char *end = x + len;

    while (x < end) {
        fprintf(f, "%02x", *x++);
    }
}

void fprintf_json_string_escaped(FILE *f, const char *key, const uint8_t *data, unsigned int len) {
    const unsigned char *x = data;
    const unsigned char *end = data + len;

    fprintf(f, "\"%s\":\"", key);
    while (x < end) {
        if (*x < 0x20) {                   /* escape control characters   */
            fprintf(f, "\\u%04x", *x);
        } else if (*x > 0x7f) {            /* escape non-ASCII characters */
            fprintf(f, "\\u%04x", *x);
        } else {
            if (*x == '"' || *x == '\\') { /* escape special characters   */
                fprintf(f, "\\");
            }
            fprintf(f, "%c", *x);
        }
        x++;
    }
    fprintf(f, "\"");

}

#endif /* #ifndef UTILS_H */

void fprintf_json_string_escaped(struct buffer_stream &buf, const char *key, const uint8_t *data, unsigned int len) {
    const unsigned char *x = data;
    const unsigned char *end = data + len;

    buf.snprintf("\"%s\":\"", key);
    while (x < end) {
        if (*x < 0x20) {                   /* escape control characters   */
            buf.snprintf("\\u%04x", *x);
        } else if (*x >= 0x80) {           /* escape non-ASCII characters */

            uint32_t codepoint = 0;
            if (*x >= 0xc0) {

                if (*x >= 0xe0) {
                    if (*x >= 0xf0) {
                        codepoint = (*x++ & 0x07);
                        codepoint = (*x++ & 0x3f) | (codepoint << 6);
                        codepoint = (*x++ & 0x3f) | (codepoint << 6);
                        codepoint = (*x   & 0x3f) | (codepoint << 6);

                    } else {
                        codepoint = (*x++ & 0x0F);
                        codepoint = (*x++ & 0x3f) | (codepoint << 6);
                        codepoint = (*x   & 0x3f) | (codepoint << 6);
                    }

                } else {
                    codepoint = ((*x++ & 0x1f) << 6);
                    codepoint |= *x & 0x3f;
                }

            } else {
                codepoint = *x & 0x7f;
            }
            if (codepoint < 0x10000) {
                // basic multilingual plane
                if (codepoint < 0xd800) {
                    buf.snprintf("\\u%04x", codepoint);
                } else {
                    // error: invalid or private codepoint
                    buf.snprintf("\\ue000", codepoint); // indicate error with private use codepoint
                }
            } else {
                // surrogate pair
                codepoint -= 0x10000;
                uint32_t hi = (codepoint >> 10) + 0xd800;
                uint32_t lo = (codepoint & 0x3ff) + 0xdc00;
                buf.snprintf("\\u%04x", hi);
                buf.snprintf("\\u%04x", lo);
            }

        } else {
            if (*x == '"' || *x == '\\') { /* escape special characters   */
                buf.snprintf("\\");
            }
            buf.snprintf("%c", *x);
        }
        x++;
    }
    buf.snprintf("\"");

}

void fprintf_json_char_escaped(FILE *f, unsigned char x) {
    if (x < 0x20) {                   /* escape control characters   */
        fprintf(f, "\\u%04x", x);
    } else if (x > 0x7f) {            /* escape non-ASCII characters */
        fprintf(f, "\\u%04x", x);
    } else {
        if (x == '"' || x == '\\') { /* escape special characters   */
            fprintf(f, "\\");
        }
        fprintf(f, "%c", x);
    }
}

void fprintf_json_char_escaped(struct buffer_stream &buf, unsigned char x) {
    if (x < 0x20) {                   /* escape control characters   */
        buf.snprintf("\\u%04x", x);
    } else if (x > 0x7f) {            /* escape non-ASCII characters */
        buf.snprintf("\\u%04x", x);
    } else {
        if (x == '"' || x == '\\') { /* escape special characters   */
            buf.snprintf("\\");
        }
        buf.snprintf("%c", x);
    }
}

void fprintf_ip_address(FILE *f, const uint8_t *buffer, size_t length) {
    const uint8_t *b = buffer;
    if (length == 4) {
        fprintf(f, "%u.%u.%u.%u", b[0], b[1], b[2], b[3]);
    } else if (length == 16) {
        fprintf(f, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
                b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
    } else {
        fprintf(f, "malformed (length: %zu)", length);
    }
}

void fprintf_ip_address(struct buffer_stream &buf, const uint8_t *buffer, size_t length) {
    const uint8_t *b = buffer;
    if (length == 4) {
        buf.snprintf("%u.%u.%u.%u", b[0], b[1], b[2], b[3]);
    } else if (length == 16) {
        buf.snprintf("%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
                b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
    } else {
        buf.snprintf("malformed (length: %zu)", length);
    }
}

/*
 * UTCTime (Coordinated Universal Time) consists of 13 bytes that
 * encode the Greenwich Mean Time in the format YYMMDDhhmmssZ.  For
 * instance, the bytes 17 0d 31 35 31 30 32 38 31 38 35 32 31 32 5a
 * encode the string "151028185212Z", which represents the time
 * "2015-10-28 18:52:12"
 */
void fprintf_json_utctime(FILE *f, const char *key, const uint8_t *data, unsigned int len) {

    fprintf(f, "\"%s\":\"", key);
    if (len != 13) {
        fprintf(f, "malformed\"");
        return;
    }
    if (data[0] < '5') {
        fprintf(f, "20");
    } else {
       fprintf(f, "19");
    }
    fprintf_json_char_escaped(f, data[0]);
    fprintf_json_char_escaped(f, data[1]);
    fprintf(f, "-");
    fprintf_json_char_escaped(f, data[2]);
    fprintf_json_char_escaped(f, data[3]);
    fprintf(f, "-");
    fprintf_json_char_escaped(f, data[4]);
    fprintf_json_char_escaped(f, data[5]);
    fprintf(f, " ");
    fprintf_json_char_escaped(f, data[6]);
    fprintf_json_char_escaped(f, data[7]);
    fprintf(f, ":");
    fprintf_json_char_escaped(f, data[8]);
    fprintf_json_char_escaped(f, data[9]);
    fprintf(f, ":");
    fprintf_json_char_escaped(f, data[10]);
    fprintf_json_char_escaped(f, data[11]);

    fprintf(f, "\"");
}

/*
 *  For the purposes of [RFC 5280], GeneralizedTime values MUST be
 *  expressed in Greenwich Mean Time (Zulu) and MUST include seconds
 *  (i.e., times are YYYYMMDDHHMMSSZ), even where the number of
 *  seconds is zero.
 */
void fprintf_json_generalized_time(FILE *f, const char *key, const uint8_t *data, unsigned int len) {

    fprintf(f, "\"%s\":\"", key);
    if (len != 15) {
        fprintf(f, "malformed (length %u)\"", len);
        return;
    }
    fprintf_json_char_escaped(f, data[0]);
    fprintf_json_char_escaped(f, data[1]);
    fprintf_json_char_escaped(f, data[2]);
    fprintf_json_char_escaped(f, data[3]);
    fprintf(f, "-");
    fprintf_json_char_escaped(f, data[4]);
    fprintf_json_char_escaped(f, data[5]);
    fprintf(f, "-");
    fprintf_json_char_escaped(f, data[6]);
    fprintf_json_char_escaped(f, data[7]);
    fprintf(f, " ");
    fprintf_json_char_escaped(f, data[8]);
    fprintf_json_char_escaped(f, data[9]);
    fprintf(f, ":");
    fprintf_json_char_escaped(f, data[10]);
    fprintf_json_char_escaped(f, data[11]);
    fprintf(f, ":");
    fprintf_json_char_escaped(f, data[12]);
    fprintf_json_char_escaped(f, data[13]);

    fprintf(f, "\"");
}

/*
 * UTCTime (Coordinated Universal Time) consists of 13 bytes that
 * encode the Greenwich Mean Time in the format YYMMDDhhmmssZ.  For
 * instance, the bytes 17 0d 31 35 31 30 32 38 31 38 35 32 31 32 5a
 * encode the string "151028185212Z", which represents the time
 * "2015-10-28 18:52:12"
 */
void fprintf_json_utctime(struct buffer_stream &buf, const char *key, const uint8_t *data, unsigned int len) {

    buf.snprintf("\"%s\":\"", key);
    if (len != 13) {
        buf.snprintf("malformed\"");
        return;
    }
    if (data[0] < '5') {
        buf.snprintf("20");
    } else {
       buf.snprintf("19");
    }
    fprintf_json_char_escaped(buf, data[0]);
    fprintf_json_char_escaped(buf, data[1]);
    buf.write_char('-');
    fprintf_json_char_escaped(buf, data[2]);
    fprintf_json_char_escaped(buf, data[3]);
    buf.write_char('-');
    fprintf_json_char_escaped(buf, data[4]);
    fprintf_json_char_escaped(buf, data[5]);
    buf.write_char(' ');
    fprintf_json_char_escaped(buf, data[6]);
    fprintf_json_char_escaped(buf, data[7]);
    buf.write_char(':');
    fprintf_json_char_escaped(buf, data[8]);
    fprintf_json_char_escaped(buf, data[9]);
    buf.write_char(':');
    fprintf_json_char_escaped(buf, data[10]);
    fprintf_json_char_escaped(buf, data[11]);
    buf.write_char('\"');
}


/*
 *  For the purposes of [RFC 5280], GeneralizedTime values MUST be
 *  expressed in Greenwich Mean Time (Zulu) and MUST include seconds
 *  (i.e., times are YYYYMMDDHHMMSSZ), even where the number of
 *  seconds is zero.
 */
void fprintf_json_generalized_time(struct buffer_stream &buf, const char *key, const uint8_t *data, unsigned int len) {

    buf.snprintf("\"%s\":\"", key);
    if (len != 15) {
        buf.snprintf("malformed (length %u)\"", len);
        return;
    }
    fprintf_json_char_escaped(buf, data[0]);
    fprintf_json_char_escaped(buf, data[1]);
    fprintf_json_char_escaped(buf, data[2]);
    fprintf_json_char_escaped(buf, data[3]);
    buf.snprintf("-");
    fprintf_json_char_escaped(buf, data[4]);
    fprintf_json_char_escaped(buf, data[5]);
    buf.snprintf("-");
    fprintf_json_char_escaped(buf, data[6]);
    fprintf_json_char_escaped(buf, data[7]);
    buf.snprintf(" ");
    fprintf_json_char_escaped(buf, data[8]);
    fprintf_json_char_escaped(buf, data[9]);
    buf.snprintf(":");
    fprintf_json_char_escaped(buf, data[10]);
    fprintf_json_char_escaped(buf, data[11]);
    buf.snprintf(":");
    fprintf_json_char_escaped(buf, data[12]);
    fprintf_json_char_escaped(buf, data[13]);

    buf.snprintf("\"");
}

/*
 * generalized_time_gt(d1, l1, d2, l2) compares two strings at
 * locations d1 and d2 with lengths l1 and l2 assuming that they are
 * in generalized time format (YYYYMMDDHHMMSSZ), and returns 1
 * if d1 > d2, returns -1 if d1 < d2, and returns 0 if they are equal.
 */

int generalized_time_gt(const uint8_t *d1, unsigned int l1,
                        const uint8_t *d2, unsigned int l2) {

    if (l1 != 15 || l2 != 15) {
        return -1;  // malformed input
    }
    for (int i=0; i<15; i++) {
        if (d1[i] < d2[i]) {
            return -1;
        } else if (d1[i] > d2[i]) {
            return 1;
        }
    }
    return 0;
}

int utctime_to_generalized_time(uint8_t *gt, size_t gt_len, const uint8_t *utc_time, size_t utc_len) {
    if (gt_len != 15) {
        return -1;  // error: wrong output buffer size
    }
    if (utc_len != 12) {
        return -1;  // error: wrong input buffer size
    }
    if (utc_time[0] < '5') {
        gt[0] = '2';
        gt[1] = '0';
    } else {
        gt[0] = '1';
        gt[1] = '9';
    }
    memcpy(gt+2, utc_time, 12);
    return 0;
}

inline uint8_t hex_to_raw(const char *hex) {
    int value = 0;
    if(*hex >= '0' && *hex <= '9') {
        value = (*hex - '0');
    } else if (*hex >= 'A' && *hex <= 'F') {
        value = (10 + (*hex - 'A'));
    } else if (*hex >= 'a' && *hex <= 'f') {
        value = (10 + (*hex - 'a'));
    }
    value = value << 4;
    hex++;
    if(*hex >= '0' && *hex <= '9') {
        value |= (*hex - '0');
    } else if (*hex >= 'A' && *hex <= 'F') {
        value |= (10 + (*hex - 'A'));
    } else if (*hex >= 'a' && *hex <= 'f') {
        value |= (10 + (*hex - 'a'));
    }

    return value;
}

void hex_string_print_as_oid(FILE *f, const char *c, size_t length) {
    if (length & 1) {
        return;  // error: odd number of characters in hex string
    }
    uint32_t component = hex_to_raw(c);
    uint32_t div = component / 40;
    uint32_t rem = component - (div * 40);
    if (div > 2 || rem > 39) {
        return; // error: invalid input
    }
    fprintf(f, "%u.%u", div, rem);

    c += 2;
    component = 0;
    for (unsigned int i=2; i<length; i += 2) {
        uint8_t tmp = hex_to_raw(c);
        if (tmp & 0x80) {
            component = component * 128 + (tmp & 0x7f);
        } else {
            component = component * 128 + tmp;
            fprintf(f, ".%u", component);
            component = 0;
        }
        c += 2;
    }
}

void raw_string_print_as_oid(FILE *f, const uint8_t *raw, size_t length) {
    if (raw == NULL) {
        return;  // error: invalid input
    }
    uint32_t component = *raw;
    uint32_t div = component / 40;
    uint32_t rem = component - (div * 40);
    if (div > 2 || rem > 39) {
        return; // error: invalid input
    }
    fprintf(f, "%u.%u", div, rem);

    raw++;
    component = 0;
    for (unsigned int i=1; i<length; i++) {
        uint8_t tmp = *raw++;
        if (tmp & 0x80) {
            component = component * 128 + (tmp & 0x7f);
        } else {
            component = component * 128 + tmp;
            fprintf(f, ".%u", component);
            component = 0;
        }
    }
}

void raw_string_print_as_oid(struct buffer_stream &buf, const uint8_t *raw, size_t length) {
    if (raw == NULL) {
        return;  // error: invalid input
    }
    uint32_t component = *raw;
    uint32_t div = component / 40;
    uint32_t rem = component - (div * 40);
    if (div > 2 || rem > 39) {
        return; // error: invalid input
    }
    buf.snprintf("%u.%u", div, rem);

    raw++;
    component = 0;
    for (unsigned int i=1; i<length; i++) {
        uint8_t tmp = *raw++;
        if (tmp & 0x80) {
            component = component * 128 + (tmp & 0x7f);
        } else {
            component = component * 128 + tmp;
            buf.snprintf(".%u", component);
            component = 0;
        }
    }
}

static const char *oid_empty_string = "";
const char *datum_get_oid_string(const struct datum *p) {
    std::basic_string<uint8_t> s = p->get_bytestring();
    auto pair = oid_dict.find(s);
    if (pair == oid_dict.end()) {
        return oid_empty_string;
    }
    return pair->second.c_str();
}

enum oid datum_get_oid_enum(const struct datum *p) {
    std::basic_string<uint8_t> s = p->get_bytestring();
    auto pair = oid_to_enum.find(s);
    if (pair == oid_to_enum.end()) {
        return oid::unknown;
    }
    return pair->second;
}

/*
 * json_object extensions for printing to TLVs
 */

struct json_object_asn1 : public json_object {
    explicit json_object_asn1(struct buffer_stream *buf) : json_object(buf) {}
    json_object_asn1(struct json_object &object, const char *name) : json_object(object, name) {
        //fprintf(stderr, "json_object_asn1 constructor\n");
    }
    explicit json_object_asn1(struct json_object &object) : json_object(object) {
        //fprintf(stderr, "json_object_asn1 constructor\n");
    }
    explicit json_object_asn1(struct json_array &array);

    void print_key_oid(const char *k, const struct datum &value) {
        const char *output = datum_get_oid_string(&value);
        write_comma(comma);
        if (output != oid_empty_string) {
            b->snprintf("\"%s\":\"%s\"", k, output);
        } else {
            b->snprintf("\"%s\":\"", k);
            if (value.data && value.data_end) {
                raw_string_print_as_oid(*b, value.data, value.data_end - value.data);
            }
            b->write_char('\"');
        }
    }

    void print_key_bitstring_flags(const char *name, const struct datum &value, char * const *flags) {
        struct json_array a{*this, name};
        if (value.data) {
            struct datum p = value;
            char *const *tmp = flags;
            size_t number_of_unused_bits = 0;
            datum_read_and_skip_uint(&p, 1, &number_of_unused_bits);
            while (p.data < p.data_end-1) {
                for (uint8_t x = 0x80; x > 0; x=x>>1) {
                    if (x & *p.data) {
                        if (*tmp) {
                            a.print_string(*tmp);
                        }         // note: we don't report excess length
                    }
                    if (*tmp) {
                        tmp++;
                    }
                }
                p.data++;
            }
            uint8_t terminus = 0x80 >> (8-number_of_unused_bits);
            for (uint8_t x = 0x80; x > terminus; x=x>>1) {
                if (x & *p.data) {
                    if (*tmp) {
                        a.print_string(*tmp);
                    }         // note: we don't report excess length
                }
                if (*tmp) {
                    tmp++;
                }
            }

        }
        a.close();
        comma = true;
    }

    void print_key_escaped_string(const char *k, const struct datum &value) {
        write_comma(comma);
        fprintf_json_string_escaped(*b, k, value.data, value.data_end - value.data);
    }

    /*
     * UTCTime (Coordinated Universal Time) consists of 13 bytes that
     * encode the Greenwich Mean Time in the format YYMMDDhhmmssZ.  For
     * instance, the bytes 17 0d 31 35 31 30 32 38 31 38 35 32 31 32 5a
     * encode the string "151028185212Z", which represents the time
     * "2015-10-28 18:52:12"
     */
    void print_key_utctime(const char *key, const uint8_t *data, unsigned int len) {
        write_comma(comma);
        b->snprintf("\"%s\":\"", key);
        if (len != 13) {
            b->snprintf("malformed\"");
            return;
        }
        if (data[0] < '5') {
            b->snprintf("20");
        } else {
            b->snprintf("19");
        }
        fprintf_json_char_escaped(*b, data[0]);
        fprintf_json_char_escaped(*b, data[1]);
        b->write_char('-');
        fprintf_json_char_escaped(*b, data[2]);
        fprintf_json_char_escaped(*b, data[3]);
        b->write_char('-');
        fprintf_json_char_escaped(*b, data[4]);
        fprintf_json_char_escaped(*b, data[5]);
        b->write_char(' ');
        fprintf_json_char_escaped(*b, data[6]);
        fprintf_json_char_escaped(*b, data[7]);
        b->write_char(':');
        fprintf_json_char_escaped(*b, data[8]);
        fprintf_json_char_escaped(*b, data[9]);
        b->write_char(':');
        fprintf_json_char_escaped(*b, data[10]);
        fprintf_json_char_escaped(*b, data[11]);
        fprintf_json_char_escaped(*b, data[12]);
        b->write_char('\"');
    }

    /*
     *  For the purposes of [RFC 5280], GeneralizedTime values MUST be
     *  expressed in Greenwich Mean Time (Zulu) and MUST include seconds
     *  (i.e., times are YYYYMMDDHHMMSSZ), even where the number of
     *  seconds is zero.
     */
    void print_key_generalized_time(const char *key, const uint8_t *data, unsigned int len) {
        write_comma(comma);
        b->snprintf("\"%s\":\"", key);
        if (len != 15) {
            b->snprintf("malformed (length %u)\"", len);
            return;
        }
        fprintf_json_char_escaped(*b, data[0]);
        fprintf_json_char_escaped(*b, data[1]);
        fprintf_json_char_escaped(*b, data[2]);
        fprintf_json_char_escaped(*b, data[3]);
        b->write_char('-');
        fprintf_json_char_escaped(*b, data[4]);
        fprintf_json_char_escaped(*b, data[5]);
        b->write_char('-');
        fprintf_json_char_escaped(*b, data[6]);
        fprintf_json_char_escaped(*b, data[7]);
        b->write_char(' ');
        fprintf_json_char_escaped(*b, data[8]);
        fprintf_json_char_escaped(*b, data[9]);
        b->write_char(':');
        fprintf_json_char_escaped(*b, data[10]);
        fprintf_json_char_escaped(*b, data[11]);
        b->write_char(':');
        fprintf_json_char_escaped(*b, data[12]);
        fprintf_json_char_escaped(*b, data[13]);
        fprintf_json_char_escaped(*b, data[14]);
        b->write_char('\"');
    }

    void print_key_ip_address(const char *name, const datum &value) {
        write_comma(comma);
        b->snprintf("\"%s\":\"", name);
        fprintf_ip_address(*b, value.data, value.data_end - value.data);
        b->write_char('\"');
    }

};

json_object_asn1::json_object_asn1(struct json_array &array) : json_object(array) { }

struct json_array_asn1 : public json_array {
    explicit json_array_asn1(struct buffer_stream *b) : json_array(b) { }
    explicit json_array_asn1(struct json_object &object, const char *name) : json_array(object, name) { }
    void print_oid(const struct datum &value) {
        const char *output = datum_get_oid_string(&value);
        write_comma(comma);
        if (output != oid_empty_string) {
            b->snprintf("\"%s\"", output);
        } else {
            b->write_char('\"');
            if (value.data && value.data_end) {
                raw_string_print_as_oid(*b, value.data, value.data_end - value.data);
            }
            b->write_char('\"');
        }
    }
};

/*
 * struct tlv holds the tag, length, and (pointers to the beginning
 * and end of the) value.  ASN.1 consists of a sequence of TLV
 * elements, and a struct tlv represents one of these elements.
 *
 * A struct tlv can be initialized to a null value, or initialized
 * with data.  To read data into a struct tlv, call the member
 * function 'parse()'.  The pointers in the 'value' member of a struct
 * tlv are either NULL, or are set to point to a region of memory by
 * the call to parse().
 *
 * WARNING: if that memory is deallocated or changed, the pointers in
 * the struct tlv object will be invalid.  To prevent a struct tlv
 * object from having dangling pointers, its scope MUST be no larger
 * than the scope of the memory buffer it is parsing.
 */

struct tlv {
    unsigned char tag;
    size_t length;
    struct datum value;

    bool operator == (const struct tlv &r) {
        return tag == r.tag && length == r.length && value == r.value;
    }

    constexpr static unsigned char explicit_tag(unsigned char tag) {
        return 0x80 + tag;  // warning: tag must be between 0 and 31 inclusive
    }
    constexpr static unsigned char explicit_tag_constructed(unsigned char tag) {
        return 0xa0 + tag;  // warning: tag must be between 0 and 31 inclusive
    }

    enum tag {
        END_OF_CONTENT	  = 0x00,
        BOOLEAN	          = 0x01,
        INTEGER		      = 0x02,
        BIT_STRING		  = 0x03,
        OCTET_STRING      = 0x04,
        NULL_TAG          = 0x05,
        OBJECT_IDENTIFIER = 0x06,
        OBJECT_DESCRIPTOR = 0x07,
        EXTERNAL          = 0x08,
        REAL  		      = 0x09,
        ENUMERATED	      = 0x0a,
        EMBEDDED_PDV	  = 0x0b,
        UTF8String		  = 0x0c,
        RELATIVE_OID      = 0x0d,
        TIME		      = 0x0e,
        RESERVED	      = 0x0f,
        SEQUENCE          = 0x30,  // also "SEQUENCE OF"
        SET 		      = 0x31,  // also "SET OF"
        NUMERIC_STRING	  = 0x12,
        PRINTABLE_STRING  = 0x13,
        T61String		  = 0x14,
        VIDEOTEX_STRING   = 0x15,
        IA5String		  = 0x16,
        UTCTime		      = 0x17,
        GeneralizedTime	  = 0x18,
        GraphicString	  = 0x19,
        VisibleString	  = 0x1a,
        GeneralString	  = 0x1b,
        UniversalString	  = 0x1c,
        CHARACTER_STRING  = 0x1d,
        BMP_STRING		  = 0x1e
    };

    bool is_not_null() const {
        return value.data;
    }
    bool is_null() const {
        return (value.data == NULL);
    }
    bool is_truncated() const {
        return value.data != NULL && value.length() != (ssize_t) length;
    }
    bool is_complete() const {
        return value.data != NULL && value.length() == (ssize_t) length;
    }
    uint8_t get_little_tag() const { return tag & 0x1f; }
    tlv() {
        // initialize to null/zero
        tag = 0;
        length = 0;
        value.data = NULL;
        value.data_end = NULL;
    }
    tlv(const tlv &r) : tag{0}, length{0}, value{NULL, NULL} {
        tag = r.tag;
        length = r.length;
        value.data = r.value.data;
        value.data_end = r.value.data_end;
    }
    void operator=(const tlv &r) {
        tag = r.tag;
        length = r.length;
        value.data = r.value.data;
        value.data_end = r.value.data_end;
    }
    explicit tlv(struct datum *p, uint8_t expected_tag=0x00, const char *tlv_name=NULL) : tag{0}, length{0}, value{NULL, NULL} {
        parse(p, expected_tag, tlv_name);
    }
    void handle_parse_error(const char *msg, const char *tlv_name) {
#ifdef TLV_ERR_INFO
        fprintf(stderr, "%s in %s\n", msg, tlv_name ? tlv_name : "unknown TLV");
#else
        (void)msg;
        (void)tlv_name;
#endif
#ifdef THROW
        throw msg;
#endif
    }
    void parse(struct datum *p, uint8_t expected_tag=0x00, const char *tlv_name=NULL) {

        if (p->data == NULL) {
            handle_parse_error("warning: NULL data", tlv_name ? tlv_name : "unknown TLV");
        }
        if (datum_get_data_length(p) < 2) {
            p->set_empty();  // parser is no longer good for reading
            // fprintf(stderr, "error: incomplete data (only %ld bytes in %s)\n", p->data_end - p->data, tlv_name ? tlv_name : "unknown TLV");
            handle_parse_error("warning: incomplete data", tlv_name);
            return;  // leave tlv uninitialized
        }

        if (expected_tag && p->data[0] != expected_tag) {
            // fprintf(stderr, "note: unexpected type (got %02x, expected %02x)\n", p->data[0], expected_tag);
            // p->set_empty();  // TODO: do we want this?  parser is no longer good for reading
            handle_parse_error("note: unexpected type", tlv_name);
            return;  // unexpected type
        }
        // set tag
        tag = p->data[0];
        length = p->data[1];

        datum_skip(p, 2);

        // set length
        if (length >= 128) {
            ssize_t num_octets_in_length = length - 128;  // note: signed to avoid underflow
            if (num_octets_in_length < 0) {
                p->set_empty();  // parser is no longer good for reading
                handle_parse_error("error: invalid length field", tlv_name);
                return;
            }
            if (datum_read_and_skip_uint(p, num_octets_in_length, &length) == status_err) {
                p->set_empty();  // parser is no longer good for reading
                // fprintf(stderr, "error: could not read length (want %lu bytes, only %ld bytes remaining)\n", length, datum_get_data_length(p));
                handle_parse_error("warning: could not read length", tlv_name);
                return;
            }
        }

        // set value
        datum_init_from_outer_parser(&value, p, length);
        if (datum_skip(p, length) == status_err) {
            p->set_empty();   // parser is no longer good for reading
            handle_parse_error("warning: value field is truncated", tlv_name);
        }

#ifdef ASN1_DEBUG
        fprint_tlv(stderr, tlv_name);
#endif
    }

    void remove_bitstring_encoding() {
        size_t first_octet = 0;
        datum_read_and_skip_uint(&value, 1, &first_octet);
        if (first_octet) {
            // throw "error removing bitstring encoding";
            value.set_null();
        }
        if (length > 0) {
            length = length - 1;
        }
    }
    /*
     * fprintf_tlv(f, name) prints the ASN1 TLV details to f
     *
     * Tag notation: (Tag Class:Constructed:Tag Number)
     *
     *    Tag Class: 0=universal (native to ASN.1), 1=Application
     *    specific, 2=Context-specific, 3=Private
     *
     *    Constructed: 1=yes (value contains zero or more element
     *    encodings), 0=no (primitive)
     *
     *    Tag Number: 0-31
     *
     * End-of-Content (EOC)		0	0
     * BOOLEAN		            1	1
     * INTEGER		            2	2
     * BIT STRING		        3	3
     * OCTET STRING		        4	4
     * NULL		                5	5
     * OBJECT IDENTIFIER		6	6
     * Object Descriptor		7	7
     * EXTERNAL		            8	8
     * REAL (float)		        9	9
     * ENUMERATED		       10	A
     * EMBEDDED PDV		       11	B
     * UTF8String		       12	C
     * RELATIVE-OID		       13	D
     * TIME		               14	E
     * Reserved		           15	F
     * SEQUENCE, SEQUENCE OF   16	10
     * SET and SET OF		   17	11
     * NumericString		   18	12
     * PrintableString		   19	13
     * T61String		       20	14
     * VideotexString		   21	15
     * IA5String		       22	16
     * UTCTime		           23	17
     * GeneralizedTime		   24	18
     * GraphicString		   25	19
     * VisibleString		   26	1A
     * GeneralString		   27	1B
     * UniversalString		   28	1C
     * CHARACTER STRING		   29	1D
     * BMPString		       30	1E
     * DATE		               31	1F *** LONG FORM TAG NUMBER ***
     * TIME-OF-DAY		       32	20 *** LONG FORM TAG NUMBER ***
     * DATE-TIME		       33	21 *** LONG FORM TAG NUMBER ***
     * DURATION		           34	22 *** LONG FORM TAG NUMBER ***
     * OID-IRI		           35	23 *** LONG FORM TAG NUMBER ***
     * RELATIVE-OID-IRI		   36	24 *** LONG FORM TAG NUMBER ***
     */

    void fprint_tlv(FILE *f, const char *tlv_name) const {
        const char *type[32] = {
            "End-of-Content",
            "BOOLEAN",
            "INTEGER",
            "BIT STRING",
            "OCTET STRING",
            "NULL",
            "OBJECT IDENTIFIER",
            "Object Descriptor",
            "EXTERNAL",
            "REAL (float)",
            "ENUMERATED",
            "EMBEDDED PDV",
            "UTF8String",
            "RELATIVE-OID",
            "TIME",
            "Reserved",
            "SEQUENCE, SEQUENCE OF",
            "SET and SET OF",
            "NumericString",
            "PrintableString",
            "T61String",
            "VideotexString",
            "IA5String",
            "UTCTime",
            "GeneralizedTime",
            "GraphicString",
            "VisibleString",
            "GeneralString",
            "UniversalString",
            "CHARACTER STRING",
            "BMPString"
        };

        if (value.data) {
            uint8_t tag_class = tag >> 6;
            uint8_t constructed = (tag >> 5) & 1;
            uint8_t tag_number = tag & 31;
            if (tag_class == 2) {
                // tag is context-specific
                fprintf(f, "T:%02x (%u:%u:%u, explicit tag %u)\tL:%08zu\tV:", tag, tag_class, constructed, tag_number, tag_number, length);
            } else {
                fprintf(f, "T:%02x (%u:%u:%u, %s)\tL:%08zu\tV:", tag, tag_class, constructed, tag_number, type[tag_number], length);
            }
            fprintf_raw_as_hex(f, value.data, value.data_end - value.data);
            if (tlv_name) {
                fprintf(f, "\t(%s)\n", tlv_name);
            } else {
                fprintf(f, "\n");
            }
        } else {
            fprintf(f, "null (%s)\n", tlv_name);
        }
    }

    inline bool is_constructed() const {
        return tag & 0x20;
    }

    // is_der_format(data, length) is a spot-check to provide
    // early detection of malformed input, not a complete check
    //
    static bool is_der_format(const void *data, size_t length) {
        uint8_t *d = (uint8_t *)data;
        struct datum p{d, d + length};
        struct tlv test(&p, tlv::SEQUENCE);
        if (test.is_null() || test.length > (length - 2)) {
            return false;
        }
        return true;
    }

    int time_cmp(const struct tlv &t) const {
        ssize_t l1 = value.data_end - value.data;
        ssize_t l2 = t.value.data_end - t.value.data;
        ssize_t min = l1 < l2 ? l1 : l2;
        if (min == 0) {
            return 0;
        }
        // fprintf(stderr, "comparing %zd bytes of times\nl1: %.*s\nl2: %.*s\n", min, l1, value.data, l2, t.value.data);

        const uint8_t *d1 = value.data;
        const uint8_t *d2 = t.value.data;
        uint8_t gt1[15];
        if (tag == tlv::UTCTime) {
            d1 = gt1;
            utc_to_generalized_time(gt1, value.data);
        } else if (tag != tlv::GeneralizedTime) {
            return 0; // error; attempt to compare non-time value
        }
        uint8_t gt2[15];
        if (t.tag == tlv::UTCTime) {
            d2 = gt2;
            utc_to_generalized_time(gt2, t.value.data);
        } else if (tag != tlv::GeneralizedTime) {
            return 0; // error; attempt to compare non-time value
        }

        if (d1 && d2) {
            return memcmp((const char *)d1, (const char *)d2, min);
        }
        return 0;
    }
    void set(enum tlv::tag type, const void *data, size_t len) {
        tag = type;
        length = len;
        value.data = (const uint8_t *)data;
        value.data_end = (const uint8_t *)data + len;
    }

#if 0
    /*
     * functions for printing to a FILE *
     */
    void print_as_json_hex(FILE *f, const char *name, bool comma=false) const {
        const char *format_string = "\"%s\":\"";
        if (comma) {
            format_string = ",\"%s\":\"";
        }
        fprintf(f, format_string, name);
        if (value.data && value.data_end) {
            fprintf_raw_as_hex(f, value.data, value.data_end - value.data);
        }
        fprintf(f, "\"");
    }
    void print_as_json_oid(FILE *f, const char *name, bool comma=false) const {
        const char *format_string = "\"%s\":";
        if (comma) {
            format_string = ",\"%s\":";
        }
        fprintf(f, format_string, name);

        const char *output = datum_get_oid_string(&value);
        if (output != oid_empty_string) {
            fprintf(f, "\"%s\"", output);
        } else {
            fprintf(f, "\"");
            raw_string_print_as_oid(f, value.data, value.data_end - value.data);
            fprintf(f, "\"");
        }

    }
    void print_as_json_utctime(FILE *f, const char *name) const {
        fprintf_json_utctime(f, name, value.data, value.data_end - value.data);
    }
    void print_as_json_generalized_time(FILE *f, const char *name) const {
        fprintf_json_generalized_time(f, name, value.data, value.data_end - value.data);
    }
    void print_as_json_escaped_string(FILE *f, const char *name) const {
        fprintf_json_string_escaped(f, name, value.data, value.data_end - value.data);
    }
    void print_as_json_ip_address(FILE *f, const char *name) const {
        fprintf(f, "{\"%s\":\"", name);
        fprintf_ip_address(f, value.data, value.data_end - value.data);
        fprintf(f, "\"}");
    }
    void print_as_json_bitstring(FILE *f, const char *name, bool comma=false) const {
        const char *format_string = "\"%s\":[";
        if (comma) {
            format_string = ",\"%s\":[";
        }
        fprintf(f, format_string, name);
        if (value.data) {
            struct datum p = value;
            size_t number_of_unused_bits;
            datum_read_and_skip_uint(&p, 1, &number_of_unused_bits);
            const char *comma = "";
            while (p.data < p.data_end-1) {
                for (uint8_t x = 0x80; x > 0; x=x>>1) {
                    fprintf(f, "%s%c", comma, x & *p.data ? '1' : '0');
                    comma = ",";
                }
                p.data++;
            }
            uint8_t terminus = 0x80 >> (8-number_of_unused_bits);
            for (uint8_t x = 0x80; x > terminus; x=x>>1) {
                fprintf(f, "%s%c", comma, x & *p.data ? '1' : '0');
                comma = ",";
            }

        }
        fprintf(f, "]");
    }
    void print_as_json_bitstring_flags(FILE *f, const char *name, char * const *flags, bool comma=false) const {
        const char *format_string = "\"%s\":[";
        if (comma) {
            format_string = ",\"%s\":[";
        }
        fprintf(f, format_string, name);
        if (value.data) {
            struct datum p = value;
            char *const *tmp = flags;
            size_t number_of_unused_bits;
            datum_read_and_skip_uint(&p, 1, &number_of_unused_bits);
            const char *comma = "";
            while (p.data < p.data_end-1) {
                for (uint8_t x = 0x80; x > 0; x=x>>1) {
                    if (x & *p.data) {
                        fprintf(f, "%s\"%s\"", comma, *tmp);
                        comma = ",";
                    }
                    if (*tmp) {
                        tmp++;
                    }
                }
                p.data++;
            }
            uint8_t terminus = 0x80 >> (8-number_of_unused_bits);
            for (uint8_t x = 0x80; x > terminus; x=x>>1) {
                if (x & *p.data) {
                    fprintf(f, "%s\"%s\"", comma, *tmp);
                    comma = ",";
                }
                if (*tmp) {
                    tmp++;
                }
            }

        }
        fprintf(f, "]");
    }

    void print_as_json(FILE *f, const char *name) const {
        switch(tag) {
        case tlv::UTCTime:
            print_as_json_utctime(f, name);
            break;
        case tlv::GeneralizedTime:
            print_as_json_generalized_time(f, name);
            break;
        case tlv::OBJECT_IDENTIFIER:
            print_as_json_oid(f, name);
            break;
        case tlv::PRINTABLE_STRING:
        case tlv::T61String:
        case tlv::VIDEOTEX_STRING:
        case tlv::IA5String:
        case tlv::GraphicString:
        case tlv::VisibleString:
            print_as_json_escaped_string(f, name);
            break;
        case tlv::BIT_STRING:
            print_as_json_bitstring(f, name);
            break;
        default:
            print_as_json_hex(f, name);  // handle unexpected type
        }
    }

    /*
     * functions for json serialization to a buffer_stream
     */
    void print_as_json_hex(struct buffer_stream &buf, const char *name, bool comma=false) const {
        const char *format_string = "\"%s\":\"";
        if (comma) {
            format_string = ",\"%s\":\"";
        }
        buf.snprintf(format_string, name);
        if (value.data && value.data_end) {
            buf.raw_as_hex(value.data, value.data_end - value.data);
        }
        buf.snprintf("\"");
    }

    void print_as_json_oid(struct buffer_stream &buf, const char *name, bool comma=false) const {
        const char *format_string = "\"%s\":";
        if (comma) {
            format_string = ",\"%s\":";
        }
        buf.snprintf(format_string, name);

        const char *output = datum_get_oid_string(&value);
        if (output != oid_empty_string) {
            buf.snprintf("\"%s\"", output);
        } else {
            buf.snprintf("\"");
            raw_string_print_as_oid(buf, value.data, value.data_end - value.data);
            buf.snprintf("\"");
        }

    }
    void print_as_json_utctime(struct buffer_stream &buf, const char *name) const {
        fprintf_json_utctime(buf, name, value.data, value.data_end - value.data);
    }
    void print_as_json_generalized_time(struct buffer_stream &buf, const char *name) const {
        fprintf_json_generalized_time(buf, name, value.data, value.data_end - value.data);
    }
    void print_as_json_escaped_string(struct buffer_stream &buf, const char *name) const {
        fprintf_json_string_escaped(buf, name, value.data, value.data_end - value.data);
    }

    void print_as_json_ip_address(struct buffer_stream &buf, const char *name) const {
        buf.snprintf("{\"%s\":\"", name);
        fprintf_ip_address(buf, value.data, value.data_end - value.data);
        buf.snprintf("\"}");
    }

    void print_as_json_bitstring(struct buffer_stream &buf, const char *name, bool comma=false) const {
        const char *format_string = "\"%s\":[";
        if (comma) {
            format_string = ",\"%s\":[";
        }
        buf.snprintf(format_string, name);
        if (value.data) {
            struct datum p = value;
            size_t number_of_unused_bits;
            datum_read_and_skip_uint(&p, 1, &number_of_unused_bits);
            const char *comma = "";
            while (p.data < p.data_end-1) {
                for (uint8_t x = 0x80; x > 0; x=x>>1) {
                    buf.snprintf("%s%c", comma, x & *p.data ? '1' : '0');
                    comma = ",";
                }
                p.data++;
            }
            uint8_t terminus = 0x80 >> (8-number_of_unused_bits);
            for (uint8_t x = 0x80; x > terminus; x=x>>1) {
                buf.snprintf("%s%c", comma, x & *p.data ? '1' : '0');
                comma = ",";
            }

        }
        buf.write_char(']');
    }

    void print_as_json_bitstring_flags(struct buffer_stream &buf, const char *name, char * const *flags, bool comma=false) const {
        const char *format_string = "\"%s\":[";
        if (comma) {
            format_string = ",\"%s\":[";
        }
        buf.snprintf(format_string, name);
        if (value.data) {
            struct datum p = value;
            char *const *tmp = flags;
            size_t number_of_unused_bits;
            datum_read_and_skip_uint(&p, 1, &number_of_unused_bits);
            const char *comma = "";
            while (p.data < p.data_end-1) {
                for (uint8_t x = 0x80; x > 0; x=x>>1) {
                    if (x & *p.data) {
                        buf.snprintf("%s\"%s\"", comma, *tmp);
                        comma = ",";
                    }
                    if (*tmp) {
                        tmp++;
                    }
                }
                p.data++;
            }
            uint8_t terminus = 0x80 >> (8-number_of_unused_bits);
            for (uint8_t x = 0x80; x > terminus; x=x>>1) {
                if (x & *p.data) {
                    buf.snprintf("%s\"%s\"", comma, *tmp);
                    comma = ",";
                }
                if (*tmp) {
                    tmp++;
                }
            }

        }
        buf.write_char(']');
    }

    void print_as_json(struct buffer_stream &b, const char *name) const {
        switch(tag) {
        case tlv::UTCTime:
            print_as_json_utctime(b, name);
            break;
        case tlv::GeneralizedTime:
            print_as_json_generalized_time(b, name);
            break;
        case tlv::OBJECT_IDENTIFIER:
            print_as_json_oid(b, name);
            break;
        case tlv::PRINTABLE_STRING:
        case tlv::T61String:
        case tlv::VIDEOTEX_STRING:
        case tlv::IA5String:
        case tlv::GraphicString:
        case tlv::VisibleString:
            print_as_json_escaped_string(b, name);
            break;
        case tlv::BIT_STRING:
            print_as_json_bitstring(b, name);
            break;
        default:
            print_as_json_hex(b, name);  // handle unexpected type
        }
    }
#endif // 0

    /*
     * functions for json_object serialization
     */
    void print_as_json_hex(struct json_object &o, const char *name) const {
        o.print_key_hex(name, value);
        if ((unsigned)value.length() != length) { o.print_key_string("truncated", name); }
    }

    void print_as_json_oid(struct json_object_asn1 &o, const char *name) const {
        o.print_key_oid(name, value);
        if ((unsigned)value.length() != length) { o.print_key_string("truncated", name); }
    }

    void print_as_json_escaped_string(struct json_object_asn1 &o, const char *name) const {
        o.print_key_escaped_string(name, value);
        if ((unsigned)value.length() != length) { o.print_key_string("truncated", name); }
    }

    void print_as_json_utctime(struct json_object_asn1 &o, const char *name) const {
        o.print_key_utctime(name, value.data, value.data_end - value.data);
        if ((unsigned)value.length() != length) { o.print_key_string("truncated", name); }
    }

    void print_as_json_generalized_time(struct json_object_asn1 &o, const char *name) const {
        o.print_key_generalized_time(name, value.data, value.data_end - value.data);
        if ((unsigned)value.length() != length) { o.print_key_string("truncated", name); }
    }
    void print_as_json_ip_address(struct json_object_asn1 &o, const char *name) const {
        o.write_comma(o.comma);
        o.b->snprintf("\"%s\":\"", name);
        fprintf_ip_address(*o.b, value.data, value.data_end - value.data);
        o.b->write_char('\"');
        o.comma = ',';
        if ((unsigned)value.length() != length) { o.print_key_string("truncated", name); }
    }

    void print_as_json_bitstring(struct json_object &o, const char *name, bool comma=false) const {
        const char *format_string = "\"%s\":[";
        if (comma) {
            format_string = ",\"%s\":[";
        }
        o.b->snprintf(format_string, name);
        if (value.data) {
            struct datum p = value;
            size_t number_of_unused_bits = 0;
            datum_read_and_skip_uint(&p, 1, &number_of_unused_bits);
            const char *comma = "";
            while (p.data < p.data_end-1) {
                for (uint8_t x = 0x80; x > 0; x=x>>1) {
                    o.b->snprintf("%s%c", comma, x & *p.data ? '1' : '0');
                    comma = ",";
                }
                p.data++;
            }
            uint8_t terminus = 0x80 >> (8-number_of_unused_bits);
            for (uint8_t x = 0x80; x > terminus; x=x>>1) {
                o.b->snprintf("%s%c", comma, x & *p.data ? '1' : '0');
                comma = ",";
            }

        }
        o.b->write_char(']');
        if ((unsigned)value.length() != length) { o.print_key_string("truncated", name); }
    }

    void print_as_json_bitstring_flags(struct json_object_asn1 &o, const char *name, char * const *flags) const {
        o.print_key_bitstring_flags(name, value, flags);
        if ((unsigned)value.length() != length) { o.print_key_string("truncated", name); }
    }

    void print_as_json(struct json_object_asn1 &o, const char *name) const {
        switch(tag) {
        case tlv::UTCTime:
            print_as_json_utctime(o, name);
            break;
        case tlv::GeneralizedTime:
            print_as_json_generalized_time(o, name);
            break;
        case tlv::OBJECT_IDENTIFIER:
            print_as_json_oid(o, name);
            break;
        case tlv::PRINTABLE_STRING:
        case tlv::T61String:
        case tlv::VIDEOTEX_STRING:
        case tlv::IA5String:
        case tlv::GraphicString:
        case tlv::VisibleString:
            print_as_json_escaped_string(o, name);
            break;
        case tlv::BIT_STRING:
            print_as_json_bitstring(o, name);
            break;
        default:
            print_as_json_hex(o, name);  // handle unexpected type
        }
    }

    void print_tag_as_json_hex(struct json_object &o, const char *name) const {
        struct datum p{&tag, &tag+1};
        o.print_key_hex(name, p);
    }

};

#endif /* ASN1_H */
