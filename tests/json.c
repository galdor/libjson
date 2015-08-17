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

#include "../src/json.h"
#include "tests.h"

TEST(arrays) {
    struct json_value *value;

    JSONT_PARSE_ARRAY("[]", 0, JSON_PARSE_DEFAULT);
    json_value_delete(value);

    JSONT_PARSE_ARRAY(" [\t\n ] ", 0, JSON_PARSE_DEFAULT);
    json_value_delete(value);
}

TEST(integers) {
    struct json_value *value;

    JSONT_PARSE("0", JSON_PARSE_DEFAULT);
    JSONT_INTEGER_EQ(value, 0);
    json_value_delete(value);

    JSONT_PARSE("1", JSON_PARSE_DEFAULT);
    JSONT_INTEGER_EQ(value, 1);
    json_value_delete(value);

    JSONT_PARSE("-1", JSON_PARSE_DEFAULT);
    JSONT_INTEGER_EQ(value, -1);
    json_value_delete(value);

    JSONT_PARSE("42", JSON_PARSE_DEFAULT);
    JSONT_INTEGER_EQ(value, 42);
    json_value_delete(value);

    JSONT_PARSE("-127", JSON_PARSE_DEFAULT);
    JSONT_INTEGER_EQ(value, -127);
    json_value_delete(value);

    JSONT_PARSE("-9223372036854775808", JSON_PARSE_DEFAULT);
    JSONT_INTEGER_EQ(value, INT64_MIN);
    json_value_delete(value);

    JSONT_PARSE("9223372036854775807", JSON_PARSE_DEFAULT);
    JSONT_INTEGER_EQ(value, INT64_MAX);
    json_value_delete(value);
}

TEST(reals) {
    struct json_value *value;

    JSONT_PARSE("0.0", JSON_PARSE_DEFAULT);
    JSONT_REAL_EQ(value, 0.0);
    json_value_delete(value);

    JSONT_PARSE("-0.1", JSON_PARSE_DEFAULT);
    JSONT_REAL_EQ(value, -0.1);
    json_value_delete(value);

    JSONT_PARSE("1337.0042", JSON_PARSE_DEFAULT);
    JSONT_REAL_EQ(value, 1337.0042);
    json_value_delete(value);

    JSONT_PARSE("5e3", JSON_PARSE_DEFAULT);
    JSONT_REAL_EQ(value, 5e3);
    json_value_delete(value);

    JSONT_PARSE("-4.38e117", JSON_PARSE_DEFAULT);
    JSONT_REAL_EQ(value, -4.38e117);
    json_value_delete(value);
}

