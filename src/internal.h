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

#ifndef LIBJSON_INTERNAL_H
#define LIBJSON_INTERNAL_H

/* Errors */
void json_set_error(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));
void json_set_error_invalid_character(unsigned char, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

/* JSON */
struct json_object_entry {
    struct json_value *key;
    struct json_value *value;
};

struct json_object {
    struct json_object_entry *entries;
    size_t nb_entries;
};

struct json_array {
    struct json_value **elements;
    size_t nb_elements;
};

struct json_value {
    enum json_type type;

    union {
        struct json_object object;
        struct json_array array;
        int64_t integer;
        double real;
        char *string;
        bool boolean;
    } u;
};

struct json_value *json_value_new(enum json_type);

#endif

