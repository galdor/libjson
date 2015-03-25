/*
 * Copyright (c) 2015 Nicolas Martyanoff
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

#define JSONT_SCHEMA_VALID(schema_string_, json_string_)          \
    do {                                                          \
        struct json_schema *schema;                               \
        struct json_value *value;                                 \
                                                                  \
        schema = json_schema_parse(schema_string_,                \
                                   strlen(schema_string_));       \
        if (!schema)                                              \
            TEST_ABORT("cannot parse schema: %s", c_get_error()); \
                                                                  \
        value = json_parse(json_string_, strlen(json_string_),    \
                           JSON_PARSE_DEFAULT);                   \
        if (!value)                                               \
            TEST_ABORT("cannot parse value: %s", c_get_error());  \
                                                                  \
        if (json_schema_validate(schema, value) == -1)            \
            TEST_ABORT("validation failed: %s", c_get_error());   \
                                                                  \
        json_value_delete(value);                                 \
        json_schema_delete(schema);                               \
    } while (0)

#define JSONT_SCHEMA_INVALID(schema_string_, json_string_)        \
    do {                                                          \
        struct json_schema *schema;                               \
        struct json_value *value;                                 \
                                                                  \
        schema = json_schema_parse(schema_string_,                \
                                   strlen(schema_string_));       \
        if (!schema)                                              \
            TEST_ABORT("cannot parse schema: %s", c_get_error()); \
                                                                  \
        value = json_parse(json_string_, strlen(json_string_),    \
                           JSON_PARSE_DEFAULT);                   \
        if (!value)                                               \
            TEST_ABORT("cannot parse value: %s", c_get_error());  \
                                                                  \
        if (json_schema_validate(schema, value) == 0)             \
            TEST_ABORT("validation succeeded");                   \
                                                                  \
        json_value_delete(value);                                 \
        json_schema_delete(schema);                               \
    } while (0)

TEST(empty) {
    JSONT_SCHEMA_VALID("{}", "[1, true, {\"foo\": 42}]");
    JSONT_SCHEMA_VALID("{}", "{}");
}

TEST(array) {
    /* minItems/maxItems */
    JSONT_SCHEMA_VALID("{\"minItems\": 2, \"maxItems\": 4}", "[1,2]");
    JSONT_SCHEMA_VALID("{\"minItems\": 2, \"maxItems\": 4}", "[1,2,3]");
    JSONT_SCHEMA_VALID("{\"minItems\": 2, \"maxItems\": 4}", "[1,2,3,4]");
    JSONT_SCHEMA_INVALID("{\"minItems\": 2, \"maxItems\": 4}", "[1]");
    JSONT_SCHEMA_INVALID("{\"minItems\": 2, \"maxItems\": 4}", "[1,2,3,4,5]");

    /* uniqueItems */
    JSONT_SCHEMA_VALID("{\"uniqueItems\": true}", "[]");
    JSONT_SCHEMA_VALID("{\"uniqueItems\": true}", "[1, 2, 3]");
    JSONT_SCHEMA_VALID("{\"uniqueItems\": true}", "[\"\", {}, true, null]");
    JSONT_SCHEMA_INVALID("{\"uniqueItems\": true}", "[1, 1]");
    JSONT_SCHEMA_INVALID("{\"uniqueItems\": true}", "[\"foo\", \"foo\"]");
    JSONT_SCHEMA_VALID("{\"uniqueItems\": false}", "[]");
    JSONT_SCHEMA_VALID("{\"uniqueItems\": false}", "[1, 1]");

    /* items/additionaItems */
    JSONT_SCHEMA_VALID("{\"items\": {\"type\": \"integer\"}}",
                       "[1,2,3]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"type\": \"integer\"}}",
                         "[1,2,\"3\"]");

    JSONT_SCHEMA_VALID("{\"items\": [{\"type\": \"integer\"},"
                       "             {\"type\": \"string\"}]}",
                       "[1,\"2\"]");
    JSONT_SCHEMA_INVALID("{\"items\": [{\"type\": \"integer\"}]}",
                         "[1,2,3]");
    JSONT_SCHEMA_INVALID("{\"items\": [{\"type\": \"integer\"},"
                         "             {\"type\": \"string\"}]}",
                         "[1,2]");

    JSONT_SCHEMA_VALID("{\"items\": [{\"type\": \"integer\"}],"
                       " \"additionalItems\": false}",
                       "[1]");
    JSONT_SCHEMA_INVALID("{\"items\": [{\"type\": \"integer\"}],"
                         " \"additionalItems\": false}",
                         "[1,2]");

    JSONT_SCHEMA_VALID("{\"items\": [{\"type\": \"integer\"}],"
                       " \"additionalItems\": {\"enum\": [true, null]}}",
                       "[1]");
    JSONT_SCHEMA_VALID("{\"items\": [{\"type\": \"integer\"}],"
                       " \"additionalItems\": {\"enum\": [true, null]}}",
                       "[1,true,null]");
    JSONT_SCHEMA_INVALID("{\"items\": [{\"type\": \"integer\"}],"
                         " \"additionalItems\": {\"enum\": [true, null]}}",
                         "[1,2,3]");
}