TEST(strings) {
    struct json_value *value;

    JSONT_PARSE("\"\"", JSON_PARSE_DEFAULT);
    JSONT_STRING_EQ(value, "");
    json_value_delete(value);

    JSONT_PARSE("\"a\"", JSON_PARSE_DEFAULT);
    JSONT_STRING_EQ(value, "a");
    json_value_delete(value);

    JSONT_PARSE("\"foo bar\"", JSON_PARSE_DEFAULT);
    JSONT_STRING_EQ(value, "foo bar");
    json_value_delete(value);

    JSONT_PARSE("\"\\\"\"", JSON_PARSE_DEFAULT);
    JSONT_STRING_EQ(value, "\"");
    json_value_delete(value);

    JSONT_PARSE("\"\\\\\"", JSON_PARSE_DEFAULT);
    JSONT_STRING_EQ(value, "\\");
    json_value_delete(value);

    JSONT_PARSE("\"\\/\"", JSON_PARSE_DEFAULT);
    JSONT_STRING_EQ(value, "/");
    json_value_delete(value);

    JSONT_PARSE("\"\\b\\f\\n\\r\\t\"", JSON_PARSE_DEFAULT);
    JSONT_STRING_EQ(value, "\b\f\n\r\t");
    json_value_delete(value);

    JSONT_PARSE("\"\\u0061\"", JSON_PARSE_DEFAULT);
    JSONT_STRING_EQ(value, "a");
    json_value_delete(value);

    JSONT_PARSE("\"\\u00e0\\U00E9\"", JSON_PARSE_DEFAULT);
    JSONT_STRING_EQ(value, "àé");
    json_value_delete(value);

    JSONT_PARSE("\"\\u250c\\u2500\\u2510\"", JSON_PARSE_DEFAULT);
    JSONT_STRING_EQ(value, "┌─┐");
    json_value_delete(value);

    JSONT_PARSE("\"\\ud834\\udd1e\"", JSON_PARSE_DEFAULT);
    JSONT_STRING_EQ(value, "𝄞");
    json_value_delete(value);

    JSONT_PARSE("\"\\UD834\\UDD1E\"", JSON_PARSE_DEFAULT);
    JSONT_STRING_EQ(value, "𝄞");
    json_value_delete(value);

    JSONT_PARSE2("\"foo\0bar\"", 9, JSON_PARSE_DEFAULT);
    JSONT_STRING2_EQ(value, "foo\0bar", 7);
    json_value_delete(value);

    JSONT_PARSE2("\"\0tail\"", 7, JSON_PARSE_DEFAULT);
    JSONT_STRING2_EQ(value, "\0tail", 5);
    json_value_delete(value);

    JSONT_PARSE2("\"head\0\"", 7, JSON_PARSE_DEFAULT);
    JSONT_STRING2_EQ(value, "head\0", 5);
    json_value_delete(value);

    JSONT_PARSE2("\"foo\\u0000bar\"", 14, JSON_PARSE_DEFAULT);
    JSONT_STRING2_EQ(value, "foo\0bar", 7);
    json_value_delete(value);

    JSONT_PARSE2("\"\\u0000tail\"", 12, JSON_PARSE_DEFAULT);
    JSONT_STRING2_EQ(value, "\0tail", 5);
    json_value_delete(value);

    JSONT_PARSE2("\"head\\u0000\"", 12, JSON_PARSE_DEFAULT);
    JSONT_STRING2_EQ(value, "head\0", 5);
    json_value_delete(value);
}

TEST(booleans) {
    struct json_value *value;

    JSONT_PARSE("true", JSON_PARSE_DEFAULT);
    JSONT_BOOLEAN_EQ(value, true);
    json_value_delete(value);

    JSONT_PARSE("false", JSON_PARSE_DEFAULT);
    JSONT_BOOLEAN_EQ(value, false);
    json_value_delete(value);
}

TEST(null) {
    struct json_value *value;

    JSONT_PARSE("null", JSON_PARSE_DEFAULT);
    JSONT_NULL_EQ(value);
    json_value_delete(value);
}

