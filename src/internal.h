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
void json_set_error_invalid_character(unsigned char, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

/* JSON */
struct json_object_member {
    struct json_value *key;
    struct json_value *value;
    size_t index;
};

enum json_object_sort_mode {
    JSON_OBJECT_UNSORTED = 0,
    JSON_OBJECT_SORTED_BY_INDEX,
    JSON_OBJECT_SORTED_BY_KEY,
    JSON_OBJECT_SORTED_BY_KEY_VALUE,
};

struct json_object {
    struct json_object_member *members;
    size_t nb_members;
    enum json_object_sort_mode sort_mode;
};

void json_object_sort_by_index(struct json_object *);
void json_object_sort_by_key(struct json_object *);
void json_object_sort_by_key_value(struct json_object *);

struct json_object_iterator {
    struct json_object *object;
    size_t index;
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
        struct {
            char *ptr;
            size_t len;
        } string;
        bool boolean;
    } u;
};

struct json_value *json_value_new(enum json_type);

void json_value_sort_objects_by_index(struct json_value *);

/* JSON schema */
struct json_schema {
    char *title;
};

#endif
