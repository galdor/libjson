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

#include <alloca.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <string.h>

#include <pcre.h>

#include "json.h"

/* ------------------------------------------------------------------------
 *  Errors
 * ------------------------------------------------------------------------ */
void json_set_error_invalid_character(unsigned char, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

/* ------------------------------------------------------------------------
 *  JSON
 * ------------------------------------------------------------------------ */
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

/* ------------------------------------------------------------------------
 *  JSON schema
 * ------------------------------------------------------------------------ */
/* URI */
enum json_schema_uri {
    JSON_SCHEMA_URI_CURRENT,
    JSON_SCHEMA_URI_DRAFT_V4,
};

int json_schema_uri_parse(const char *, enum json_schema_uri *);

/* Simple type */
enum json_schema_simple_type {
    JSON_SCHEMA_SIMPLE_ARRAY,
    JSON_SCHEMA_SIMPLE_BOOLEAN,
    JSON_SCHEMA_SIMPLE_INTEGER,
    JSON_SCHEMA_SIMPLE_NULL,
    JSON_SCHEMA_SIMPLE_NUMBER,
    JSON_SCHEMA_SIMPLE_OBJECT,
    JSON_SCHEMA_SIMPLE_STRING,
};

int json_schema_simple_type_parse(const char *, enum json_schema_simple_type *);
bool json_schema_simple_type_matches_type(enum json_schema_simple_type,
                                          enum json_type);

/* Format */
enum json_schema_format {
    JSON_SCHEMA_FORMAT_DATE_TIME,
    JSON_SCHEMA_FORMAT_EMAIL,
    JSON_SCHEMA_FORMAT_HOSTNAME,
    JSON_SCHEMA_FORMAT_IPV4,
    JSON_SCHEMA_FORMAT_IPV6,
    JSON_SCHEMA_FORMAT_URI,
    JSON_SCHEMA_FORMAT_REGEX,
};

int json_schema_format_parse(const char *, enum json_schema_format *);

/* Generic validator */
struct json_generic_validator {
    struct c_vector *types; /* enum json_schema_simple_type */

    struct c_ptr_vector *enumeration; /* struct json_value * */

    struct c_ptr_vector *all_of; /* struct json_schema * */
    struct c_ptr_vector *any_of; /* struct json_schema * */
    struct c_ptr_vector *one_of; /* struct json_schema * */
    struct json_schema *not;

    enum json_schema_format format;
};

void json_generic_validator_init(struct json_generic_validator *);
void json_generic_validator_free(struct json_generic_validator *);

int json_generic_validator_check(struct json_generic_validator *,
                                 struct json_value *);

/* Numeric validator */
struct json_numeric_validator {
    struct json_value *multiple_of;

    struct json_value *min;
    bool exclusive_min;
    struct json_value *max;
    bool exclusive_max;
};

void json_numeric_validator_init(struct json_numeric_validator *);
void json_numeric_validator_free(struct json_numeric_validator *);

int json_numeric_validator_check(struct json_numeric_validator *,
                                 struct json_value *);

/* String validator */
struct json_string_validator {
    bool has_min_length;
    size_t min_length;
    bool has_max_length;
    size_t max_length;

    char *pattern;
    pcre *pattern_re;
};

void json_string_validator_init(struct json_string_validator *);
void json_string_validator_free(struct json_string_validator *);

int json_string_validator_check(struct json_string_validator *,
                                struct json_value *);

/* Array validator */
struct json_array_validator {
    bool has_min_items;
    size_t min_items;
    bool has_max_items;
    size_t max_items;

    bool unique_items;

    union {
        struct json_schema *schema;
        bool boolean;
    } additional_items;
    bool additional_items_is_schema;

    struct c_ptr_vector *items; /* struct json_schema * */
    bool items_is_array;
};

void json_array_validator_init(struct json_array_validator *);
void json_array_validator_free(struct json_array_validator *);

int json_array_validator_check(struct json_array_validator *,
                               struct json_value *);

/* Object validator */
struct json_object_validator_property {
    char *string;

    struct json_schema *schema;
};

struct json_object_validator_pattern {
    char *pattern;
    pcre *pattern_re;

    struct json_schema *schema;
};

struct json_object_validator {
    bool has_min_properties;
    size_t min_properties;
    bool has_max_properties;
    size_t max_properties;

    struct c_ptr_vector *required; /* char * */

    struct c_vector *properties; /* struct json_object_validator_property * */

    union {
        struct json_schema *schema;
        bool boolean;
    } additional_properties;
    bool additional_properties_is_schema;

    struct c_vector *pattern_properties; /* struct json_object_validator_pattern */

    struct c_hash_table *schema_dependencies; /* name -> schema */
    struct c_hash_table *property_dependencies; /* name -> string vector */
};

void json_object_validator_init(struct json_object_validator *);
void json_object_validator_free(struct json_object_validator *);

int json_object_validator_check(struct json_object_validator *,
                               struct json_value *);

/* Validator */
struct json_validator {
    struct c_hash_table *definitions; /* name -> schema */

    struct json_generic_validator generic;
    struct json_numeric_validator numeric;
    struct json_string_validator string;
    struct json_array_validator array;
    struct json_object_validator object;
};

void json_validator_init(struct json_validator *);
void json_validator_free(struct json_validator *);

int json_validator_check(struct json_validator *, struct json_value *);

/* Schema */
struct json_schema {
    char *id; /* uri */
    enum json_schema_uri schema_uri; /* $schema */

    char *title;
    char *description;
    struct json_value *default_value;

    struct json_validator validator;

    /* TODO refcount */
};

struct json_schema *json_schema_new(void);

#endif
