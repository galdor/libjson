/*
 * Copyright (c) 2014 Nicolas Martyanoff
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "json.h"
#include "internal.h"

struct json_parser {
    const char *ptr;
    size_t len;

    uint32_t options;
};

static void json_parser_skip(struct json_parser *, size_t);
static void json_parser_skip_ws(struct json_parser *);

static int json_parse_value(struct json_parser *, struct json_value **);
static int json_parse_object(struct json_parser *, struct json_value **);
static int json_parse_array(struct json_parser *, struct json_value **);
static int json_parse_number(struct json_parser *, struct json_value **);
static int json_parse_string(struct json_parser *, struct json_value **);
static int json_parse_literal(struct json_parser *, struct json_value **);

static bool json_is_ws(int);
static bool json_is_boundary(int);
static bool json_is_number_first_char(int);
static bool json_is_number_char(int);
static bool json_is_integer_char(int);
static bool json_is_real_char(int);

static char *json_decode_string(const char *, size_t, size_t *);
static int json_decode_utf8_character(const char *, uint32_t *);
static int json_decode_utf16_surrogate_pair(const char *, uint32_t *);
static int json_write_codepoint_as_utf8(uint32_t, char *, size_t *);
static int json_decode_hex_digit(unsigned char);

struct json_value *
json_parse(const char *buf, size_t sz, uint32_t options) {
    struct json_parser parser;
    struct json_value *value;

    memset(&parser, 0, sizeof(struct json_parser));

    parser.ptr = buf;
    parser.len = sz;
    parser.options = options;

    if (json_parse_value(&parser, &value) == -1)
        return NULL;

    if (value->type != JSON_OBJECT && value->type != JSON_ARRAY) {
        c_set_error("top-level value is neither an object nor an array");
        json_value_delete(value);
        return NULL;
    }

    return value;
}

static void
json_parser_skip(struct json_parser *parser, size_t n) {
    if (n > parser->len)
        n = parser->len;

    parser->ptr += n;
    parser->len -= n;
}

static void
json_parser_skip_ws(struct json_parser *parser) {
    while (parser->len > 0 && json_is_ws((unsigned char)*parser->ptr)) {
        parser->ptr++;
        parser->len--;
    }
}

static int
json_parse_value(struct json_parser *parser, struct json_value **pvalue) {
    json_parser_skip_ws(parser);

    if (*parser->ptr == '{') {
        return json_parse_object(parser, pvalue);
    } else if (*parser->ptr == '[') {
        return json_parse_array(parser, pvalue);
    } else if (*parser->ptr == '"') {
        return json_parse_string(parser, pvalue);
    } else if (*parser->ptr == 't' || *parser->ptr == 'f'
            || *parser->ptr == 'n') {
        return json_parse_literal(parser, pvalue);
    } else if (json_is_number_first_char((unsigned char)*parser->ptr)) {
        return json_parse_number(parser, pvalue);
    } else {
        json_set_error_invalid_character((unsigned char)*parser->ptr, " ");
        return -1;
    }
}

static int
json_parse_object(struct json_parser *parser, struct json_value **pvalue) {
    struct json_value *object_value;

    object_value = json_object_new();
    if (!object_value)
        return -1;

    json_parser_skip(parser, 1); /* '{' */
    if (parser->len == 0) {
        c_set_error("truncated object");
        goto error;
    }

    while (parser->len > 0) {
        struct json_value *key;
        struct json_value *value;
        int ret;

        json_parser_skip_ws(parser);

        if (*parser->ptr == '}') {
            if (object_value->u.object.nb_members > 0) {
                c_set_error("truncated object");
                goto error;
            }

            break;
        }

        ret = json_parse_value(parser, &key);
        if (ret == -1)
            goto error;

        if (json_value_type(key) != JSON_STRING) {
            c_set_error("key in object member is not a string");
            goto error;
        }

        json_parser_skip_ws(parser);

        if (*parser->ptr != ':') {
            json_set_error_invalid_character((unsigned char)*parser->ptr,
                                             " in object");
            json_value_delete(key);
            goto error;
        }

        json_parser_skip(parser, 1); /* ':' */
        json_parser_skip_ws(parser);
        if (parser->len == 0) {
            json_value_delete(key);
            c_set_error("truncated object");
            goto error;
        }

        ret = json_parse_value(parser, &value);
        if (ret == -1) {
            json_value_delete(key);
            goto error;
        }

        if (parser->options & JSON_PARSE_REJECT_DUPLICATE_KEYS) {
            if (json_object_has_member2(object_value,
                                        key->u.string.ptr,
                                        key->u.string.len)) {
                c_set_error("duplicate object key");
                json_value_delete(key);
                json_value_delete(value);
                goto error;
            }
        }

        if (json_object_add_member2(object_value, key->u.string.ptr,
                                    key->u.string.len, value) == -1) {
            json_value_delete(key);
            json_value_delete(value);
            goto error;
        }

        json_value_delete(key);

        json_parser_skip_ws(parser);
        if (parser->len == 0) {
            c_set_error("truncated object");
            goto error;
        }

        if (*parser->ptr == ',') {
            json_parser_skip(parser, 1);
        } else if (*parser->ptr == '}') {
            break;
        } else {
            json_set_error_invalid_character((unsigned char)*parser->ptr,
                                             " in object");
            goto error;
        }
    }

    if (*parser->ptr != '}') {
        c_set_error("truncated object");
        goto error;
    }

    json_parser_skip(parser, 1); /* '}' */

    *pvalue = object_value;
    return 1;

