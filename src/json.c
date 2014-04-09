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

struct json_value *
json_value_new(enum json_type type) {
    struct json_value *value;

    value = json_malloc(sizeof(struct json_value));
    memset(value, 0, sizeof(struct json_value));

    value->type = type;

    return value;
}

void
json_value_delete(struct json_value *value) {
    if (!value)
        return;

    switch (value->type) {
    case JSON_OBJECT:
        for (size_t i = 0; i < value->u.object.nb_entries; i++) {
            json_value_delete(value->u.object.entries[i].key);
            json_value_delete(value->u.object.entries[i].value);
        }
        json_free(value->u.object.entries);
        break;

    case JSON_ARRAY:
        for (size_t i = 0; i < value->u.array.nb_elements; i++)
            json_value_delete(value->u.array.elements[i]);
        json_free(value->u.array.elements);
        break;

    case JSON_STRING:
        json_free(value->u.string.ptr);
        break;

    default:
        break;
    }

    memset(value, 0, sizeof(struct json_value));
    json_free(value);
}

struct json_value *
json_value_clone(const struct json_value *value) {
    switch (value->type) {
    case JSON_OBJECT:
    {
        struct json_value *nvalue;

        nvalue = json_object_new();

        for (size_t i = 0; i < value->u.object.nb_entries; i++) {
            struct json_value *key, *val;

            key = json_value_clone(value->u.object.entries[i].key);
            if (!key) {
                json_value_delete(nvalue);
                return NULL;
            }

            val = json_value_clone(value->u.object.entries[i].value);
            if (!val) {
                json_value_delete(key);
                json_value_delete(nvalue);
                return NULL;
            }

            if (json_object_add_entry(nvalue, key, val) == -1) {
                json_value_delete(key);
                json_value_delete(val);
                json_value_delete(nvalue);
                return NULL;
            }
        }

        return nvalue;
    }

    case JSON_ARRAY:
    {
        struct json_value *nvalue;

        nvalue = json_array_new();

        for (size_t i = 0; i < value->u.array.nb_elements; i++) {
            struct json_value *element;

            element = json_value_clone(value->u.array.elements[i]);
            if (!element) {
                json_value_delete(nvalue);
                return NULL;
            }

            if (json_array_add_element(nvalue, element) == -1) {
                json_value_delete(element);
                json_value_delete(nvalue);
                return NULL;
            }
        }

        return nvalue;
    }

    case JSON_INTEGER:
        return json_integer_new(value->u.integer);

    case JSON_REAL:
        return json_real_new(value->u.real);

    case JSON_STRING:
        return json_string_new2(value->u.string.ptr, value->u.string.len);

    case JSON_BOOLEAN:
        return json_boolean_new(value->u.boolean);

    case JSON_NULL:
        return json_null_new();
    }

    json_set_error("unknown json value type %d", value->type);
    return NULL;
}

enum json_type
json_value_type(const struct json_value *value) {
    return value->type;
}

struct json_value *
json_object_new(void) {
    return json_value_new(JSON_OBJECT);
}

size_t
json_object_nb_entries(struct json_value *value) {
    return value->u.object.nb_entries;
}

bool
json_object_has_entry(struct json_value *value, const char *key) {
    return json_object_entry2(value, key, strlen(key)) != NULL;
}

bool
json_object_has_entry2(struct json_value *value, const char *key, size_t len) {
    return json_object_entry2(value, key, len) != NULL;
}

struct json_value *
json_object_entry(struct json_value *value, const char *key) {
    return json_object_entry2(value, key, strlen(key));
}

struct json_value *
json_object_entry2(struct json_value *value,
                   const char *key, size_t len) {
    struct json_object *object;

    object = &value->u.object;

    for (size_t i = 0; i < object->nb_entries; i++) {
        struct json_object_entry *entry;

        entry = object->entries + i;

        if (entry->key->u.string.len != len)
            continue;

        if (memcmp(entry->key->u.string.ptr, key, len) == 0)
            return entry->value;
    }

    return NULL;
}

int
json_object_add_entry(struct json_value *object_value, struct json_value *key,
                      struct json_value *value) {
    struct json_object *object;
    struct json_object_entry *entries;
    struct json_object_entry *entry;
    size_t nb_entries;

    if (key->type != JSON_STRING) {
        json_set_error("key is not a string");
        return -1;
    }

    object = &object_value->u.object;

    if (object->nb_entries == 0) {
        nb_entries = 1;
        entries = json_malloc(sizeof(struct json_object_entry));
    } else {
        nb_entries = object->nb_entries + 1;
        entries = json_realloc(object->entries,
                               nb_entries * sizeof(struct json_object_entry));
    }

    if (!entries)
        return -1;

    object->entries = entries;
    object->nb_entries = nb_entries;

    entry = &object->entries[object->nb_entries - 1];
    entry->key = key;
    entry->value = value;

    return 0;
}