TEST(objects) {
    struct json_value *value, *child;
    const char *key;

    JSONT_PARSE_OBJECT("{}", 0, JSON_PARSE_DEFAULT);
    json_value_delete(value);

    JSONT_PARSE_OBJECT(" {\t\n  } ", 0, JSON_PARSE_DEFAULT);
    json_value_delete(value);

    JSONT_PARSE_OBJECT("{\"a\": 1, \"b\"  :2  ,  \"c\":3}", 3,
                      JSON_PARSE_DEFAULT);
    TEST_TRUE(json_object_has_member(value, "a"));
    JSONT_INTEGER_EQ(json_object_member(value, "a"), 1);
    TEST_TRUE(json_object_has_member(value, "b"));
    JSONT_INTEGER_EQ(json_object_member(value, "b"), 2);
    TEST_TRUE(json_object_has_member(value, "c"));
    JSONT_INTEGER_EQ(json_object_member(value, "c"), 3);
    json_value_delete(value);

    JSONT_PARSE_OBJECT("{\"\": 1, \"   \": 2}", 2,
                      JSON_PARSE_DEFAULT);
    TEST_TRUE(json_object_has_member(value, ""));
    JSONT_INTEGER_EQ(json_object_member(value, ""), 1);
    TEST_TRUE(json_object_has_member(value, "   "));
    JSONT_INTEGER_EQ(json_object_member(value, "   "), 2);
    json_value_delete(value);

    JSONT_PARSE_OBJECT2("{\"foo\0bar\": 1, \"\0tail\": 2, \"head\0\": 3}", 38, 3,
                      JSON_PARSE_DEFAULT);
    TEST_TRUE(json_object_has_member2(value, "foo\0bar", 7));
    JSONT_INTEGER_EQ(json_object_member2(value, "foo\0bar", 7), 1);
    TEST_TRUE(json_object_has_member2(value, "\0tail", 5));
    JSONT_INTEGER_EQ(json_object_member2(value, "\0tail", 5), 2);
    TEST_TRUE(json_object_has_member2(value, "head\0", 5));
    JSONT_INTEGER_EQ(json_object_member2(value, "head\0", 5), 3);
    json_value_delete(value);

    JSONT_PARSE_OBJECT("{\"a\": {\"aa\": 1}, \"b\": [{\"ba\": 1, \"bb\": 2}]}", 2,
                      JSON_PARSE_DEFAULT);
    TEST_TRUE(json_object_has_member(value, "a"));
    child = json_object_member(value, "a");
    TEST_PTR_NOT_NULL(child);
    TEST_INT_EQ(json_value_type(child), JSON_OBJECT);
    TEST_UINT_EQ(json_object_nb_members(child), 1);
    TEST_TRUE(json_object_has_member(child, "aa"));
    JSONT_INTEGER_EQ(json_object_member(child, "aa"), 1);
    TEST_TRUE(json_object_has_member(value, "b"));
    child = json_object_member(value, "b");
    TEST_PTR_NOT_NULL(child);
    TEST_INT_EQ(json_value_type(child), JSON_ARRAY);
    TEST_UINT_EQ(json_array_nb_elements(child), 1);
    child = json_array_element(child, 0);
    TEST_INT_EQ(json_value_type(child), JSON_OBJECT);
    TEST_UINT_EQ(json_object_nb_members(child), 2);
    TEST_TRUE(json_object_has_member(child, "ba"));
    JSONT_INTEGER_EQ(json_object_member(child, "ba"), 1);
    TEST_TRUE(json_object_has_member(child, "bb"));
    JSONT_INTEGER_EQ(json_object_member(child, "bb"), 2);
    json_value_delete(value);

    /* Duplicate keys */
    JSONT_PARSE_OBJECT("{\"a\": 1, \"a\": 2}", 2, JSON_PARSE_DEFAULT);
    TEST_UINT_EQ(json_object_nb_members(value), 2);
    TEST_TRUE(json_object_has_member(value, "a"));
    child = json_object_member(value, "a");
    TEST_PTR_NOT_NULL(child);
    JSONT_INTEGER_EQ(child, 1);
    key = json_object_nth_member(value, 0, &child);
    TEST_PTR_NOT_NULL(key);
    TEST_STRING_EQ(key, "a");
    TEST_PTR_NOT_NULL(child);
    JSONT_INTEGER_EQ(child, 1);
    key = json_object_nth_member(value, 1, &child);
    TEST_PTR_NOT_NULL(key);
    TEST_STRING_EQ(key, "a");
    TEST_PTR_NOT_NULL(child);
    JSONT_INTEGER_EQ(child, 2);
    json_value_delete(value);
}

TEST(object_iterators) {
    struct json_value *value, *key, *val;
    struct json_object_iterator *it;

    JSONT_PARSE("{}", JSON_PARSE_DEFAULT);
    it = json_object_iterate(value);
    TEST_INT_EQ(json_object_iterator_get_next(it, &key, &val), 0);
    TEST_INT_EQ(json_object_iterator_get_next(it, &key, &val), 0);
    json_object_iterator_delete(it);
    json_value_delete(value);

    JSONT_PARSE("{\"a\": 1, \"b\": 2, \"c\": 3}",
                JSON_PARSE_DEFAULT);
    it = json_object_iterate(value);
    TEST_INT_EQ(json_object_iterator_get_next(it, &key, &val), 1);
    JSONT_STRING_EQ(key, "a");
    JSONT_INTEGER_EQ(val, 1);
    TEST_INT_EQ(json_object_iterator_get_next(it, &key, &val), 1);
    JSONT_STRING_EQ(key, "b");
    JSONT_INTEGER_EQ(val, 2);
    TEST_INT_EQ(json_object_iterator_get_next(it, &key, &val), 1);
    JSONT_STRING_EQ(key, "c");
    JSONT_INTEGER_EQ(val, 3);
    TEST_INT_EQ(json_object_iterator_get_next(it, &key, &val), 0);
    TEST_INT_EQ(json_object_iterator_get_next(it, &key, &val), 0);
    json_object_iterator_delete(it);
    json_value_delete(value);
}

