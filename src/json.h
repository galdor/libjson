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

struct json_memory_allocator {
   void *(*malloc)(size_t sz);
   void (*free)(void *ptr);
   void *(*calloc)(size_t nb, size_t sz);
   void *(*realloc)(void *ptr, size_t sz);
};

extern const struct json_memory_allocator *json_default_memory_allocator;

void json_set_memory_allocator(const struct json_memory_allocator *allocator);

const char *json_get_error(void);

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

int json_parse(const char *, size_t, struct json_value **);
void json_value_delete(struct json_value *);

enum json_format_option {
    JSON_FORMAT_INDENT         = 0x01,
    JSON_FORMAT_ESCAPE_SOLIDUS = 0x02,
};

char *json_value_format(const struct json_value *, int, size_t *);

struct json_value *json_object_new(void);
size_t json_object_get_nb_entries(struct json_value *);
int json_object_add_entry(struct json_value *, struct json_value *,
                          struct json_value *);

struct json_value *json_array_new(void);
size_t json_array_get_nb_elements(struct json_value *);
int json_array_add_element(struct json_value *, struct json_value *);

struct json_value *json_integer_new(int64_t);

struct json_value *json_real_new(double);

struct json_value *json_string_new(const char *);
struct json_value *json_string_new2(const char *, size_t);

struct json_value *json_boolean_new(bool);

struct json_value *json_null_new(void);

#endif

