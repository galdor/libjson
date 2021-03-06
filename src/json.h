/*
 * Copyright (c) 2014-2015 Nicolas Martyanoff
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

#include <stdbool.h>
#include <stdint.h>
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

const char *json_type_to_string(enum json_type);

struct json_value;

enum json_parse_option {
    JSON_PARSE_DEFAULT                = 0,

    JSON_PARSE_REJECT_DUPLICATE_KEYS  = (1 << 1),
    JSON_PARSE_REJECT_NULL_CHARACTERS = (1 << 2),
};

struct json_value *json_parse(const char *, size_t, uint32_t);
struct json_value *json_parse_string(const char *, uint32_t);
struct json_value *json_parse_fd(int, uint32_t);
struct json_value *json_parse_file(const char *, uint32_t);

void json_value_delete(struct json_value *);
struct json_value *json_value_clone(const struct json_value *);
bool json_value_equal(struct json_value *, struct json_value *);

enum json_type json_value_type(const struct json_value *);

bool json_value_is_object(const struct json_value *);
bool json_value_is_array(const struct json_value *);
bool json_value_is_integer(const struct json_value *);
bool json_value_is_real(const struct json_value *);
bool json_value_is_number(const struct json_value *);
bool json_value_is_string(const struct json_value *);
bool json_value_is_boolean(const struct json_value *);
bool json_value_is_null(const struct json_value *);

enum json_format_option {
    JSON_FORMAT_DEFAULT        = 0,

    JSON_FORMAT_INDENT         = (1 << 0),
    JSON_FORMAT_COLOR_ANSI     = (1 << 1),
    JSON_FORMAT_ESCAPE_SOLIDUS = (1 << 2),
};

int json_value_format_to_buffer(struct json_value *, struct c_buffer *,
                                uint32_t);
char *json_value_format(struct json_value *, uint32_t, size_t *);

struct json_value *json_object_new(void);
size_t json_object_nb_members(const struct json_value *);
bool json_object_has_member(const struct json_value *, const char *);
bool json_object_has_member2(const struct json_value *, const char *, size_t);
struct json_value *json_object_member(const struct json_value *, const char *);
struct json_value *json_object_member2(const struct json_value *,
                                       const char *, size_t);
const char *json_object_nth_member(const struct json_value *, size_t,
                                   struct json_value **);
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
void json_object_merge(struct json_value *, const struct json_value *);

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
struct json_value *json_string_new_nocopy(char *);
struct json_value *json_string_new_nocopy2(char *, size_t);
struct json_value *json_string_new_vprintf(const char *, va_list);
struct json_value *json_string_new_printf(const char *, ...)
    __attribute__ ((format(printf, 1, 2)));
const char *json_string_value(const struct json_value *);
size_t json_string_length(const struct json_value *);
char *json_string_dup(const struct json_value *);

struct json_value *json_boolean_new(bool);
bool json_boolean_value(const struct json_value *);

struct json_value *json_null_new(void);

/* JSON schema */
struct json_schema *json_schema_parse(const char *, size_t);
struct json_schema *json_schema_parse_string(const char *);
struct json_schema *json_schema_parse_fd(int);
struct json_schema *json_schema_parse_file(const char *);
void json_schema_delete(struct json_schema *);

int json_schema_validate(struct json_schema *, struct json_value *);

#endif