TEST(object) {
    /* minProperties/maxProperties */
    JSONT_SCHEMA_VALID("{\"minProperties\":2, \"maxProperties\":4}",
                       "{\"a\":1, \"b\":2}");
    JSONT_SCHEMA_VALID("{\"minProperties\":2, \"maxProperties\":4}",
                       "{\"a\":1, \"b\":2, \"c\":3}");
    JSONT_SCHEMA_VALID("{\"minProperties\":2, \"maxProperties\":4}",
                       "{\"a\":1, \"b\":2, \"c\":3, \"d\":4}");
    JSONT_SCHEMA_INVALID("{\"minProperties\":2, \"maxProperties\":4}",
                         "{\"a\":1}");
    JSONT_SCHEMA_INVALID("{\"minProperties\":2, \"maxProperties\":4}",
                         "{\"a\":1, \"b\":2, \"c\":3, \"d\":4, \"e\":5}");

    /* required */
    JSONT_SCHEMA_VALID("{\"required\": [\"a\", \"b\"]}",
                       "{\"a\":1, \"b\":2}");
    JSONT_SCHEMA_VALID("{\"required\": [\"a\", \"b\"]}",
                       "{\"a\":1, \"b\":2, \"c\":3, \"d\":4}");
    JSONT_SCHEMA_INVALID("{\"required\": [\"a\", \"b\"]}",
                         "{\"a\":1, \"c\":3, \"d\":4}");
    JSONT_SCHEMA_INVALID("{\"required\": [\"a\", \"b\"]}", "{}");

    /* properties/additionalProperties/patternProperties */
    JSONT_SCHEMA_VALID("{\"properties\": {\"a\": {\"type\": \"integer\"},"
                                         "\"b\": {\"type\": \"boolean\"}}}",
                       "{\"a\": 1, \"b\": true}");
    JSONT_SCHEMA_INVALID("{\"properties\": {\"a\": {\"type\": \"integer\"},"
                                           "\"b\": {\"type\": \"boolean\"}}}",
                         "{\"a\": 1, \"b\": null}");

    JSONT_SCHEMA_VALID("{\"properties\": {\"a\": {\"type\": \"integer\"},"
                                         "\"b\": {\"type\": \"boolean\"}}}",
                       "{\"a\": 1, \"b\": true, \"c\": null}");
    JSONT_SCHEMA_INVALID("{\"properties\": {\"a\": {\"type\": \"integer\"},"
                                           "\"b\": {\"type\": \"boolean\"}},"
                         " \"additionalProperties\": false}",
                         "{\"a\": 1, \"b\": true, \"c\": null}");

    JSONT_SCHEMA_VALID("{\"properties\": {\"a\": {\"type\": \"integer\"},"
                                         "\"b\": {\"type\": \"boolean\"}},"
                       " \"additionalProperties\": {\"type\": \"string\"}}",
                       "{\"a\": 1, \"b\": true, \"c\": \"foo\"}");
    JSONT_SCHEMA_INVALID("{\"properties\": {\"a\": {\"type\": \"integer\"},"
                                           "\"b\": {\"type\": \"boolean\"}},"
                         " \"additionalProperties\": {\"type\": \"string\"}}",
                         "{\"a\": 1, \"b\": true, \"c\": null}");

    /* patternProperties */
    JSONT_SCHEMA_VALID("{\"properties\": {\"a\": {\"type\": \"integer\"},"
                                         "\"b\": {\"type\": \"boolean\"}},"
                       " \"patternProperties\": {\"^f\": {\"type\": \"null\"}}}",
                       "{\"a\": 1, \"b\": true, \"foo\": null}");
    JSONT_SCHEMA_INVALID("{\"properties\": {\"a\": {\"type\": \"integer\"},"
                                           "\"b\": {\"type\": \"boolean\"}},"
                         " \"patternProperties\": {\"^f\": {\"type\": \"null\"}}}",
                         "{\"a\": 1, \"b\": true, \"foo\": 42}");

    JSONT_SCHEMA_VALID("{\"properties\": {\"a\": {\"type\": \"integer\"},"
                                         "\"b\": {\"type\": \"boolean\"}},"
                       " \"patternProperties\": {\"^a\": {\"minimum\": 10}}}",
                       "{\"a\": 12, \"b\": true}");
    JSONT_SCHEMA_INVALID("{\"properties\": {\"a\": {\"type\": \"integer\"},"
                                           "\"b\": {\"type\": \"boolean\"}},"
                         " \"patternProperties\": {\"^a\": {\"minimum\": 10}}}",
                         "{\"a\": 8, \"b\": true}");
    JSONT_SCHEMA_INVALID("{\"properties\": {\"a\": {\"type\": \"integer\"},"
                                           "\"b\": {\"type\": \"boolean\"}},"
                         " \"patternProperties\": {\"^a\": {\"minimum\": 10}}}",
                         "{\"a\": 10.5, \"b\": true}");

    JSONT_SCHEMA_VALID("{\"properties\": {\"a\": {\"type\": \"integer\"},"
                                         "\"b\": {\"type\": \"boolean\"}},"
                       " \"patternProperties\": {\"^f\": {\"type\": \"null\"}},"
                       " \"additionalProperties\": {\"type\": \"string\"}}",
                       "{\"a\": 1, \"b\": true, \"d\": \"foobar\"}");
    JSONT_SCHEMA_INVALID("{\"properties\": {\"a\": {\"type\": \"integer\"},"
                                           "\"b\": {\"type\": \"boolean\"}},"
                         " \"patternProperties\": {\"^f\": {\"type\": \"null\"}},"
                         " \"additionalProperties\": {\"type\": \"string\"}}",
                         "{\"a\": 1, \"b\": true, \"d\": false}");

    /* TODO dependencies */
}

