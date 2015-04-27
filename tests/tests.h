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

#ifndef JSON_TESTS_H
#define JSON_TESTS_H

#include <utest.h>

#define JSONT_BEGIN2(str_, len_, opts_)                            \
    do {                                                           \
        value = json_parse(str_, len_, opts_);                     \
        if (!value) {                                              \
            TEST_ABORT("cannot parse json: %s", c_get_error());    \
        }                                                          \
    } while (0)

#define JSONT_BEGIN(str_, opts_) JSONT_BEGIN2(str_, strlen(str_), opts_)

#define JSONT_BEGIN_ARRAY2(str_, len_, nb_elements_, opts_)        \
    do {                                                           \
        value = json_parse(str_, len_, opts_);                     \
        if (!value) {                                              \
            TEST_ABORT("cannot parse json: %s", c_get_error());    \
        }                                                          \
                                                                   \
        TEST_INT_EQ(json_value_type(value), JSON_ARRAY);           \
        TEST_UINT_EQ(json_array_nb_elements(value), nb_elements_); \
    } while (0)

#define JSONT_BEGIN_ARRAY(str_, nb_elements_, opts_)               \
    JSONT_BEGIN_ARRAY2(str_, strlen(str_), nb_elements_, opts_)

#define JSONT_BEGIN_OBJECT2(str_, len_, nb_members_, opts_)        \
    do {                                                           \
        value = json_parse(str_, len_, opts_);                     \
        if (!value) {                                              \
            TEST_ABORT("cannot parse json: %s", c_get_error());    \
        }                                                          \
                                                                   \
        TEST_INT_EQ(json_value_type(value), JSON_OBJECT);          \
        TEST_UINT_EQ(json_object_nb_members(value), nb_members_);  \
    } while (0)

#define JSONT_BEGIN_OBJECT(str_, nb_members_, opts_) \
    JSONT_BEGIN_OBJECT2(str_, strlen(str_), nb_members_, opts_)

#define JSONT_END(str_) json_value_delete(value)

#define JSONT_INTEGER_EQ(value_, i_)                        \
    do {                                                    \
        TEST_INT_EQ(json_value_type(value_), JSON_INTEGER); \
        TEST_INT_EQ(json_integer_value(value_), i_);        \
    } while (0)

#define JSONT_REAL_EQ(value_, r_)                           \
    do {                                                    \
        TEST_INT_EQ(json_value_type(value_), JSON_REAL);    \
        TEST_DOUBLE_EQ(json_real_value(value_), r_);        \
    } while (0)

#define JSONT_STRING_EQ(value_, s_)                         \
    do {                                                    \
        TEST_INT_EQ(json_value_type(value_), JSON_STRING);  \
        TEST_STRING_EQ(json_string_value(value_), s_);      \
    } while (0)

#define JSONT_STRING2_EQ(value_, s_, s_len_)                \
    do {                                                    \
        TEST_INT_EQ(json_value_type(value_), JSON_STRING);  \
        TEST_MEM_EQ(json_string_value(value_),              \
                    json_string_length(value_),             \
                    s_, s_len_);                            \
    } while (0)

#define JSONT_BOOLEAN_EQ(value_, b_)                        \
    do {                                                    \
        TEST_INT_EQ(json_value_type(value_), JSON_BOOLEAN); \
        TEST_BOOL_EQ(json_boolean_value(value_), b_);       \
    } while (0)

#define JSONT_NULL_EQ(value_) \
    TEST_INT_EQ(json_value_type(value_), JSON_NULL)

#define JSONT_IS_INVALID(str_, opts_)                  \
    do {                                               \
        struct json_value *value;                      \
        value = json_parse(str_, strlen(str_), opts_); \
        if (value)                                     \
            TEST_ABORT("parsed invalid json");         \
    } while (0)

#endif
