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

    value = c_malloc(sizeof(struct json_value));
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
        for (size_t i = 0; i < value->u.object.nb_members; i++) {
            json_value_delete(value->u.object.members[i].key);
            json_value_delete(value->u.object.members[i].value);
        }
        c_free(value->u.object.members);
        break;

    case JSON_ARRAY:
        for (size_t i = 0; i < value->u.array.nb_elements; i++)
            json_value_delete(value->u.array.elements[i]);
        c_free(value->u.array.elements);
        break;

    case JSON_STRING:
        c_free(value->u.string.ptr);
        break;

    default:
        break;
    }

    memset(value, 0, sizeof(struct json_value));
    c_free(value);
}

struct json_value *
json_value_clone(const struct json_value *value) {
    switch (value->type) {
    case JSON_OBJECT:
    {
        struct json_value *nvalue;

        nvalue = json_object_new();

        for (size_t i = 0; i < value->u.object.nb_members; i++) {
            struct json_object_member *member;
            struct json_value *val;
            const char *key;
            size_t len;

            member = value->u.object.members + i;

            key = member->key->u.string.ptr;
            len = member->key->u.string.len;

            val = json_value_clone(member->value);
            if (!val) {
                json_value_delete(nvalue);
                return NULL;
            }

            if (json_object_add_member2(nvalue, key, len, val) == -1) {
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

    c_set_error("unknown json value type %d", value->type);
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
json_object_nb_members(const struct json_value *value) {
    return value->u.object.nb_members;
}

bool
json_object_has_member(const struct json_value *value, const char *key) {
    return json_object_member2(value, key, strlen(key)) != NULL;
}

bool
json_object_has_member2(const struct json_value *value,
                        const char *key, size_t len) {
    return json_object_member2(value, key, len) != NULL;
}

struct json_value *
json_object_member(const struct json_value *value, const char *key) {
    return json_object_member2(value, key, strlen(key));
}

struct json_value *
json_object_member2(const struct json_value *value,
                    const char *key, size_t len) {
    const struct json_object *object;

    object = &value->u.object;

    for (size_t i = 0; i < object->nb_members; i++) {
        struct json_object_member *member;

        member = object->members + i;

        if (member->key->u.string.len != len)
            continue;

        if (memcmp(member->key->u.string.ptr, key, len) == 0)
            return member->value;
    }

    return NULL;
}

int
json_object_add_member2(struct json_value *object_value, const char *key,
                        size_t len, struct json_value *value) {
    struct json_object *object;
    struct json_value *key_json;
    struct json_object_member *members;
    struct json_object_member *member;
    size_t nb_members;

    key_json = json_string_new2(key, len);
    if (!key_json)
        return -1;

    object = &object_value->u.object;

    if (object->nb_members == 0) {
        nb_members = 1;
        members = c_malloc(sizeof(struct json_object_member));
    } else {
        nb_members = object->nb_members + 1;
        members = c_realloc(object->members,
                               nb_members * sizeof(struct json_object_member));
    }

    if (!members) {
        json_value_delete(key_json);
        return -1;
    }

    object->members = members;
    object->nb_members = nb_members;

    member = &object->members[object->nb_members - 1];
    member->key = key_json;
    member->value = value;

    return 0;
}

int
json_object_add_member(struct json_value *object, const char *key,
                       struct json_value *value) {
    return json_object_add_member2(object, key, strlen(key), value);
}

void
json_object_remove_member2(struct json_value *object_value,
                           const char *key, size_t sz) {
    struct json_object *object;

    object = &object_value->u.object;

    for (size_t i = 0; i < object->nb_members; i++) {
        struct json_object_member *member;

        member = object->members + i;

        if (member->key->u.string.len != sz)
            continue;

        if (memcmp(member->key->u.string.ptr, key, sz) == 0) {
            json_value_delete(member->key);
            json_value_delete(member->value);

            if (i == object->nb_members - 1) {
                member->key = NULL;
                member->value = NULL;
            } else {
                size_t member_sz;

                member_sz = sizeof(struct json_object_member);
                memmove(object->members + i, object->members + i + 1,
                        (object->nb_members - i - 1) * member_sz);
            }

            object->nb_members--;
        }
    }
}

void
json_object_remove_member(struct json_value *object, const char *key) {
    json_object_remove_member2(object, key, strlen(key));
}

int
json_object_set_member2(struct json_value *value, const char *key, size_t len,
                        struct json_value *val) {
    struct json_object *object;
    struct json_object_member *member;
    bool found;

    object = &value->u.object;

    member = NULL;
    found = false;

    for (size_t i = 0; i < object->nb_members; i++) {
        member = object->members + i;

        if (member->key->u.string.len != len)
            continue;

        if (memcmp(member->key->u.string.ptr, key, len) == 0) {
            found = true;
            break;
        }
    }

    if (!found)
        return json_object_add_member2(value, key, len, val);

    json_value_delete(member->value);
    member->value = val;
    return 1;
}

int
json_object_set_member(struct json_value *value, const char *key,
                       struct json_value *val) {
    return json_object_set_member2(value, key, strlen(key), val);
}

struct json_object_iterator *
json_object_iterate(struct json_value *value) {
    struct json_object_iterator *it;

    it = c_malloc(sizeof(struct json_object_iterator));
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
    c_free(it);
}

int
json_object_iterator_get_next(struct json_object_iterator *it,
                              struct json_value **pkey,
                              struct json_value **pvalue) {
    struct json_object_member *member;

    if (it->index >= it->object->nb_members)
        return 0;

    member = it->object->members + it->index;

    if (pkey)
        *pkey = member->key;
    if (pvalue)
        *pvalue = member->value;

    it->index++;
    return 1;
}

struct json_value *
json_array_new(void) {
    return json_value_new(JSON_ARRAY);
}

size_t
json_array_nb_elements(const struct json_value *value) {
    return value->u.array.nb_elements;
}

struct json_value *
json_array_element(const struct json_value *value, size_t idx) {
    if (idx >= value->u.array.nb_elements) {
        c_set_error("invalid index %zu", idx);
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
        c_set_error("value is not an array");
        return -1;
    }

    array = &value->u.array;

    if (array->nb_elements == 0) {
        nb_elements = 1;
        elements = c_malloc(sizeof(struct json_value *));
    } else {
        nb_elements = array->nb_elements + 1;
        elements = c_realloc(array->elements,
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
json_integer_value(const struct json_value *value) {
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
json_real_value(const struct json_value *value) {
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

    value->u.string.ptr = c_malloc(length + 1);
    if (!value->u.string.ptr) {
        json_value_delete(value);
        return NULL;
    }

    memcpy(value->u.string.ptr, string, length + 1);
    return value;
}

const char *
json_string_value(const struct json_value *value) {
    return value->u.string.ptr;
}

size_t
json_string_length(const struct json_value *value) {
    return value->u.string.len;
}

struct json_value *
json_string_new2(const char *string, size_t length) {
    struct json_value *value;

    value = json_value_new(JSON_STRING);
    if (!value)
        return NULL;

    value->u.string.len = length;

    value->u.string.ptr = c_malloc(length + 1);
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
json_boolean_value(const struct json_value *value) {
    return value->u.boolean;
}

struct json_value *
json_null_new() {
    return json_value_new(JSON_NULL);
}