TEST(generic) {
    /* type */
    JSONT_SCHEMA_VALID("{\"type\": \"array\"}", "[]");
    JSONT_SCHEMA_INVALID("{\"type\": \"array\"}", "{}");
    JSONT_SCHEMA_VALID("{\"type\": [\"array\", \"object\"]}", "[]");
    JSONT_SCHEMA_VALID("{\"type\": [\"array\", \"object\"]}", "{}");
    JSONT_SCHEMA_INVALID("{\"type\": [\"integer\", \"null\"]}", "{}");

    /* enum */
    JSONT_SCHEMA_VALID("{\"enum\": [[], [1], [1,2]]}", "[]");
    JSONT_SCHEMA_VALID("{\"enum\": [[], [1], [1,2]]}", "[1,2]");
    JSONT_SCHEMA_INVALID("{\"enum\": [[], [1], [1,2]]}", "[1,2,3]");

    /* allOf */
    JSONT_SCHEMA_VALID("{\"allOf\": [{\"type\": \"array\"},"
                                    "{\"minItems\": 3}]}",
                       "[1,2,3]");
    JSONT_SCHEMA_INVALID("{\"allOf\": [{\"type\": \"array\"},"
                                      "{\"minItems\": 3}]}",
                         "[1,2]");
    JSONT_SCHEMA_INVALID("{\"allOf\": [{\"type\": \"array\"},"
                                      "{\"minItems\": 3}]}",
                         "{}");

    /* anyOf */
    JSONT_SCHEMA_VALID("{\"anyOf\": [{\"type\": \"object\"},"
                                    "{\"type\": \"array\", \"minItems\": 3}]}",
                       "{}");
    JSONT_SCHEMA_VALID("{\"anyOf\": [{\"type\": \"object\"},"
                                    "{\"type\": \"array\", \"minItems\": 3}]}",
                       "[1, 2, 3]");
    JSONT_SCHEMA_VALID("{\"anyOf\": [{\"type\": \"array\"},"
                                    "{\"minItems\": 2}]}",
                       "[1,2,3]");
    JSONT_SCHEMA_INVALID("{\"anyOf\": [{\"type\": \"object\"},"
                                      "{\"type\": \"array\", \"minItems\": 3}]}",
                         "[1]");

    /* oneOf */
    JSONT_SCHEMA_VALID("{\"oneOf\": [{\"type\": \"object\"},"
                                    "{\"type\": \"array\", \"minItems\": 3}]}",
                       "{}");
    JSONT_SCHEMA_VALID("{\"oneOf\": [{\"type\": \"object\"},"
                                    "{\"type\": \"array\", \"minItems\": 3}]}",
                       "[1, 2, 3]");
    JSONT_SCHEMA_INVALID("{\"oneOf\": [{\"type\": \"array\"},"
                                      "{\"minItems\": 2}]}",
                         "[1,2,3]");
    JSONT_SCHEMA_INVALID("{\"oneOf\": [{\"type\": \"object\"},"
                                      "{\"type\": \"array\", \"minItems\": 3}]}",
                         "[1]");

    /* not */
    JSONT_SCHEMA_VALID("{\"not\": {\"type\": \"object\"}}", "[]");
    JSONT_SCHEMA_INVALID("{\"not\": {\"type\": \"object\"}}", "{}");

    /* TODO format */
}

