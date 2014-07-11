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

TEST(arrays) {
    struct json_value *value;

    JSONT_BEGIN_ARRAY("[]", 0);
    JSONT_END();

    JSONT_BEGIN_ARRAY(" [\t\n ] ", 0);
    JSONT_END();
}

TEST(integers) {
    struct json_value *value;

    JSONT_BEGIN_ARRAY("[0, 1, -1, 42, -127]", 5);
    JSONT_INTEGER_EQ(json_array_element(value, 0), 0);
    JSONT_INTEGER_EQ(json_array_element(value, 1), 1);
    JSONT_INTEGER_EQ(json_array_element(value, 2), -1);
    JSONT_INTEGER_EQ(json_array_element(value, 3), 42);
    JSONT_INTEGER_EQ(json_array_element(value, 4), -127);
    JSONT_END();

    JSONT_BEGIN_ARRAY("[-9223372036854775808, 9223372036854775807]", 2);
    JSONT_INTEGER_EQ(json_array_element(value, 0), INT64_MIN);
    JSONT_INTEGER_EQ(json_array_element(value, 1), INT64_MAX);
    JSONT_END();
}

TEST(reals) {
    struct json_value *value;

    JSONT_BEGIN_ARRAY("[0.0, -0.1, 1337.0042, 5e3, -4.38e117]", 5);
    JSONT_REAL_EQ(json_array_element(value, 0), 0.0);
    JSONT_REAL_EQ(json_array_element(value, 1), -0.1);
    JSONT_REAL_EQ(json_array_element(value, 2), 1337.0042);
    JSONT_REAL_EQ(json_array_element(value, 3), 5e3);
    JSONT_REAL_EQ(json_array_element(value, 4), -4.38e117);
    JSONT_END();
}

TEST(strings) {
    struct json_value *value;

    JSONT_BEGIN_ARRAY("[\"\", \"a\", \"foo bar\"]", 3);
    JSONT_STRING_EQ(json_array_element(value, 0), "");
    JSONT_STRING_EQ(json_array_element(value, 1), "a");
    JSONT_STRING_EQ(json_array_element(value, 2), "foo bar");
    JSONT_END();

    JSONT_BEGIN_ARRAY("[\"\\\"\", \"\\\\\", \"\\/\"]", 3);
    JSONT_STRING_EQ(json_array_element(value, 0), "\"");
    JSONT_STRING_EQ(json_array_element(value, 1), "\\");
    JSONT_STRING_EQ(json_array_element(value, 2), "/");
    JSONT_END();

    JSONT_BEGIN_ARRAY("[\"\\b\\f\\n\\r\\t\"]", 1);
    JSONT_STRING_EQ(json_array_element(value, 0), "\b\f\n\r\t");
    JSONT_END();

    JSONT_BEGIN_ARRAY("[\"\\u0061\", \"\\u00e0\\U00E9\"]", 2);
    JSONT_STRING_EQ(json_array_element(value, 0), "a");
    JSONT_STRING_EQ(json_array_element(value, 1), "àé");
    JSONT_END();

    JSONT_BEGIN_ARRAY("[\"\\u250c\\u2500\\u2510\"]", 1);
    JSONT_STRING_EQ(json_array_element(value, 0), "┌─┐");
    JSONT_END();

    JSONT_BEGIN_ARRAY("[\"\\ud834\\udd1e\", \"\\UD834\\UDD1E\"]", 2);
    JSONT_STRING_EQ(json_array_element(value, 0), "𝄞");
    JSONT_STRING_EQ(json_array_element(value, 1), "𝄞");
    JSONT_END();

    JSONT_BEGIN_ARRAY2("[\"foo\0bar\", \"\0tail\", \"head\0\"]", 29, 3);
    JSONT_STRING2_EQ(json_array_element(value, 0), "foo\0bar", 7);
    JSONT_STRING2_EQ(json_array_element(value, 1), "\0tail", 5);
    JSONT_STRING2_EQ(json_array_element(value, 2), "head\0", 5);
    JSONT_END();

    JSONT_BEGIN_ARRAY2("[\"foo\\u0000bar\", \"\\u0000tail\", \"head\\u0000\"]",
                       44, 3);
    JSONT_STRING2_EQ(json_array_element(value, 0), "foo\0bar", 7);
    JSONT_STRING2_EQ(json_array_element(value, 1), "\0tail", 5);
    JSONT_STRING2_EQ(json_array_element(value, 2), "head\0", 5);
    JSONT_END();
}

TEST(booleans) {
    struct json_value *value;

    JSONT_BEGIN_ARRAY("[true, false]", 2);
    JSONT_BOOLEAN_EQ(json_array_element(value, 0), true);
    JSONT_BOOLEAN_EQ(json_array_element(value, 1), false);
    JSONT_END();
}

TEST(null) {
    struct json_value *value;

    JSONT_BEGIN_ARRAY("[null]", 1);
    JSONT_NULL_EQ(json_array_element(value, 0));
    JSONT_END();
}