error:
    json_value_delete(object_value);
    return -1;
}

static int
json_parse_array(struct json_parser *parser, struct json_value **pvalue) {
    struct json_value *value;

    value = json_array_new();
    if (!value)
        return -1;

    json_parser_skip(parser, 1); /* '[' */
    if (parser->len == 0) {
        c_set_error("truncated array");
        goto error;
    }

    while (parser->len > 0) {
        struct json_value *element;
        int ret;

        json_parser_skip_ws(parser);

        if (*parser->ptr == ']') {
            if (value->u.array.nb_elements > 0) {
                c_set_error("truncated array");
                goto error;
            }

            break;
        }

        ret = json_parse_value(parser, &element);
        if (ret == -1)
            goto error;

        if (json_array_add_element(value, element) == -1) {
            json_value_delete(element);
            goto error;
        }

        json_parser_skip_ws(parser);
        if (parser->len == 0) {
            c_set_error("truncated array");
            goto error;
        }

        if (*parser->ptr == ',') {
            json_parser_skip(parser, 1);
        } else if (*parser->ptr == ']') {
            break;
        } else {
            json_set_error_invalid_character((unsigned char)*parser->ptr,
                                             " in array");
            goto error;
        }
    }

    if (*parser->ptr != ']') {
        c_set_error("truncated array");
        goto error;
    }

    json_parser_skip(parser, 1); /* ']' */

    *pvalue = value;
    return 1;

error:
    json_value_delete(value);
    return -1;
}