TEST(object_remove_member) {
    struct json_value *value;

    JSONT_PARSE("{\"a\": 1}",
                JSON_PARSE_DEFAULT);
    json_object_remove_member(value, "a");
    TEST_UINT_EQ(json_object_nb_members(value), 0);
    TEST_FALSE(json_object_has_member(value, "a"));
    json_value_delete(value);

    JSONT_PARSE("{\"a\": 1, \"b\": 2, \"c\": 3, \"d\": 4, \"e\": 5}",
                JSON_PARSE_DEFAULT);
    json_object_remove_member(value, "a");
    TEST_UINT_EQ(json_object_nb_members(value), 4);
    TEST_FALSE(json_object_has_member(value, "a"));
    json_object_remove_member(value, "c");
    TEST_UINT_EQ(json_object_nb_members(value), 3);
    TEST_FALSE(json_object_has_member(value, "c"));
    json_object_remove_member(value, "e");
    TEST_UINT_EQ(json_object_nb_members(value), 2);
    TEST_FALSE(json_object_has_member(value, "e"));
    json_value_delete(value);

    JSONT_PARSE("{}", JSON_PARSE_DEFAULT);
    json_object_remove_member(value, "a");
    TEST_UINT_EQ(json_object_nb_members(value), 0);
    json_value_delete(value);

    JSONT_PARSE("{\"a\": 1}", JSON_PARSE_DEFAULT);
    json_object_remove_member(value, "b");
    TEST_UINT_EQ(json_object_nb_members(value), 1);
    TEST_TRUE(json_object_has_member(value, "a"));
    json_value_delete(value);

    JSONT_PARSE("{\"a\": 1, \"b\": 2, \"c\": 3, \"b\": 4}",
                JSON_PARSE_DEFAULT);
    json_object_remove_member(value, "b");
    TEST_UINT_EQ(json_object_nb_members(value), 2);
    TEST_FALSE(json_object_has_member(value, "b"));
    json_value_delete(value);
}

TEST(invalid) {
    JSONT_IS_INVALID("", JSON_PARSE_DEFAULT);
}

