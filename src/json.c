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

static int json_value_cmp(const void *, const void *);

static int json_object_member_cmp_by_index(const void *, const void *);
static int json_object_member_cmp_by_key(const void *, const void *);
static int json_object_member_cmp_by_key_value(const void *, const void *);

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

bool
json_value_equal(struct json_value *val1, struct json_value *val2) {
    struct json_object_member *members1, *members2;
    size_t nb_members;

    if (val1->type != val2->type)
        return false;

    switch (val1->type) {
    case JSON_OBJECT:
        if (val1->u.object.nb_members != val2->u.object.nb_members)
            return false;

        json_object_sort_by_key_value(&val1->u.object);
        json_object_sort_by_key_value(&val2->u.object);

        nb_members = val1->u.object.nb_members;
        members1 = val1->u.object.members;
        members2 = val2->u.object.members;

        for (size_t i = 0; i < nb_members; i++) {
            if (!json_value_equal(members1[i].key, members2[i].key)
             || !json_value_equal(members1[i].value, members2[i].value)) {
                return false;
            }
        }

        return true;

    case JSON_ARRAY:
        if (val1->u.array.nb_elements != val2->u.array.nb_elements)
            return false;

        for (size_t i = 0; i < val1->u.array.nb_elements; i++) {
            if (!json_value_equal(json_array_element(val1, i),
                                  json_array_element(val2, i))) {
                return false;
            }
        }

        return true;

    case JSON_INTEGER:
        return val1->u.integer == val2->u.integer;

    case JSON_REAL:
        return val1->u.real == val2->u.real;

    case JSON_STRING:
        if (val1->u.string.len != val2->u.string.len)
            return false;

        return memcmp(val1->u.string.ptr, val2->u.string.ptr,
                      val1->u.string.len) == 0;

    case JSON_BOOLEAN:
        return val1->u.boolean == val2->u.boolean;

    case JSON_NULL:
        return true;
    }
}

void
json_value_sort_objects_by_index(struct json_value *value) {
    if (value->type == JSON_OBJECT) {
        json_object_sort_by_index(&value->u.object);

        for (size_t i = 0; i < value->u.object.nb_members; i++)
            json_value_sort_objects_by_index(value->u.object.members[i].value);
    }
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

const char *
json_object_nth_member(const struct json_value *value, size_t idx,
                       struct json_value **pvalue) {
    const struct json_object *object;
    const struct json_object_member *member;

    object = &value->u.object;
    member = object->members + idx;

    if (pvalue)
        *pvalue = member->value;
    return member->key->u.string.ptr;
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
    member->index = nb_members - 1;

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
        size_t removed_index;

        member = object->members + i;

        if (member->key->u.string.len != sz)
            continue;

        if (memcmp(member->key->u.string.ptr, key, sz) == 0) {
            removed_index = member->index;

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
            object->sort_mode = JSON_OBJECT_UNSORTED;

            /* Renumber to avoid holes in the index sequence */
            if (i == object->nb_members) {
                for (size_t j = 0; j < object->nb_members; j++) {
                    struct json_object_member *member2;

                    member2 = object->members + j;

                    if (member2->index > removed_index)
                        member2->index--;
                }
            }
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

void
json_object_sort_by_index(struct json_object *object) {
    if (object->sort_mode == JSON_OBJECT_SORTED_BY_INDEX)
        return;

    qsort(object->members, object->nb_members,
          sizeof(struct json_object_member),
          json_object_member_cmp_by_index);

    object->sort_mode = JSON_OBJECT_SORTED_BY_INDEX;
}

void
json_object_sort_by_key(struct json_object *object) {
    if (object->sort_mode == JSON_OBJECT_SORTED_BY_KEY)
        return;

    qsort(object->members, object->nb_members,
          sizeof(struct json_object_member),
          json_object_member_cmp_by_key);

    object->sort_mode = JSON_OBJECT_SORTED_BY_KEY;
}

void
json_object_sort_by_key_value(struct json_object *object) {
    if (object->sort_mode == JSON_OBJECT_SORTED_BY_KEY_VALUE)
        return;

    qsort(object->members, object->nb_members,
          sizeof(struct json_object_member),
          json_object_member_cmp_by_key_value);

    object->sort_mode = JSON_OBJECT_SORTED_BY_KEY_VALUE;
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

static int
json_value_cmp(const void *arg1, const void *arg2) {
    const struct json_value *val1, *val2;

    val1 = arg1;
    val2 = arg2;

    if (val1->type < val2->type) {
        return -1;
    } else if (val1->type > val2->type) {
        return 1;
    }

    switch (val1->type) {
    case JSON_OBJECT:
        if (val1->u.object.nb_members != val2->u.object.nb_members) {
            return -1;
        } else if (val1->u.object.nb_members != val2->u.object.nb_members) {
            return 1;
        }

        for (size_t i = 0; i < val1->u.object.nb_members; i++) {
            const char *key1, *key2;
            struct json_value *mval1, *mval2;
            int ret;

            key1 = json_object_nth_member(val1, i, &mval1);
            key2 = json_object_nth_member(val2, i, &mval2);

            ret = strcmp(key1, key2);
            if (ret != 0)
                return ret;

            return json_value_cmp(mval1, mval2);
        }

        return 0;

    case JSON_ARRAY:
        if (val1->u.array.nb_elements > val2->u.array.nb_elements) {
            return -1;
        } else if (val1->u.array.nb_elements > val2->u.array.nb_elements) {
            return 0;
        }

        for (size_t i = 0; i < val1->u.array.nb_elements; i++) {
            int ret;

            ret = json_value_equal(json_array_element(val1, i),
                                   json_array_element(val2, i));
            if (ret != 0)
                return ret;
        }

        return 0;

    case JSON_INTEGER:
        if (val1->u.integer < val2->u.integer) {
            return -1;
        } else if (val1->u.integer > val2->u.integer) {
            return 1;
        }

        return 0;

    case JSON_REAL:
        if (val1->u.real < val2->u.real) {
            return -1;
        } else if (val1->u.real > val2->u.real) {
            return 1;
        }

        return 0;

    case JSON_STRING:
        return strcmp(val1->u.string.ptr, val2->u.string.ptr);

    case JSON_BOOLEAN:
        if (val1->u.boolean) {
            return val2->u.boolean ? 0 : 1;
        } else {
            return val2->u.boolean ? -1 : 0;
        }

    case JSON_NULL:
        return 0;
    }
}

static int
json_object_member_cmp_by_index(const void *arg1, const void *arg2) {
    const struct json_object_member *member1, *member2;

    member1 = arg1;
    member2 = arg2;

    if (member1->index < member2->index) {
        return -1;
    } else if (member1->index > member2->index) {
        return 1;
    }

    return 0;
}

static int
json_object_member_cmp_by_key(const void *arg1, const void *arg2) {
    const struct json_object_member *member1, *member2;

    member1 = arg1;
    member2 = arg2;

    return strcmp(member1->key->u.string.ptr, member2->key->u.string.ptr);
}

static int
json_object_member_cmp_by_key_value(const void *arg1, const void *arg2) {
    const struct json_object_member *member1, *member2;
    int ret;

    member1 = arg1;
    member2 = arg2;

    ret = strcmp(member1->key->u.string.ptr, member2->key->u.string.ptr);
    if (ret != 0)
        return ret;

    return json_value_cmp(member1->value, member2->value);
}