static int
json_parse_number(struct json_parser *parser, struct json_value **pvalue) {
    struct json_value *value;
    const char *ptr, *start;
    size_t len, toklen;
    enum json_type type;
    bool found;

    /* Search for the decimal point if there is one */
    ptr = parser->ptr;
    len = parser->len;

    found = false;
    while (len > 0) {
        if (*ptr == '.' || *ptr == 'e' || *ptr == 'E') {
            type = JSON_REAL;
            found = true;
            break;
        } else if (!json_is_number_char((unsigned char)*ptr)) {
            type = JSON_INTEGER;
            found = true;
            break;
        }

        ptr++;
        len--;
    }

    if (!found) {
        c_set_error("truncated number");
        return -1;
    }

    start = parser->ptr;

    if (type == JSON_INTEGER) {
        char tmp[21]; /* INT64_MIN contains 20 digits */
        long long llval;

        while (parser->len > 0) {
            if (json_is_integer_char((unsigned char)*parser->ptr)) {
                json_parser_skip(parser, 1);
            } else if (json_is_boundary((unsigned char)*parser->ptr)) {
                break;
            } else {
                json_set_error_invalid_character((unsigned char)*parser->ptr,
                                                 " in integer");
                return -1;
            }
        }

        toklen = (size_t)(parser->ptr - start);
        if (toklen >= sizeof(tmp)) {
            c_set_error("integer too long");
            return -1;
        }

        memcpy(tmp, start, toklen);
        tmp[toklen] = '\0';

        errno = 0;
        llval = strtoll(tmp, NULL, 10);
        if (errno) {
            c_set_error("%s", strerror(errno));
            return -1;
        }

        value = json_integer_new((int64_t)llval);
        if (!value)
            return -1;
    } else if (type == JSON_REAL) {
        char tmp[64];
        double real;

        while (parser->len > 0) {
            if (json_is_real_char((unsigned char)*parser->ptr)) {
                json_parser_skip(parser, 1);
            } else if (json_is_boundary((unsigned char)*parser->ptr)) {
                break;
            } else {
                json_set_error_invalid_character((unsigned char)*parser->ptr,
                                                 " in real");
                return -1;
            }
        }

        toklen = (size_t)(parser->ptr - start);
        if (toklen >= sizeof(tmp)) {
            c_set_error("real too long");
            return -1;
        }

        memcpy(tmp, start, toklen);
        tmp[toklen] = '\0';

        errno = 0;
        real = strtod(tmp, NULL);
        if (errno) {
            c_set_error("%s", strerror(errno));
            return -1;
        }

        value = json_real_new(real);
        if (!value)
            return -1;
    } else {
        /* Should never happen */
        c_set_error("unknown number type %d", type);
        return -1;
    }

    *pvalue = value;
    return 1;
}

static int
json_parse_string(struct json_parser *parser, struct json_value **pvalue) {
    struct json_value *value;
    const char *start;
    size_t toklen;

    value = json_value_new(JSON_STRING);
    if (!value)
        return -1;

    json_parser_skip(parser, 1); /* '"' */
    start = parser->ptr;

    while (parser->len > 0) {
        if (*parser->ptr == '\\') {
            if (parser->len < 2) {
                c_set_error("truncated escape sequence");
                json_value_delete(value);
                return -1;
            }

            json_parser_skip(parser, 2);
            continue;
        } else if (*parser->ptr == '"') {
            break;
        }

        json_parser_skip(parser, 1);
    }

    if (*parser->ptr != '"') {
        c_set_error("truncated string");
        json_value_delete(value);
        return -1;
    }

    toklen = (size_t)(parser->ptr - start);

    value->u.string.ptr = json_decode_string(start, toklen,
                                             &value->u.string.len);
    if (!value->u.string.ptr) {
        json_value_delete(value);
        return -1;
    }

    json_parser_skip(parser, 1); /* '"' */

    *pvalue = value;
    return 1;
}

static int
json_parse_literal(struct json_parser *parser, struct json_value **pvalue) {
    struct json_value *value;
    size_t length;

    if (*parser->ptr == 't' && parser->len >= 4
     && memcmp(parser->ptr, "true", 4) == 0) {
        value = json_boolean_new(true);
        if (!value)
            return -1;

        length = 4;
    } else if (*parser->ptr == 'f' && parser->len >= 5
     && memcmp(parser->ptr, "false", 5) == 0) {
        value = json_boolean_new(false);
        if (!value)
            return -1;

        length = 5;
    } else if (*parser->ptr == 'n' && parser->len >= 4
     && memcmp(parser->ptr, "null", 4) == 0) {
        value = json_null_new();
        if (!value)
            return -1;

        length = 4;
    } else {
        c_set_error("unknown literal");
        return -1;
    }

    parser->ptr += length;
    parser->len -= length;

    *pvalue = value;
    return 1;
}