TEST(objects) {
    struct json_value *value, *child;

    JSONT_BEGIN_OBJECT("{}", 0);
    JSONT_END();

    JSONT_BEGIN_OBJECT(" {\t\n  } ", 0);
    JSONT_END();

    JSONT_BEGIN_OBJECT("{\"a\": 1, \"b\"  :2  ,  \"c\":3}", 3);
    TEST_TRUE(json_object_has_member(value, "a"));
    JSONT_INTEGER_EQ(json_object_member(value, "a"), 1);
    TEST_TRUE(json_object_has_member(value, "b"));
    JSONT_INTEGER_EQ(json_object_member(value, "b"), 2);
    TEST_TRUE(json_object_has_member(value, "c"));
    JSONT_INTEGER_EQ(json_object_member(value, "c"), 3);
    JSONT_END();

    JSONT_BEGIN_OBJECT("{\"\": 1, \"   \": 2}", 2);
    TEST_TRUE(json_object_has_member(value, ""));
    JSONT_INTEGER_EQ(json_object_member(value, ""), 1);
    TEST_TRUE(json_object_has_member(value, "   "));
    JSONT_INTEGER_EQ(json_object_member(value, "   "), 2);
    JSONT_END();

    JSONT_BEGIN_OBJECT2("{\"foo\0bar\": 1, \"\0tail\": 2, \"head\0\": 3}", 38, 3);
    TEST_TRUE(json_object_has_member2(value, "foo\0bar", 7));
    JSONT_INTEGER_EQ(json_object_member2(value, "foo\0bar", 7), 1);
    TEST_TRUE(json_object_has_member2(value, "\0tail", 5));
    JSONT_INTEGER_EQ(json_object_member2(value, "\0tail", 5), 2);
    TEST_TRUE(json_object_has_member2(value, "head\0", 5));
    JSONT_INTEGER_EQ(json_object_member2(value, "head\0", 5), 3);
    JSONT_END();

    JSONT_BEGIN_OBJECT("{\"a\": {\"aa\": 1}, \"b\": [{\"ba\": 1, \"bb\": 2}]}", 2);
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
    JSONT_END();
}

TEST(object_iterators) {
    struct json_value *value, *key, *val;
    struct json_object_iterator *it;

    JSONT_BEGIN("{}");
    it = json_object_iterate(value);
    TEST_INT_EQ(json_object_iterator_get_next(it, &key, &val), 0);
    TEST_INT_EQ(json_object_iterator_get_next(it, &key, &val), 0);
    json_object_iterator_delete(it);
    JSONT_END();

    JSONT_BEGIN("{\"a\": 1, \"b\": 2, \"c\": 3}");
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
    JSONT_END();
}

TEST(object_remove_member) {
    struct json_value *value;

    JSONT_BEGIN("{\"a\": 1}");
    json_object_remove_member(value, "a");
    TEST_UINT_EQ(json_object_nb_members(value), 0);
    TEST_FALSE(json_object_has_member(value, "a"));
    JSONT_END();

    JSONT_BEGIN("{\"a\": 1, \"b\": 2, \"c\": 3, \"d\": 4, \"e\": 5}");
    json_object_remove_member(value, "a");
    TEST_UINT_EQ(json_object_nb_members(value), 4);
    TEST_FALSE(json_object_has_member(value, "a"));
    json_object_remove_member(value, "c");
    TEST_UINT_EQ(json_object_nb_members(value), 3);
    TEST_FALSE(json_object_has_member(value, "c"));
    json_object_remove_member(value, "e");
    TEST_UINT_EQ(json_object_nb_members(value), 2);
    TEST_FALSE(json_object_has_member(value, "e"));
    JSONT_END();

    JSONT_BEGIN("{}");
    json_object_remove_member(value, "a");
    TEST_UINT_EQ(json_object_nb_members(value), 0);
    JSONT_END();

    JSONT_BEGIN("{\"a\": 1}");
    json_object_remove_member(value, "b");
    TEST_UINT_EQ(json_object_nb_members(value), 1);
    TEST_TRUE(json_object_has_member(value, "a"));
    JSONT_END();

    JSONT_BEGIN("{\"a\": 1, \"b\": 2, \"c\": 3, \"b\": 4}");
    json_object_remove_member(value, "b");
    TEST_UINT_EQ(json_object_nb_members(value), 2);
    TEST_FALSE(json_object_has_member(value, "b"));
    JSONT_END();
}

TEST(invalid) {
    JSONT_IS_INVALID("");
    JSONT_IS_INVALID("1");
    JSONT_IS_INVALID("1.0");
    JSONT_IS_INVALID("\"\"");
    JSONT_IS_INVALID("true");
    JSONT_IS_INVALID("null");
}

TEST(invalid_arrays) {
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
}

TEST(invalid_integers) {
    JSONT_IS_INVALID("[92233720368547758080]");
    JSONT_IS_INVALID("[9223372036854775808]");
    JSONT_IS_INVALID("[-9223372036854775809]");
    JSONT_IS_INVALID("[1foo]");
}

TEST(invalid_reals) {
    JSONT_IS_INVALID("[1.0foo]");
    JSONT_IS_INVALID("[1.0efoo]");
}

TEST(invalid_strings) {
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
}

TEST(invalid_literals) {
    JSONT_IS_INVALID("[yes]");
    JSONT_IS_INVALID("[nullx]");
    JSONT_IS_INVALID("[t]");
    JSONT_IS_INVALID("[f]");
    JSONT_IS_INVALID("[n]");
}

TEST(invalid_objects) {
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

    test_suite_print_results_and_exit(suite);
}