TEST(numeric) {
    /* multipleOf */
    JSONT_SCHEMA_VALID("{\"items\": {\"multipleOf\": 2}}",
                       "[-2,0,2,8]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"multipleOf\": 2}}", "[-1]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"multipleOf\": 2}}", "[3]");

    JSONT_SCHEMA_VALID("{\"items\": {\"multipleOf\": 2}}",
                       "[-2.0,0.0,2.0,8.0]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"multipleOf\": 2}}", "[-1.0]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"multipleOf\": 2}}", "[3.0]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"multipleOf\": 2}}", "[2.0001]");

    JSONT_SCHEMA_VALID("{\"items\": {\"multipleOf\": 1.5}}",
                       "[-4.5,0.0,1.5,3.0]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"multipleOf\": 1.5}}", "[-2.0]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"multipleOf\": 1.5}}", "[3.1]");

    JSONT_SCHEMA_VALID("{\"items\": {\"multipleOf\": 1.5}}", "[-6, 3]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"multipleOf\": 1.5}}", "[-7]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"multipleOf\": 1.5}}", "[2]");

    /* minimum/exclusiveMinimum */
    JSONT_SCHEMA_VALID("{\"items\": {\"minimum\": 0}}", "[0,0.0,1,2.0]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"minimum\": 0}}", "[-1]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"minimum\": 0}}", "[-2.5]");

    JSONT_SCHEMA_VALID("{\"items\": {\"minimum\": 0.0}}", "[0.0,0,3]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"minimum\": 0.0}}", "[-0.1]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"minimum\": 0.0}}", "[-3]");

    JSONT_SCHEMA_VALID("{\"items\": {\"minimum\": 0,"
                                    "\"exclusiveMinimum\": true}}", "[1,2.0]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"minimum\": 0,"
                                      "\"exclusiveMinimum\": true}}", "[0]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"minimum\": 0,"
                                       "\"exclusiveMinimum\": true}}", "[0.0]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"minimum\": 0,"
                                      "\"exclusiveMinimum\": true}}", "[-1]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"minimum\": 0,"
                                      "\"exclusiveMinimum\": true}}", "[-2.5]");

    /* maximum/exclusiveMaximum */
    JSONT_SCHEMA_VALID("{\"items\": {\"maximum\": 0}}", "[0,0.0,-1,-2.0]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"maximum\": 0}}", "[1]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"maximum\": 0}}", "[2.5]");

    JSONT_SCHEMA_VALID("{\"items\": {\"maximum\": 0.0}}", "[0.0,0,-3]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"maximum\": 0.0}}", "[0.1]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"maximum\": 0.0}}", "[3]");

    JSONT_SCHEMA_VALID("{\"items\": {\"maximum\": 0,"
                                    "\"exclusiveMaximum\": true}}", "[-1,-2.0]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"maximum\": 0,"
                                      "\"exclusiveMaximum\": true}}", "[0]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"maximum\": 0,"
                                       "\"exclusiveMaximum\": true}}", "[0.0]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"maximum\": 0,"
                                      "\"exclusiveMaximum\": true}}", "[1]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"maximum\": 0,"
                                      "\"exclusiveMaximum\": true}}", "[2.5]");
}