TEST(invalid_arrays) {
    JSONT_IS_INVALID("[", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("[1", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("[1,", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("[1 ,", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("[1 , ", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("[1, 2", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("[1, 2 ", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("[1, 2,]", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("[1, 2 ,]", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("[1, 2 , ]", JSON_PARSE_DEFAULT);
}

TEST(invalid_integers) {
    JSONT_IS_INVALID("92233720368547758080", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("9223372036854775808", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("-9223372036854775809", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("1foo", JSON_PARSE_DEFAULT);
}

TEST(invalid_reals) {
    JSONT_IS_INVALID("1.0foo", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("1.0efoo", JSON_PARSE_DEFAULT);
}

TEST(invalid_strings) {
    JSONT_IS_INVALID("\"", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("\"\\", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("\"\\\"", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("\"\\i\"", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("\"\\u\"", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("\"\\u000\"", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("\"\\u00gh\"", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("\"\\U\"", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("\"\\U000\"", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("\"\\U00gh\"", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("\"\\ud834\"", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("\"\\ud834\\u\"", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("\"\\ud834\\udd1\"", JSON_PARSE_DEFAULT);

    JSONT_IS_INVALID("\"\\u0000\"", JSON_PARSE_REJECT_NULL_CHARACTERS);
    JSONT_IS_INVALID("\"abc\\u0000def\"", JSON_PARSE_REJECT_NULL_CHARACTERS);
    JSONT_IS_INVALID("\"abc\0def\"", JSON_PARSE_REJECT_NULL_CHARACTERS);
    JSONT_IS_INVALID("\"\0abc\0def\0\"", JSON_PARSE_REJECT_NULL_CHARACTERS);
}

TEST(invalid_literals) {
    JSONT_IS_INVALID("yes", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("t", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("f", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("n", JSON_PARSE_DEFAULT);
}

TEST(invalid_objects) {
    JSONT_IS_INVALID("{", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("{\"foo\",", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("{\"foo\" ,", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("{\"foo\":", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("{\"foo\" :", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("{\"foo\": 1,", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("{\"foo\": 1 ,", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("{\"foo\"}", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("{\"foo\",}", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("{\"foo\" ,}", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("{\"a\":}", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("{\"a\" :}", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("{\"a\" : }", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("{\"a\": 1,}", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("{\"a\": 1 ,}", JSON_PARSE_DEFAULT);
    JSONT_IS_INVALID("{\"a\": 1 , }", JSON_PARSE_DEFAULT);

    JSONT_IS_INVALID("{\"a\": 1, \"a\": 2}", JSON_PARSE_REJECT_DUPLICATE_KEYS);
}

TEST(value_equal) {
    struct json_value *val1, *val2;
    struct json_value *tmp;

#define JSONT_VALUE_EQUAL(val1_, val2_, res_)         \
    do {                                              \
        if (json_value_equal(val1_, val2_) != res_) { \
            if (res_) {                               \
                TEST_ABORT("value are not equal");    \
            } else {                                  \
                TEST_ABORT("value are equal");        \
            }                                         \
        }                                             \
                                                      \
        json_value_delete(val1_);                     \
        json_value_delete(val2_);                     \
    } while (0)

    /* Null */
    val1 = json_null_new();
    val2 = json_null_new();
    JSONT_VALUE_EQUAL(val1, val2, true);

    /* Booleans */
    val1 = json_boolean_new(true);
    val2 = json_boolean_new(true);
    JSONT_VALUE_EQUAL(val1, val2, true);

    val1 = json_boolean_new(false);
    val2 = json_boolean_new(false);
    JSONT_VALUE_EQUAL(val1, val2, true);

    val1 = json_boolean_new(true);
    val2 = json_boolean_new(false);
    JSONT_VALUE_EQUAL(val1, val2, false);

    /* Integers */
    val1 = json_integer_new(0);
    val2 = json_integer_new(0);
    JSONT_VALUE_EQUAL(val1, val2, true);

    val1 = json_integer_new(-1);
    val2 = json_integer_new(1);
    JSONT_VALUE_EQUAL(val1, val2, false);

    /* Reals */
    val1 = json_real_new(1.42);
    val2 = json_real_new(1.42);
    JSONT_VALUE_EQUAL(val1, val2, true);

    val1 = json_real_new(-31.3423e2);
    val2 = json_real_new(-31.3424e2);
    JSONT_VALUE_EQUAL(val1, val2, false);

    /* Strings */
    val1 = json_string_new("");
    val2 = json_string_new("");
    JSONT_VALUE_EQUAL(val1, val2, true);

    val1 = json_string_new("foo bar");
    val2 = json_string_new("foo bar");
    JSONT_VALUE_EQUAL(val1, val2, true);

    val1 = json_string_new("foo");
    val2 = json_string_new("foo bar");
    JSONT_VALUE_EQUAL(val1, val2, false);

    /* Arrays */
    val1 = json_array_new();
    val2 = json_array_new();
    JSONT_VALUE_EQUAL(val1, val2, true);

    val1 = json_array_new();
    json_array_add_element(val1, json_integer_new(42));
    json_array_add_element(val1, json_array_new());
    val2 = json_array_new();
    json_array_add_element(val2, json_integer_new(42));
    json_array_add_element(val2, json_array_new());
    JSONT_VALUE_EQUAL(val1, val2, true);

    val1 = json_array_new();
    json_array_add_element(val1, json_integer_new(42));
    json_array_add_element(val1, json_array_new());
    val2 = json_array_new();
    json_array_add_element(val2, json_integer_new(42));
    json_array_add_element(val2, json_null_new());
    JSONT_VALUE_EQUAL(val1, val2, false);

    /* Objects */
    val1 = json_object_new();
    val2 = json_object_new();
    JSONT_VALUE_EQUAL(val1, val2, true);

    val1 = json_object_new();
    json_object_add_member(val1, "a", json_boolean_new(true));
    json_object_add_member(val1, "b", json_integer_new(0));
    val2 = json_object_new();
    json_object_add_member(val2, "a", json_boolean_new(true));
    json_object_add_member(val2, "b", json_integer_new(0));
    JSONT_VALUE_EQUAL(val1, val2, true);

    val1 = json_object_new();
    json_object_add_member(val1, "a", json_boolean_new(true));
    json_object_add_member(val1, "b", json_integer_new(0));
    val2 = json_object_new();
    json_object_add_member(val2, "b", json_integer_new(0));
    json_object_add_member(val2, "a", json_boolean_new(true));
    JSONT_VALUE_EQUAL(val1, val2, true);

    val1 = json_object_new();
    json_object_add_member(val1, "a", json_boolean_new(true));
    json_object_add_member(val1, "b", json_integer_new(0));
    val2 = json_object_new();
    json_object_add_member(val2, "a", json_integer_new(0));
    json_object_add_member(val2, "b", json_null_new());
    JSONT_VALUE_EQUAL(val1, val2, false);

    val1 = json_object_new();
    json_object_add_member(val1, "a", json_boolean_new(true));
    json_object_add_member(val1, "b", json_integer_new(0));
    val2 = json_object_new();
    json_object_add_member(val2, "a", json_boolean_new(true));
    json_object_add_member(val2, "c", json_integer_new(0));
    JSONT_VALUE_EQUAL(val1, val2, false);

    val1 = json_object_new();
    json_object_add_member(val1, "a", json_boolean_new(true));
    json_object_add_member(val1, "a", json_integer_new(0));
    val2 = json_object_new();
    json_object_add_member(val2, "a", json_integer_new(0));
    json_object_add_member(val2, "a", json_boolean_new(true));
    JSONT_VALUE_EQUAL(val1, val2, true);

    val1 = json_object_new();
    tmp = json_object_new();
    json_object_add_member(tmp, "b", json_integer_new(2));
    json_object_add_member(tmp, "b", json_integer_new(1));
    json_object_add_member(val1, "a", tmp);
    tmp = json_object_new();
    json_object_add_member(tmp, "b", json_integer_new(1));
    json_object_add_member(tmp, "b", json_integer_new(2));
    json_object_add_member(val1, "a", tmp);
    val2 = json_object_new();
    tmp = json_object_new();
    json_object_add_member(tmp, "b", json_integer_new(1));
    json_object_add_member(tmp, "b", json_integer_new(2));
    json_object_add_member(val2, "a", tmp);
    tmp = json_object_new();
    json_object_add_member(tmp, "b", json_integer_new(2));
    json_object_add_member(tmp, "b", json_integer_new(1));
    json_object_add_member(val2, "a", tmp);
    JSONT_VALUE_EQUAL(val1, val2, true);

#undef JSONT_VALUE_EQUAL
}

int
main(int argc, char **argv) {
    struct test_suite *suite;

    suite = test_suite_new("json");
    test_suite_initialize_from_args(suite, argc, argv);

    test_suite_start(suite);

    TEST_RUN(suite, arrays);
    TEST_RUN(suite, integers);
    TEST_RUN(suite, reals);
    TEST_RUN(suite, strings);
    TEST_RUN(suite, booleans);
    TEST_RUN(suite, null);
    TEST_RUN(suite, objects);
    TEST_RUN(suite, object_iterators);
    TEST_RUN(suite, object_remove_member);

    TEST_RUN(suite, invalid);
    TEST_RUN(suite, invalid_arrays);
    TEST_RUN(suite, invalid_integers);
    TEST_RUN(suite, invalid_reals);
    TEST_RUN(suite, invalid_strings);
    TEST_RUN(suite, invalid_literals);
    TEST_RUN(suite, invalid_objects);

    TEST_RUN(suite, value_equal);

    test_suite_print_results_and_exit(suite);
}
