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

/* Memory */
struct json_memory_allocator {
   void *(*malloc)(size_t sz);
   void (*free)(void *ptr);
   void *(*calloc)(size_t nb, size_t sz);
   void *(*realloc)(void *ptr, size_t sz);
};

extern const struct json_memory_allocator *json_default_memory_allocator;

void json_set_memory_allocator(const struct json_memory_allocator *allocator);

void *json_malloc(size_t);
void json_free(void *);
void *json_calloc(size_t, size_t);
void *json_realloc(void *, size_t);

/* Errors */
const char *json_get_error(void);

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

struct json_value *json_parse(const char *, size_t);

void json_value_delete(struct json_value *);
struct json_value *json_value_clone(const struct json_value *);

enum json_type json_value_type(const struct json_value *);

enum json_format_option {
    JSON_FORMAT_INDENT         = 0x01,
    JSON_FORMAT_ESCAPE_SOLIDUS = 0x02,
};

char *json_value_format(const struct json_value *, uint32_t, size_t *);

struct json_value *json_object_new(void);
size_t json_object_nb_entries(struct json_value *);
bool json_object_has_entry(struct json_value *, const char *);
bool json_object_has_entry2(struct json_value *, const char *, size_t);
struct json_value *json_object_entry(struct json_value *, const char *);
struct json_value *json_object_entry2(struct json_value *,
                                      const char *, size_t);
int json_object_add_entry(struct json_value *, struct json_value *,
                          struct json_value *);

struct json_value *json_array_new(void);
size_t json_array_nb_elements(struct json_value *);
struct json_value *json_array_element(struct json_value *, size_t);
int json_array_add_element(struct json_value *, struct json_value *);

struct json_value *json_integer_new(int64_t);
int64_t json_integer_value(struct json_value *);

struct json_value *json_real_new(double);
double json_real_value(struct json_value *);

struct json_value *json_string_new(const char *);
struct json_value *json_string_new2(const char *, size_t);
const char *json_string_value(struct json_value *);
size_t json_string_length(struct json_value *);

struct json_value *json_boolean_new(bool);
bool json_boolean_value(struct json_value *);

struct json_value *json_null_new(void);

#endif

