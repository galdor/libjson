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

#include "json.h"

#include "tests.h"

int
main(int argc, char **argv) {
    struct json_value *value, *child;

#define JSONT_BEGIN2(str_, len_)                                \
    do {                                                        \
        value = json_parse(str_, len_);                         \
        if (!value) {                                           \
            JSONT_DIE("%s:%d: cannot parse json: %s",           \
                      __FILE__, __LINE__, json_get_error());    \
        }                                                       \
    } while (0)

#define JSONT_BEGIN(str_) JSONT_BEGIN2(str_, strlen(str_))


#define JSONT_BEGIN_ARRAY2(str_, len_, nb_elements_)             \
    do {                                                         \
        value = json_parse(str_, len_);                          \
        if (!value) {                                            \
            JSONT_DIE("%s:%d: cannot parse json: %s",            \
                      __FILE__, __LINE__, json_get_error());     \
        }                                                        \
                                                                 \
        JSONT_IS_EQUAL_INT(json_value_type(value), JSON_ARRAY);  \
        JSONT_IS_EQUAL_UINT(json_array_nb_elements(value),       \
                            nb_elements_);                       \
    } while (0)

#define JSONT_BEGIN_ARRAY(str_, nb_elements_) \
    JSONT_BEGIN_ARRAY2(str_, strlen(str_), nb_elements_)

#define JSONT_END(str_)            \
    do {                           \
        json_value_delete(value);  \
    } while (0)

#define JSONT_VALUE_IS_INTEGER(value_, i_)                         \
    do {                                                           \
        JSONT_IS_EQUAL_INT(json_value_type(value_), JSON_INTEGER); \
        JSONT_IS_EQUAL_INT(json_integer_value(value_), i_);        \
    } while (0)

#define JSONT_VALUE_IS_REAL(value_, r_)                            \
    do {                                                           \
        JSONT_IS_EQUAL_INT(json_value_type(value_), JSON_REAL);    \
        JSONT_IS_EQUAL_DOUBLE(json_real_value(value_), r_);        \
    } while (0)

#define JSONT_VALUE_IS_STRING(value_, s_)                          \
    do {                                                           \
        JSONT_IS_EQUAL_INT(json_value_type(value_), JSON_STRING);  \
        JSONT_IS_EQUAL_STRING(json_string_value(value_), s_);      \
    } while (0)

#define JSONT_VALUE_IS_STRING2(value_, s_, s_len_)                 \
    do {                                                           \
        JSONT_IS_EQUAL_INT(json_value_type(value_), JSON_STRING);  \
        JSONT_IS_EQUAL_STRING2(json_string_value(value_),          \
                               json_string_length(value_),         \
                               s_, s_len_);                        \
    } while (0)

#define JSONT_VALUE_IS_BOOLEAN(value_, b_)                         \
    do {                                                           \
        JSONT_IS_TRUE(value != NULL);                              \
        JSONT_IS_EQUAL_INT(json_value_type(value_), JSON_BOOLEAN); \
        JSONT_IS_EQUAL_BOOL(json_boolean_value(value_), b_);       \
    } while (0)

    /* Arrays */
    JSONT_BEGIN("[]");
    JSONT_IS_EQUAL_INT(json_value_type(value), JSON_ARRAY);
    JSONT_IS_EQUAL_UINT(json_object_nb_entries(value), 0);
    JSONT_END();

    JSONT_BEGIN(" [\t\n ] ");
    JSONT_IS_EQUAL_INT(json_value_type(value), JSON_ARRAY);
    JSONT_IS_EQUAL_UINT(json_object_nb_entries(value), 0);
    JSONT_END();

    /* Integers */
    JSONT_BEGIN_ARRAY("[0, 1, -1, 42, -127]", 5);
    JSONT_VALUE_IS_INTEGER(json_array_element(value, 0), 0);
    JSONT_VALUE_IS_INTEGER(json_array_element(value, 1), 1);
    JSONT_VALUE_IS_INTEGER(json_array_element(value, 2), -1);
    JSONT_VALUE_IS_INTEGER(json_array_element(value, 3), 42);
    JSONT_VALUE_IS_INTEGER(json_array_element(value, 4), -127);
    JSONT_END();

    JSONT_BEGIN_ARRAY("[-9223372036854775808, 9223372036854775807]", 2);
    JSONT_VALUE_IS_INTEGER(json_array_element(value, 0), INT64_MIN);
    JSONT_VALUE_IS_INTEGER(json_array_element(value, 1), INT64_MAX);
    JSONT_END();

    /* Reals */
    JSONT_BEGIN_ARRAY("[0.0, -0.1, 1337.0042, 5e3, -4.38e117]", 5);
    JSONT_VALUE_IS_REAL(json_array_element(value, 0), 0.0);
    JSONT_VALUE_IS_REAL(json_array_element(value, 1), -0.1);
    JSONT_VALUE_IS_REAL(json_array_element(value, 2), 1337.0042);
    JSONT_VALUE_IS_REAL(json_array_element(value, 3), 5e3);
    JSONT_VALUE_IS_REAL(json_array_element(value, 4), -4.38e117);
    JSONT_END();

    /* Strings */
    JSONT_BEGIN_ARRAY("[\"\", \"a\", \"foo bar\"]", 3);
    JSONT_VALUE_IS_STRING(json_array_element(value, 0), "");
    JSONT_VALUE_IS_STRING(json_array_element(value, 1), "a");
    JSONT_VALUE_IS_STRING(json_array_element(value, 2), "foo bar");
    JSONT_END();

    JSONT_BEGIN_ARRAY("[\"\\\"\", \"\\\\\", \"\\/\"]", 3);
    JSONT_VALUE_IS_STRING(json_array_element(value, 0), "\"");
    JSONT_VALUE_IS_STRING(json_array_element(value, 1), "\\");
    JSONT_VALUE_IS_STRING(json_array_element(value, 2), "/");
    JSONT_END();

    JSONT_BEGIN_ARRAY("[\"\\b\\f\\n\\r\\t\"]", 1);
    JSONT_VALUE_IS_STRING(json_array_element(value, 0), "\b\f\n\r\t");
    JSONT_END();

    JSONT_BEGIN_ARRAY("[\"\\u0061\", \"\\u00e0\\U00E9\"]", 2);
    JSONT_VALUE_IS_STRING(json_array_element(value, 0), "a");
    JSONT_VALUE_IS_STRING(json_array_element(value, 1), "àé");
    JSONT_END();

    JSONT_BEGIN_ARRAY("[\"\\u250c\\u2500\\u2510\"]", 1);
    JSONT_VALUE_IS_STRING(json_array_element(value, 0), "┌─┐");
    JSONT_END();

    JSONT_BEGIN_ARRAY("[\"\\ud834\\udd1e\", \"\\UD834\\UDD1E\"]", 2);
    JSONT_VALUE_IS_STRING(json_array_element(value, 0), "𝄞");
    JSONT_VALUE_IS_STRING(json_array_element(value, 1), "𝄞");
    JSONT_END();

    JSONT_BEGIN_ARRAY2("[\"foo\0bar\", \"\0tail\", \"head\0\"]", 29, 3);
    JSONT_VALUE_IS_STRING2(json_array_element(value, 0), "foo\0bar", 7);
    JSONT_VALUE_IS_STRING2(json_array_element(value, 1), "\0tail", 5);
    JSONT_VALUE_IS_STRING2(json_array_element(value, 2), "head\0", 5);
    JSONT_END();

    JSONT_BEGIN_ARRAY2("[\"foo\\u0000bar\", \"\\u0000tail\", \"head\\u0000\"]",
                       44, 3);
    JSONT_VALUE_IS_STRING2(json_array_element(value, 0), "foo\0bar", 7);
    JSONT_VALUE_IS_STRING2(json_array_element(value, 1), "\0tail", 5);
    JSONT_VALUE_IS_STRING2(json_array_element(value, 2), "head\0", 5);
    JSONT_END();

    /* Booleans */
    JSONT_BEGIN_ARRAY("[true, false]", 2);
    JSONT_VALUE_IS_BOOLEAN(json_array_element(value, 0), true);
    JSONT_VALUE_IS_BOOLEAN(json_array_element(value, 1), false);
    JSONT_END();

    /* Null */
    JSONT_BEGIN_ARRAY("[null]", 1);
    JSONT_IS_EQUAL_INT(json_value_type(json_array_element(value, 0)), JSON_NULL);
    JSONT_END();

    /* Objects */
    JSONT_BEGIN("{}");
    JSONT_IS_EQUAL_INT(json_value_type(value), JSON_OBJECT);
    JSONT_IS_EQUAL_UINT(json_object_nb_entries(value), 0);
    JSONT_END();

    JSONT_BEGIN(" {\t\n  } ");
    JSONT_IS_EQUAL_INT(json_value_type(value), JSON_OBJECT);
    JSONT_IS_EQUAL_UINT(json_object_nb_entries(value), 0);
    JSONT_END();

    JSONT_BEGIN("{\"a\": 1, \"b\"  :2  ,  \"c\":3}");
    JSONT_IS_EQUAL_INT(json_value_type(value), JSON_OBJECT);
    JSONT_IS_EQUAL_UINT(json_object_nb_entries(value), 3);
    JSONT_IS_TRUE(json_object_has_entry(value, "a"));
    JSONT_VALUE_IS_INTEGER(json_object_entry(value, "a"), 1);
    JSONT_IS_TRUE(json_object_has_entry(value, "b"));
    JSONT_VALUE_IS_INTEGER(json_object_entry(value, "b"), 2);
    JSONT_IS_TRUE(json_object_has_entry(value, "c"));
    JSONT_VALUE_IS_INTEGER(json_object_entry(value, "c"), 3);
    JSONT_END();

    JSONT_BEGIN("{\"\": 1, \"   \": 2}");
    JSONT_IS_EQUAL_INT(json_value_type(value), JSON_OBJECT);
    JSONT_IS_EQUAL_UINT(json_object_nb_entries(value), 2);
    JSONT_IS_TRUE(json_object_has_entry(value, ""));
    JSONT_VALUE_IS_INTEGER(json_object_entry(value, ""), 1);
    JSONT_IS_TRUE(json_object_has_entry(value, "   "));
    JSONT_VALUE_IS_INTEGER(json_object_entry(value, "   "), 2);
    JSONT_END();

    JSONT_BEGIN2("{\"foo\0bar\": 1, \"\0tail\": 2, \"head\0\": 3}", 38);
    JSONT_IS_EQUAL_INT(json_value_type(value), JSON_OBJECT);
    JSONT_IS_EQUAL_UINT(json_object_nb_entries(value), 3);
    JSONT_IS_TRUE(json_object_has_entry2(value, "foo\0bar", 7));
    JSONT_VALUE_IS_INTEGER(json_object_entry2(value, "foo\0bar", 7), 1);
    JSONT_IS_TRUE(json_object_has_entry2(value, "\0tail", 5));
    JSONT_VALUE_IS_INTEGER(json_object_entry2(value, "\0tail", 5), 2);
    JSONT_IS_TRUE(json_object_has_entry2(value, "head\0", 5));
    JSONT_VALUE_IS_INTEGER(json_object_entry2(value, "head\0", 5), 3);
    JSONT_END();

    JSONT_BEGIN("{\"a\": {\"aa\": 1}, \"b\": [{\"ba\": 1, \"bb\": 2}]}");
    JSONT_IS_EQUAL_INT(json_value_type(value), JSON_OBJECT);
    JSONT_IS_EQUAL_UINT(json_object_nb_entries(value), 2);
    JSONT_IS_TRUE(json_object_has_entry(value, "a"));
    child = json_object_entry(value, "a");
    JSONT_IS_TRUE(child != NULL);
    JSONT_IS_EQUAL_INT(json_value_type(child), JSON_OBJECT);
    JSONT_IS_EQUAL_UINT(json_object_nb_entries(child), 1);
    JSONT_IS_TRUE(json_object_has_entry(child, "aa"));
    JSONT_VALUE_IS_INTEGER(json_object_entry(child, "aa"), 1);
    JSONT_IS_TRUE(json_object_has_entry(value, "b"));
    child = json_object_entry(value, "b");
    JSONT_IS_TRUE(child != NULL);
    JSONT_IS_EQUAL_INT(json_value_type(child), JSON_ARRAY);
    JSONT_IS_EQUAL_UINT(json_array_nb_elements(child), 1);
    child = json_array_element(child, 0);
    JSONT_IS_EQUAL_INT(json_value_type(child), JSON_OBJECT);
    JSONT_IS_EQUAL_UINT(json_object_nb_entries(child), 2);
    JSONT_IS_TRUE(json_object_has_entry(child, "ba"));
    JSONT_VALUE_IS_INTEGER(json_object_entry(child, "ba"), 1);
    JSONT_IS_TRUE(json_object_has_entry(child, "bb"));
    JSONT_VALUE_IS_INTEGER(json_object_entry(child, "bb"), 2);
    JSONT_END();

    /* Invalid json */

#define JSONT_IS_INVALID(str_)                      \
    do {                                            \
        value = json_parse(str_, strlen(str_));     \
        if (value) {                                \
            JSONT_DIE("%s:%d: parsed invalid json", \
                      __FILE__, __LINE__);          \
        }                                           \
    } while (0)

    /* Top-level */
    JSONT_IS_INVALID("");
    JSONT_IS_INVALID("1");
    JSONT_IS_INVALID("1.0");
    JSONT_IS_INVALID("\"\"");
    JSONT_IS_INVALID("true");
    JSONT_IS_INVALID("null");

    /* Arrays */
    JSONT_IS_INVALID("[");
    JSONT_IS_INVALID("[1");
    JSONT_IS_INVALID("[1,");
    JSONT_IS_INVALID("[1 ,");
    JSONT_IS_INVALID("[1 , ");
    JSONT_IS_INVALID("[1, 2");
    JSONT_IS_INVALID("[1, 2 ");
    JSONT_IS_INVALID("[1, 2,]");
    JSONT_IS_INVALID("[1, 2 ,]");
    JSONT_IS_INVALID("[1, 2 , ]");

    /* Integers */
    JSONT_IS_INVALID("[92233720368547758080]");
    JSONT_IS_INVALID("[9223372036854775808]");
    JSONT_IS_INVALID("[-9223372036854775809]");
    JSONT_IS_INVALID("[1foo]");

    /* Reals */
    JSONT_IS_INVALID("[1.0foo]");
    JSONT_IS_INVALID("[1.0efoo]");

    /* Strings */
    JSONT_IS_INVALID("[\"]");
    JSONT_IS_INVALID("[\"\\");
    JSONT_IS_INVALID("[\"\\\"]");
    JSONT_IS_INVALID("[\"\\i\"]");
    JSONT_IS_INVALID("[\"\\u\"]");
    JSONT_IS_INVALID("[\"\\u000\"]");
    JSONT_IS_INVALID("[\"\\u00gh\"]");
    JSONT_IS_INVALID("[\"\\U\"]");
    JSONT_IS_INVALID("[\"\\U000\"]");
    JSONT_IS_INVALID("[\"\\U00gh\"]");
    JSONT_IS_INVALID("[\"\\ud834\"]");
    JSONT_IS_INVALID("[\"\\ud834\\u\"]");
    JSONT_IS_INVALID("[\"\\ud834\\udd1\"]");

    /* Literals */
    JSONT_IS_INVALID("[yes]");
    JSONT_IS_INVALID("[nullx]");
    JSONT_IS_INVALID("[t]");
    JSONT_IS_INVALID("[f]");
    JSONT_IS_INVALID("[n]");

    /* Objects */
    JSONT_IS_INVALID("{");
    JSONT_IS_INVALID("{\"foo\",");
    JSONT_IS_INVALID("{\"foo\" ,");
    JSONT_IS_INVALID("{\"foo\":");
    JSONT_IS_INVALID("{\"foo\" :");
    JSONT_IS_INVALID("{\"foo\": 1,");
    JSONT_IS_INVALID("{\"foo\": 1 ,");
    JSONT_IS_INVALID("{\"foo\"}");
    JSONT_IS_INVALID("{\"foo\",}");
    JSONT_IS_INVALID("{\"foo\" ,}");
    JSONT_IS_INVALID("{\"a\":}");
    JSONT_IS_INVALID("{\"a\" :}");
    JSONT_IS_INVALID("{\"a\" : }");
    JSONT_IS_INVALID("{\"a\": 1,}");
    JSONT_IS_INVALID("{\"a\": 1 ,}");
    JSONT_IS_INVALID("{\"a\": 1 , }");

#undef JSONT_IS_INVALID

#undef JSONT_BEGIN
#undef JSONT_BEGIN_ARRAY
#undef JSONT_END
#undef JSONT_VALUE_IS_INTEGER
#undef JSONT_VALUE_IS_REAL
#undef JSONT_VALUE_IS_STRING
#undef JSONT_VALUE_IS_BOOLEAN

    return 0;
}