static bool
json_is_ws(int c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static bool
json_is_boundary(int c) {
    return json_is_ws(c) || c == ',' || c == ']' || c == ':' || c == '}';
}

static bool
json_is_number_first_char(int c) {
    return c == '-' || (c >= '0' && c <= '9');
}

static bool
json_is_number_char(int c) {
    return json_is_real_char(c);
}

static bool
json_is_integer_char(int c) {
    return (c >= '0' && c <= '9') || c == '-';
}

static bool
json_is_real_char(int c) {
    return (c >= '0' && c <= '9')
        || c == '-' || c == '+'
        || c == '.'
        || c == 'e' || c == 'E';
}

static char *
json_decode_string(const char *buf, size_t sz, size_t *plen) {
    const char *iptr;
    char *string, *optr;
    size_t ilen = sz;

    /* A decoded string has a length smaller or equal to the length of an
     * encoded string. */
    string = c_malloc(sz + 1);
    if (!string)
        return NULL;

    iptr = buf;
    ilen = sz;
    optr = string;

    while (ilen > 0) {
        if (*iptr == '\\') {
            if (ilen < 2) {
                c_set_error("truncated escaped character");
                goto error;
            }

            iptr++;
            ilen--;

            if (*iptr == '\\' || *iptr == '/' || *iptr == '"') {
                *optr++ = *iptr++;
                ilen--;
            } else if (*iptr == 'b') {
                *optr++ = '\b';
                iptr++;
                ilen--;
            } else if (*iptr == 'f') {
                *optr++ = '\f';
                iptr++;
                ilen--;
            } else if (*iptr == 'n') {
                *optr++ = '\n';
                iptr++;
                ilen--;
            } else if (*iptr == 'r') {
                *optr++ = '\r';
                iptr++;
                ilen--;
            } else if (*iptr == 't') {
                *optr++ = '\t';
                iptr++;
                ilen--;
            } else if (*iptr == 'u' || *iptr == 'U') {
                size_t nb_written;
                uint32_t codepoint;

                iptr++;
                ilen--;

                if (ilen < 4) {
                    c_set_error("truncated escaped unicode character");
                    goto error;
                }

                if (json_decode_utf8_character(iptr, &codepoint) == -1)
                    goto error;

                if (codepoint >= 0xd800 && codepoint <= 0xdfff) {
                    /* UTF-16 surrogate pair */
                    if (ilen < 10 || iptr[4] != '\\'
                     || (iptr[5] != 'u' && iptr[5] != 'U')) {
                        c_set_error("truncated escaped surrogate pair");
                        goto error;
                    }

                    if (json_decode_utf16_surrogate_pair(iptr,
                                                         &codepoint) == -1) {
                        goto error;
                    }

                    if (json_write_codepoint_as_utf8(codepoint, optr,
                                                     &nb_written) == -1) {
                        goto error;
                    }

                    iptr += 10;
                    ilen -= 10;
                    optr += nb_written;
                } else {
                    if (json_write_codepoint_as_utf8(codepoint, optr,
                                                     &nb_written) == -1) {
                        goto error;
                    }

                    iptr += 4;
                    ilen -= 4;
                    optr += nb_written;
                }
            } else {
                c_set_error("invalid escape sequence");
                goto error;
            }
        } else {
            *optr++ = *iptr++;
            ilen--;
        }
    }

    *optr = '\0';
    *plen = (size_t)(optr - string);

    return string;

error:
    c_free(string);
    return NULL;
}

static int
json_decode_utf8_character(const char *str, uint32_t *pcodepoint) {
    int d1, d2, d3, d4;

#define JSON_READ_HEX_DIGIT(var_, c_)                              \
    if ((var_ = json_decode_hex_digit((unsigned char)c_)) == -1) { \
        json_set_error_invalid_character((unsigned char)c_,        \
                                         " in unicode sequence");  \
        return -1;                                                 \
    }

    JSON_READ_HEX_DIGIT(d1, str[0]);
    JSON_READ_HEX_DIGIT(d2, str[1]);
    JSON_READ_HEX_DIGIT(d3, str[2]);
    JSON_READ_HEX_DIGIT(d4, str[3]);
#undef JSON_READ_HEX_DIGIT

    *pcodepoint = (uint32_t)((d1 << 12) | (d2 << 8) | (d3 << 4) | d4);
    return 0;
}

static int
json_decode_utf16_surrogate_pair(const char *str, uint32_t *pcodepoint) {
    int d1, d2, d3, d4;
    uint32_t hi, lo;

#define JSON_READ_HEX_DIGIT(var_, c_)                              \
    if ((var_ = json_decode_hex_digit((unsigned char)c_)) == -1) { \
        json_set_error_invalid_character((unsigned char)c_,        \
                                         " in unicode sequence");  \
        return -1;                                                 \
    }

    JSON_READ_HEX_DIGIT(d1, str[0]);
    JSON_READ_HEX_DIGIT(d2, str[1]);
    JSON_READ_HEX_DIGIT(d3, str[2]);
    JSON_READ_HEX_DIGIT(d4, str[3]);
    hi = (uint32_t)((d1 << 12) | (d2 << 8) | (d3 << 4) | d4);

    JSON_READ_HEX_DIGIT(d1, str[6]);
    JSON_READ_HEX_DIGIT(d2, str[7]);
    JSON_READ_HEX_DIGIT(d3, str[8]);
    JSON_READ_HEX_DIGIT(d4, str[9]);
    lo = (uint32_t)((d1 << 12) | (d2 << 8) | (d3 << 4) | d4);
#undef JSON_READ_HEX_DIGIT

    *pcodepoint = 0x010000 + (((hi - 0xd800) << 10) | (lo - 0xdc00));
    return 0;
}

static int
json_write_codepoint_as_utf8(uint32_t codepoint, char *out,
                             size_t *p_nb_written) {
    size_t nb_bytes;

    if (codepoint <= 0x007f) {
        nb_bytes = 1;

        /* 0xxx xxxx */
        *out++ = (char)(codepoint & 0x7f);
    } else if (codepoint <= 0x07ff) {
        nb_bytes = 2;

        /* 110x xxxx    10xx xxxx */
        *out++ = (char)(0xc0 | ((codepoint >> 6) & 0x1f));
        *out++ = (char)(0x80 | (codepoint & 0x3f));
    } else if (codepoint <= 0xffff) {
        nb_bytes = 3;

        /* 1110 xxxx    10xx xxxx    10xx xxxx */
        *out++ = (char)(0xe0 | ((codepoint >> 12) & 0x0f));
        *out++ = (char)(0x80 | ((codepoint >> 6) & 0x3f));
        *out++ = (char)(0x80 | (codepoint & 0x3f));
    } else if (codepoint <= 0x1fffff) {
        nb_bytes = 4;

        /* 1111 0xxx    10xx xxxx    10xx xxxx    10xx xxxx */
        *out++ = (char)(0xf0 | ((codepoint >> 18) & 0x07));
        *out++ = (char)(0x80 | ((codepoint >> 12) & 0x3f));
        *out++ = (char)(0x80 | ((codepoint >> 6) & 0x3f));
        *out++ = (char)(0x80 | (codepoint & 0x3f));
    } else if (codepoint <= 0x3ffffff) {
        nb_bytes = 5;

        /* 1111 10xx    10xx xxxx    10xx xxxx    10xx xxxx    10xx xxxx */
        *out++ = (char)(0xf8 | ((codepoint >> 24) & 0xfc));
        *out++ = (char)(0x80 | ((codepoint >> 18) & 0x3f));
        *out++ = (char)(0x80 | ((codepoint >> 12) & 0x3f));
        *out++ = (char)(0x80 | ((codepoint >> 6) & 0x3f));
        *out++ = (char)(0x80 | (codepoint & 0x3f));
    } else if (codepoint <= 0x7fffffff) {
        nb_bytes = 6;

        /* 1111 110x    10xx xxxx    10xx xxxx    10xx xxxx    10xx xxxx
         * 10xx xxxx */
        *out++ = (char)(0xfc | ((codepoint >> 30) & 0x01));
        *out++ = (char)(0x80 | ((codepoint >> 24) & 0x3f));
        *out++ = (char)(0x80 | ((codepoint >> 18) & 0x3f));
        *out++ = (char)(0x80 | ((codepoint >> 12) & 0x3f));
        *out++ = (char)(0x80 | ((codepoint >> 6) & 0x3f));
        *out++ = (char)(0x80 | (codepoint & 0x3f));
    } else {
        c_set_error("invalid unicode codepoint U+%X", codepoint);
        return -1;
    }

    *p_nb_written = nb_bytes;
    return 0;
}

static int
json_decode_hex_digit(unsigned char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return 10 + c - 'a';
    } else if (c >= 'A' && c <= 'F') {
        return 10 + c - 'A';
    } else {
        return -1;
    }

    return c;
}
