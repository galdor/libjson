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

#ifndef LIBJSON_JSON_H
#define LIBJSON_JSON_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#include <core.h>

/* JSON */
enum json_type {
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_INTEGER,
    JSON_REAL,
    JSON_STRING,
    JSON_BOOLEAN,
    JSON_NULL,
};

struct json_value;

enum json_parse_option {
    JSON_PARSE_DEFAULT = 0x00,
};

struct json_value *json_parse(const char *, size_t, uint32_t);

void json_value_delete(struct json_value *);
struct json_value *json_value_clone(const struct json_value *);

enum json_type json_value_type(const struct json_value *);

enum json_format_option {
    JSON_FORMAT_DEFAULT        = 0x00,
    JSON_FORMAT_INDENT         = 0x01,
    JSON_FORMAT_COLOR_ANSI     = 0x02,
    JSON_FORMAT_ESCAPE_SOLIDUS = 0x04,
};

char *json_value_format(const struct json_value *, uint32_t, size_t *);

struct json_value *json_object_new(void);
size_t json_object_nb_members(const struct json_value *);
bool json_object_has_member(const struct json_value *, const char *);
bool json_object_has_member2(const struct json_value *, const char *, size_t);
struct json_value *json_object_member(const struct json_value *, const char *);
struct json_value *json_object_member2(const struct json_value *,
                                       const char *, size_t);
int json_object_add_member2(struct json_value *, const char *, size_t,
                            struct json_value *);
int json_object_add_member(struct json_value *, const char *,
                           struct json_value *);
int json_object_set_member2(struct json_value *, const char *, size_t,
                            struct json_value *);
int json_object_set_member(struct json_value *, const char *,
                           struct json_value *);
void json_object_remove_member2(struct json_value *, const char *, size_t);
void json_object_remove_member(struct json_value *, const char *);

struct json_object_iterator *json_object_iterate(struct json_value *);
void json_object_iterator_delete(struct json_object_iterator *);
int json_object_iterator_get_next(struct json_object_iterator *,
                                  struct json_value **, struct json_value **);

struct json_value *json_array_new(void);
size_t json_array_nb_elements(const struct json_value *);
struct json_value *json_array_element(const struct json_value *, size_t);
int json_array_add_element(struct json_value *, struct json_value *);

struct json_value *json_integer_new(int64_t);
int64_t json_integer_value(const struct json_value *);

struct json_value *json_real_new(double);
double json_real_value(const struct json_value *);

struct json_value *json_string_new(const char *);
struct json_value *json_string_new2(const char *, size_t);
const char *json_string_value(const struct json_value *);
size_t json_string_length(const struct json_value *);

struct json_value *json_boolean_new(bool);
bool json_boolean_value(const struct json_value *);

struct json_value *json_null_new(void);

#endif