TEST(string) {
    /* minLength/maxLength */
    JSONT_SCHEMA_VALID("{\"items\": {\"minLength\": 0}}",
                       "[\"\"]");
    JSONT_SCHEMA_VALID("{\"items\": {\"minLength\": 3}}",
                       "[\"abc\", \"abcde\"]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"minLength\": 3}}",
                         "[\"ab\"]");

    JSONT_SCHEMA_VALID("{\"items\": {\"maxLength\": 0}}",
                       "[\"\"]");
    JSONT_SCHEMA_VALID("{\"items\": {\"maxLength\": 3}}",
                       "[\"\", \"ab\", \"abc\"]");
    JSONT_SCHEMA_VALID("{\"items\": {\"maxLength\": 6}}",
                       "[\"résumé\"]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"maxLength\": 3}}",
                         "[\"abcd\"]");

    /* pattern */
    JSONT_SCHEMA_VALID("{\"items\": {\"pattern\": \"^[0-9]+$\"}}",
                       "[\"1\", \"42\"]");
    JSONT_SCHEMA_VALID("{\"items\": {\"pattern\": \"^[0-9]+$\"}}",
                       "[\"1\", true, \"42\"]");
    JSONT_SCHEMA_INVALID("{\"items\": {\"pattern\": \"^[0-9]+$\"}}",
                         "[\"1\", \"42\", \"foo\"]");
}

#undef JSONT_SCHEMA_VALID
#undef JSONT_SCHEMA_INVALID

int
main(int argc, char **argv) {
    struct test_suite *suite;

    suite = test_suite_new("schema");
    test_suite_initialize_from_args(suite, argc, argv);

    test_suite_start(suite);

    TEST_RUN(suite, empty);
    TEST_RUN(suite, array);
    TEST_RUN(suite, object);
    TEST_RUN(suite, generic);
    TEST_RUN(suite, numeric);
    TEST_RUN(suite, string);

    test_suite_print_results_and_exit(suite);
}