struct json_object_iterator *
json_object_iterate(struct json_value *value) {
    struct json_object_iterator *it;

    it = json_malloc(sizeof(struct json_object_iterator));
    if (!it)
        return NULL;
    memset(it, 0, sizeof(struct json_object_iterator));

    it->object = &value->u.object;
    it->index = 0;

    return it;
}

void
json_object_iterator_delete(struct json_object_iterator *it) {
    if (!it)
        return;

    memset(it, 0, sizeof(struct json_object_iterator));
    json_free(it);
}

int
json_object_iterator_get_next(struct json_object_iterator *it,
                              struct json_value **pkey,
                              struct json_value **pvalue) {
    struct json_object_entry *entry;

    if (it->index >= it->object->nb_entries)
        return 0;

    entry = it->object->entries + it->index;

    if (pkey)
        *pkey = entry->key;
    if (pvalue)
        *pvalue = entry->value;

    it->index++;
    return 1;
}

struct json_value *
json_array_new(void) {
    return json_value_new(JSON_ARRAY);
}

size_t
json_array_nb_elements(struct json_value *value) {
    return value->u.array.nb_elements;
}

struct json_value *
json_array_element(struct json_value *value, size_t idx) {
    if (idx >= value->u.array.nb_elements) {
        json_set_error("invalid index %zu", idx);
        return NULL;
    }

    return value->u.array.elements[idx];
}

int
json_array_add_element(struct json_value *value, struct json_value *element) {
    struct json_array *array;
    struct json_value **elements;
    size_t nb_elements;

    if (value->type != JSON_ARRAY) {
        json_set_error("value is not an array");
        return -1;
    }

    array = &value->u.array;

    if (array->nb_elements == 0) {
        nb_elements = 1;
        elements = json_malloc(sizeof(struct json_value *));
    } else {
        nb_elements = array->nb_elements + 1;
        elements = json_realloc(array->elements,
                                nb_elements * sizeof(struct json_value *));
    }

    if (!elements)
        return -1;

    array->elements = elements;
    array->nb_elements = nb_elements;

    array->elements[array->nb_elements - 1] = element;
    return 0;
}

struct json_value *
json_integer_new(int64_t integer) {
    struct json_value *value;

    value = json_value_new(JSON_INTEGER);
    if (!value)
        return NULL;

    value->u.integer = integer;
    return value;
}

int64_t
json_integer_value(struct json_value *value) {
    return value->u.integer;
}

struct json_value *
json_real_new(double real) {
    struct json_value *value;

    value = json_value_new(JSON_REAL);
    if (!value)
        return NULL;

    value->u.real = real;
    return value;
}

double
json_real_value(struct json_value *value) {
    return value->u.real;
}

struct json_value *
json_string_new(const char *string) {
    struct json_value *value;
    size_t length;

    value = json_value_new(JSON_STRING);
    if (!value)
        return NULL;

    length = strlen(string);
    value->u.string.len = length;

    value->u.string.ptr = json_malloc(length + 1);
    if (!value->u.string.ptr) {
        json_value_delete(value);
        return NULL;
    }

    memcpy(value->u.string.ptr, string, length + 1);
    return value;
}

const char *
json_string_value(struct json_value *value) {
    return value->u.string.ptr;
}

size_t
json_string_length(struct json_value *value) {
    return value->u.string.len;
}

struct json_value *
json_string_new2(const char *string, size_t length) {
    struct json_value *value;

    value = json_value_new(JSON_STRING);
    if (!value)
        return NULL;

    value->u.string.len = length;

    value->u.string.ptr = json_malloc(length + 1);
    if (!value->u.string.ptr) {
        json_value_delete(value);
        return NULL;
    }

    memcpy(value->u.string.ptr, string, length + 1);
    return value;
}

struct json_value *
json_boolean_new(bool boolean) {
    struct json_value *value;

    value = json_value_new(JSON_BOOLEAN);
    if (!value)
        return NULL;

    value->u.boolean = boolean;
    return value;
}

bool
json_boolean_value(struct json_value *value) {
    return value->u.boolean;
}

struct json_value *
json_null_new() {
    return json_value_new(JSON_NULL);
}
