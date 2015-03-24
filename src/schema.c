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

#include "internal.h"

/* Schema */
static struct json_schema *
json_schema_parse_object(const struct json_value *);
static struct c_ptr_vector *
json_schema_parse_schema_array(const struct json_value *);

/* Generic */
static struct c_ptr_vector *
json_schema_parse_validator_enum(const struct json_value *);
static struct c_vector *
json_schema_parse_validator_type(const struct json_value *);

/* Numeric */
static struct json_value *
json_schema_parse_validator_multiple_of(const struct json_value *);

static bool
json_schema_value_is_multiple_of_integer(const struct json_value *, int64_t);
static bool
json_schema_value_is_multiple_of_real(const struct json_value *, double);

static bool
json_schema_value_lt_integer(const struct json_value *, int64_t, bool);
static bool
json_schema_value_gt_integer(const struct json_value *, int64_t, bool);
static bool
json_schema_value_lt_real(const struct json_value *, double, bool);
static bool
json_schema_value_gt_real(const struct json_value *, double, bool);

/* Array */
static int
json_schema_parse_validator_additional_items(const struct json_value *,
                                             struct json_schema **);
static struct c_ptr_vector *
json_schema_parse_validator_items(const struct json_value *);
static struct c_ptr_vector *
json_schema_parse_validator_required(const struct json_value *);

/* Object */
static int
json_schema_parse_validator_additional_properties(const struct json_value *,
                                                  struct json_schema **);
static struct c_vector *
json_schema_parse_validator_properties(const struct json_value *);
static struct c_vector *
json_schema_parse_validator_pattern_properties(const struct json_value *);
static int
json_schema_parse_validator_dependencies(struct json_object_validator *,
                                         const struct json_value *);

/* Validator */
static struct c_hash_table *
json_schema_parse_validator_definitions(const struct json_value *);

/* Misc */
static void json_value_vector_delete(struct c_ptr_vector *);
static void json_schema_vector_delete(struct c_ptr_vector *);
static void json_schema_table_delete(struct c_hash_table *);
static void json_string_vector_delete(struct c_ptr_vector *);
static void json_string_vector_table_delete(struct c_hash_table *);
static void json_object_validator_property_vector_delete(struct c_vector *);
static void json_object_validator_pattern_vector_delete(struct c_vector *);

static bool json_string_vector_contains(struct c_ptr_vector *, const char *);
static bool json_value_vector_contains(struct c_ptr_vector *,
                                       struct json_value *);


/* ------------------------------------------------------------------------
 *  Schema URI
 * ------------------------------------------------------------------------ */
int
json_schema_uri_parse(const char *string, enum json_schema_uri *puri) {
    struct {
        const char *string;
        enum json_schema_uri uri;
    } uris[] = {
        {"http://json-schema.org/schema#",          JSON_SCHEMA_URI_CURRENT},
        {"http://json-schema.org/draft-04/schema#", JSON_SCHEMA_URI_DRAFT_V4},
    };
    size_t nb_uris = sizeof(uris) / sizeof(uris[0]);

    for (size_t i = 0; i < nb_uris; i++) {
        if (strcmp(string, uris[i].string) == 0) {
            *puri = uris[i].uri;
            return 0;
        }
    }

    return -1;
}

/* ------------------------------------------------------------------------
 *  Simple types
 * ------------------------------------------------------------------------ */
int
json_schema_simple_type_parse(const char *string,
                              enum json_schema_simple_type *ptype) {
    struct {
        const char *string;
        enum json_schema_simple_type type;
    } types[] = {
        {"array",   JSON_SCHEMA_SIMPLE_ARRAY},
        {"boolean", JSON_SCHEMA_SIMPLE_BOOLEAN},
        {"integer", JSON_SCHEMA_SIMPLE_INTEGER},
        {"null",    JSON_SCHEMA_SIMPLE_NULL},
        {"number",  JSON_SCHEMA_SIMPLE_NUMBER},
        {"object",  JSON_SCHEMA_SIMPLE_OBJECT},
        {"string",  JSON_SCHEMA_SIMPLE_STRING},
    };
    size_t nb_types = sizeof(types) / sizeof(types[0]);

    for (size_t i = 0; i < nb_types; i++) {
        if (strcmp(string, types[i].string) == 0) {
            *ptype = types[i].type;
            return 0;
        }
    }

    return -1;
}

bool
json_schema_simple_type_matches_type(enum json_schema_simple_type stype,
                                     enum json_type type) {
    switch (stype) {
    case JSON_SCHEMA_SIMPLE_ARRAY:
        return type == JSON_ARRAY;
    case JSON_SCHEMA_SIMPLE_BOOLEAN:
        return type == JSON_BOOLEAN;
    case JSON_SCHEMA_SIMPLE_INTEGER:
        return type == JSON_INTEGER;
    case JSON_SCHEMA_SIMPLE_NULL:
        return type == JSON_NULL;
    case JSON_SCHEMA_SIMPLE_NUMBER:
        return type == JSON_INTEGER || type == JSON_REAL;
    case JSON_SCHEMA_SIMPLE_OBJECT:
        return type == JSON_OBJECT;
    case JSON_SCHEMA_SIMPLE_STRING:
        return type == JSON_STRING;
    }

    return false;
}

/* ------------------------------------------------------------------------
 *  Formats
 * ------------------------------------------------------------------------ */
int
json_schema_format_parse(const char *string, enum json_schema_format *pformat) {
    struct {
        const char *string;
        enum json_schema_format format;
    } formats[] = {
        {"date-time", JSON_SCHEMA_FORMAT_DATE_TIME},
        {"email",     JSON_SCHEMA_FORMAT_EMAIL},
        {"hostname",  JSON_SCHEMA_FORMAT_HOSTNAME},
        {"ipv4",      JSON_SCHEMA_FORMAT_IPV4},
        {"ipv6",      JSON_SCHEMA_FORMAT_IPV6},
        {"uri",       JSON_SCHEMA_FORMAT_URI},
        {"regex",     JSON_SCHEMA_FORMAT_REGEX},
    };
    size_t nb_formats = sizeof(formats) / sizeof(formats[0]);

    for (size_t i = 0; i < nb_formats; i++) {
        if (strcmp(string, formats[i].string) == 0) {
            *pformat = formats[i].format;
            return 0;
        }
    }

    return -1;
}

/* ------------------------------------------------------------------------
 *  Generic validator
 * ------------------------------------------------------------------------ */
void
json_generic_validator_init(struct json_generic_validator *validator) {
    memset(validator, 0, sizeof(struct json_generic_validator));
}

void
json_generic_validator_free(struct json_generic_validator *validator) {
    if (!validator)
        return;

    c_vector_delete(validator->types);

    json_value_vector_delete(validator->enumeration);

    json_schema_vector_delete(validator->all_of);
    json_schema_vector_delete(validator->any_of);
    json_schema_vector_delete(validator->one_of);
    json_schema_delete(validator->not);

    memset(validator, 0, sizeof(struct json_generic_validator));
}

static struct c_ptr_vector *
json_schema_parse_validator_enum(const struct json_value *value) {
    struct c_ptr_vector *vector;

    if (value->type != JSON_ARRAY) {
        c_set_error("value is not an array");
        return NULL;
    }

    if (value->u.array.nb_elements == 0) {
        c_set_error("array is empty");
        return NULL;
    }

    vector = c_ptr_vector_new();

    for (size_t i = 0; i < json_array_nb_elements(value); i++) {
        struct json_value *element;

        element = json_array_element(value, i);

        if (json_value_vector_contains(vector, element)) {
            c_set_error("duplicate array element");
            json_value_vector_delete(vector);
            return NULL;
        }

        c_ptr_vector_append(vector, json_value_clone(element));
    }

    return vector;
}

static struct c_vector *
json_schema_parse_validator_type(const struct json_value *value) {
    struct c_vector *vector;

    vector = c_vector_new(sizeof(enum json_schema_simple_type));

    if (value->type == JSON_STRING) {
        enum json_schema_simple_type type;

        if (json_schema_simple_type_parse(value->u.string.ptr, &type) == -1) {
            c_set_error("unknown type '%s'", value->u.string.ptr);
            c_vector_delete(vector);
            return NULL;
        }

        c_vector_append(vector, &type);
    } else if (value->type == JSON_ARRAY) {
        if (value->u.array.nb_elements == 0) {
            c_set_error("array is empty");
            return NULL;
        }

        for (size_t i = 0; i < json_array_nb_elements(value); i++) {
            enum json_schema_simple_type type;
            struct json_value *element;

            element = json_array_element(value, i);
            if (element->type != JSON_STRING) {
                c_set_error("array element is not a string");
                c_vector_delete(vector);
                return NULL;
            }

            if (json_schema_simple_type_parse(element->u.string.ptr,
                                              &type) == -1) {
                c_set_error("unknown simple type '%s'", element->u.string.ptr);
                c_vector_delete(vector);
                return NULL;
            }

            c_vector_append(vector, &type);
        }
    } else {
        c_set_error("value is not a string or array");
        c_vector_delete(vector);
        return NULL;
    }

    return vector;
}

/* ------------------------------------------------------------------------
 *  Number validator
 * ------------------------------------------------------------------------ */
void
json_numeric_validator_init(struct json_numeric_validator *validator) {
    memset(validator, 0, sizeof(struct json_numeric_validator));
}

void
json_numeric_validator_free(struct json_numeric_validator *validator) {
    if (!validator)
        return;

    json_value_delete(validator->multiple_of);
    json_value_delete(validator->min);
    json_value_delete(validator->max);

    memset(validator, 0, sizeof(struct json_numeric_validator));
}

static struct json_value *
json_schema_parse_validator_multiple_of(const struct json_value *value) {
    switch (value->type) {
    case JSON_INTEGER:
        if (value->u.integer <= 0) {
            c_set_error("negative value");
            return NULL;
        }
        break;

    case JSON_REAL:
        if (value->u.real <= 0.0) {
            c_set_error("negative value");
            return NULL;
        }
        break;

    default:
        c_set_error("value is not an integer or real");
        break;
    }

    return json_value_clone(value);
}

static bool
json_schema_value_is_multiple_of_integer(const struct json_value *value,
                                         int64_t integer) {
    assert(value->type == JSON_INTEGER || value->type == JSON_REAL);

    if (value->type == JSON_INTEGER) {
        return ((value->u.integer % integer) == 0);
    } else if (value->type == JSON_REAL) {
        if (value->u.real > (double)INT64_MAX
         || value->u.real < (double)INT64_MIN
         || trunc(value->u.real) != value->u.real) {
            return false;
        }

        return (((int64_t)value->u.real % integer) == 0);
    }

    return false;
}

static bool
json_schema_value_is_multiple_of_real(const struct json_value *value,
                                      double real) {
    assert(value->type == JSON_INTEGER || value->type == JSON_REAL);


    if (value->type == JSON_INTEGER) {
        double ratio;

        /* FIXME */
        ratio = (double)value->u.integer / real;
        return (trunc(ratio) == ratio);
    } else if (value->type == JSON_REAL) {
        double ratio;

        ratio = value->u.real / real;
        return (trunc(ratio) == ratio);
    }

    return false;
}

static bool
json_schema_value_lt_integer(const struct json_value *value, int64_t integer,
                             bool exclusive) {
    assert(value->type == JSON_INTEGER || value->type == JSON_REAL);

    if (value->type == JSON_INTEGER) {
        if (exclusive) {
            return value->u.integer < integer;
        } else {
            return value->u.integer <= integer;
        }
    } else if (value->type == JSON_REAL) {
        /* FIXME */
        if (exclusive) {
            return (int64_t)value->u.real < integer;
        } else {
            return (int64_t)value->u.real <= integer;
        }
    }

    return false;
}

static bool
json_schema_value_gt_integer(const struct json_value *value, int64_t integer,
                             bool exclusive) {
    assert(value->type == JSON_INTEGER || value->type == JSON_REAL);

    if (value->type == JSON_INTEGER) {
        if (exclusive) {
            return value->u.integer > integer;
        } else {
            return value->u.integer >= integer;
        }
    } else if (value->type == JSON_REAL) {
        if (exclusive) {
            /* FIXME */
            return (int64_t)value->u.real > integer;
        } else {
            return (int64_t)value->u.real >= integer;
        }
    }

    return false;
}

static bool
json_schema_value_lt_real(const struct json_value *value, double real,
                          bool exclusive) {
    assert(value->type == JSON_INTEGER || value->type == JSON_REAL);

    if (value->type == JSON_INTEGER) {
        if (exclusive) {
            return (double)value->u.integer < real;
        } else {
            return (double)value->u.integer <= real;
        }
    } else if (value->type == JSON_REAL) {
        if (exclusive) {
            return value->u.real < real;
        } else {
            return value->u.real <= real;
        }
    }

    return false;
}

static bool
json_schema_value_gt_real(const struct json_value *value, double real,
                          bool exclusive) {
    assert(value->type == JSON_INTEGER || value->type == JSON_REAL);

    if (value->type == JSON_INTEGER) {
        if (exclusive) {
            return (double)value->u.integer > real;
        } else {
            return (double)value->u.integer >= real;
        }
    } else if (value->type == JSON_REAL) {
        if (exclusive) {
            return value->u.real > real;
        } else {
            return value->u.real >= real;
        }
    }

    return false;
}

/* ------------------------------------------------------------------------
 *  String validator
 * ------------------------------------------------------------------------ */
void
json_string_validator_init(struct json_string_validator *validator) {
    memset(validator, 0, sizeof(struct json_string_validator));
}

void
json_string_validator_free(struct json_string_validator *validator) {
    if (!validator)
        return;

    c_free(validator->pattern);

    memset(validator, 0, sizeof(struct json_string_validator));
}

/* ------------------------------------------------------------------------
 *  Array validator
 * ------------------------------------------------------------------------ */
void
json_array_validator_init(struct json_array_validator *validator) {
    memset(validator, 0, sizeof(struct json_array_validator));
}

void
json_array_validator_free(struct json_array_validator *validator) {
    if (!validator)
        return;

    json_schema_vector_delete(validator->items);
    json_schema_delete(validator->additional_items);

    memset(validator, 0, sizeof(struct json_array_validator));
}

static int
json_schema_parse_validator_additional_items(const struct json_value *value,
                                             struct json_schema **pschema) {
    switch (value->type) {
    case JSON_BOOLEAN:
        *pschema = value->u.boolean ? json_schema_new() : NULL;
        break;

    case JSON_OBJECT:
        *pschema = json_schema_parse_object(value);
        if (!*pschema)
            return -1;
        break;

    default:
        c_set_error("value is not a boolean or object");
        return -1;
    }

    return 0;
}

static struct c_ptr_vector *
json_schema_parse_validator_items(const struct json_value *value) {
    struct c_ptr_vector *vector;
    struct json_schema *schema;

    if (value->type == JSON_OBJECT) {
        schema = json_schema_parse_object(value);
        if (!schema)
            return NULL;

        vector = c_ptr_vector_new();

        c_ptr_vector_append(vector, schema);

        return vector;
    } else if (value->type == JSON_ARRAY) {
        return json_schema_parse_schema_array(value);
    }

    c_set_error("value is not an object or array");
    return NULL;
}

static struct c_ptr_vector *
json_schema_parse_validator_required(const struct json_value *value) {
    struct c_ptr_vector *vector;

    if (value->type != JSON_ARRAY) {
        c_set_error("value is not an array");
        return NULL;
    }

    if (value->u.array.nb_elements == 0) {
        c_set_error("array is empty");
        return NULL;
    }

    vector = c_ptr_vector_new();

    for (size_t i = 0; i < json_array_nb_elements(value); i++) {
        struct json_value *element;
        char *string;

        element = json_array_element(value, i);

        if (element->type != JSON_STRING){
            c_set_error("array element is not a string");
            json_string_vector_delete(vector);
            return NULL;
        }

        if (json_string_vector_contains(vector, element->u.string.ptr)) {
            c_set_error("duplicate array element");
            json_string_vector_delete(vector);
            return NULL;
        }

        string = c_strndup(element->u.string.ptr, element->u.string.len);
        c_ptr_vector_append(vector, string);
    }

    return vector;
}

/* ------------------------------------------------------------------------
 *  Object validator
 * ------------------------------------------------------------------------ */
void
json_object_validator_init(struct json_object_validator *validator) {
    memset(validator, 0, sizeof(struct json_object_validator));
}

void
json_object_validator_free(struct json_object_validator *validator) {
    if (!validator)
        return;

    json_string_vector_delete(validator->required);

    json_object_validator_property_vector_delete(validator->properties);
    json_schema_delete(validator->additional_properties);
    json_object_validator_pattern_vector_delete(validator->pattern_properties);

    json_schema_table_delete(validator->schema_dependencies);
    json_string_vector_table_delete(validator->property_dependencies);

    memset(validator, 0, sizeof(struct json_object_validator));
}

static int
json_schema_parse_validator_additional_properties(const struct json_value *value,
                                                  struct json_schema **pschema) {
    switch (value->type) {
    case JSON_BOOLEAN:
        *pschema = value->u.boolean ? json_schema_new() : NULL;
        break;

    case JSON_OBJECT:
        *pschema = json_schema_parse_object(value);
        if (!*pschema)
            return -1;
        break;

    default:
        c_set_error("value is not a boolean or object");
        return -1;
    }

    return 0;
}

static struct c_vector *
json_schema_parse_validator_properties(const struct json_value *value) {
    struct c_vector *vector;

    if (value->type != JSON_OBJECT) {
        c_set_error("value is not an object");
        return NULL;
    }

    vector = c_vector_new(sizeof(struct json_object_validator_property));

    for (size_t i = 0; i < value->u.object.nb_members; i++) {
        struct json_object_validator_property property;
        const char *key;
        struct json_value *mvalue;

        key = json_object_nth_member(value, i, &mvalue);

        property.schema = json_schema_parse_object(mvalue);
        if (!property.schema) {
            json_object_validator_property_vector_delete(vector);
            return NULL;
        }
        property.string = c_strdup(key);

        c_vector_append(vector, &property);
    }

    return vector;
}

static struct c_vector *
json_schema_parse_validator_pattern_properties(const struct json_value *value) {
    struct c_vector *vector;

    if (value->type != JSON_OBJECT) {
        c_set_error("value is not an object");
        return NULL;
    }

    vector = c_vector_new(sizeof(struct json_object_validator_pattern));

    for (size_t i = 0; i < value->u.object.nb_members; i++) {
        struct json_object_validator_pattern pattern;
        const char *key;
        struct json_value *mvalue;

        key = json_object_nth_member(value, i, &mvalue);

        /* TODO validate regexp */

        pattern.schema = json_schema_parse_object(mvalue);
        if (!pattern.schema) {
            json_object_validator_pattern_vector_delete(vector);
            return NULL;
        }
        pattern.string = c_strdup(key);

        c_vector_append(vector, &pattern);
    }

    return vector;
}

static int
json_schema_parse_validator_dependencies(struct json_object_validator *validator,
                                         const struct json_value *value) {
    struct c_hash_table *schemas;
    struct c_hash_table *properties;

    if (value->type != JSON_OBJECT) {
        c_set_error("value is not an object");
        return -1;
    }

    schemas = c_hash_table_new(c_hash_string, c_equal_string);
    properties = c_hash_table_new(c_hash_string, c_equal_string);

    for (size_t i = 0; i < value->u.object.nb_members; i++) {
        struct json_value *mvalue;
        const char *key;

        key = json_object_nth_member(value, i, &mvalue);

        if (mvalue->type == JSON_OBJECT) {
            struct json_schema *schema;
            char *name;

            /* Schema dependency */

            schema = json_schema_parse_object(mvalue);
            if (!schema)
                goto error;

            name = c_strdup(key);

            c_hash_table_insert(schemas, name, schema);
        } else if (mvalue->type == JSON_ARRAY) {
            struct c_ptr_vector *vector;
            char *name;

            /* Property dependency */

            if (mvalue->u.array.nb_elements == 0) {
                c_set_error("empty array in property dependency");
                goto error;
            }

            vector = c_ptr_vector_new();

            for (size_t j = 0; j < mvalue->u.array.nb_elements; j++) {
                struct json_value *avalue;
                char *string;

                avalue = mvalue->u.array.elements[j];
                if (avalue->type != JSON_STRING) {
                    c_set_error("array element is not a string in "
                                "property dependency");
                    json_string_vector_delete(vector);
                    goto error;
                }

                if (json_string_vector_contains(vector, avalue->u.string.ptr)) {
                    c_set_error("duplicate array element");
                    json_string_vector_delete(vector);
                    goto error;
                }

                string = c_strndup(avalue->u.string.ptr, avalue->u.string.len);
                c_ptr_vector_append(vector, string);
            }

            name = c_strdup(key);

            c_hash_table_insert(properties, name, vector);
        } else {
            c_set_error("object member value is not an object or array");
            goto error;
        }
    }

    validator->schema_dependencies = schemas;
    validator->property_dependencies = properties;
    return 0;

error:
    json_schema_table_delete(schemas);
    json_string_vector_table_delete(properties);
    return -1;
}

/* ------------------------------------------------------------------------
 *  Validator
 * ------------------------------------------------------------------------ */
void
json_validator_init(struct json_validator *validator) {
    memset(validator, 0, sizeof(struct json_validator));

    json_generic_validator_init(&validator->generic);
    json_numeric_validator_init(&validator->numeric);
    json_string_validator_init(&validator->string);
    json_array_validator_init(&validator->array);
    json_object_validator_init(&validator->object);
}

void
json_validator_free(struct json_validator *validator) {
    if (!validator)
        return;

    json_generic_validator_free(&validator->generic);
    json_numeric_validator_free(&validator->numeric);
    json_string_validator_free(&validator->string);
    json_array_validator_free(&validator->array);
    json_object_validator_free(&validator->object);

    json_schema_table_delete(validator->definitions);

    memset(validator, 0, sizeof(struct json_validator));
}

static struct c_hash_table *
json_schema_parse_validator_definitions(const struct json_value *value) {
    struct c_hash_table *table;

    table = c_hash_table_new(c_hash_string, c_equal_string);

    for (size_t i = 0; i < value->u.object.nb_members; i++) {
        struct json_value *mvalue;
        struct json_schema *schema;
        const char *key;
        char *name;

        key = json_object_nth_member(value, i, &mvalue);

        schema = json_schema_parse_object(mvalue);
        if (!schema) {
            json_schema_table_delete(table);
            return NULL;
        }
        name = c_strdup(key);

        c_hash_table_insert(table, name, schema);
    }

    return table;
}

/* ------------------------------------------------------------------------
 *  Schema
 * ------------------------------------------------------------------------ */
struct json_schema *
json_schema_new(void) {
    struct json_schema *schema;

    schema = c_malloc0(sizeof(struct json_schema));

    json_validator_init(&schema->validator);

    return schema;
}

void
json_schema_delete(struct json_schema *schema) {
    if (!schema)
        return;

    c_free(schema->id);

    c_free(schema->title);
    c_free(schema->description);

    json_value_delete(schema->default_value);

    json_validator_free(&schema->validator);

    c_free0(schema, sizeof(struct json_schema));
}

/* ------------------------------------------------------------------------
 *  Validation
 * ------------------------------------------------------------------------ */
int
json_schema_validate(struct json_schema *schema, struct json_value *value) {
    return json_validator_check(&schema->validator, value);
}

int
json_validator_check(struct json_validator *validator,
                     struct json_value *value) {
    if (json_generic_validator_check(&validator->generic, value) == -1)
        return -1;

    switch (value->type) {
    case JSON_INTEGER:
    case JSON_REAL:
        return json_numeric_validator_check(&validator->numeric, value);

    case JSON_STRING:
        return json_string_validator_check(&validator->string, value);

    case JSON_ARRAY:
        return json_array_validator_check(&validator->array, value);

    case JSON_OBJECT:
        return json_object_validator_check(&validator->object, value);

    case JSON_BOOLEAN:
    case JSON_NULL:
        return 0;
    }

    return 0;
}

int
json_generic_validator_check(struct json_generic_validator *validator,
                             struct json_value *value) {
    bool match;

    /* type */
    if (validator->types) {
        match = false;

        for (size_t i = 0; i < c_vector_length(validator->types); i++) {
            enum json_schema_simple_type *stype;

            stype = c_vector_entry(validator->types, i);
            if (json_schema_simple_type_matches_type(*stype, value->type)) {
                match = true;
                break;
            }
        }

        if (!match) {
            c_set_error("value does not match 'type' constraint");
            return -1;
        }
    }

    /* enum */
    if (validator->enumeration) {
        match = false;

        for (size_t i = 0; i < c_ptr_vector_length(validator->enumeration); i++) {
            struct json_value *evalue;

            evalue = c_ptr_vector_entry(validator->enumeration, i);
            if (json_value_equal(evalue, value)) {
                match = true;
                break;
            }
        }

        if (!match) {
            c_set_error("value does not match 'enum' constraint");
            return -1;
        }
    }

    /* allOf */
    if (validator->all_of) {
        match = true;

        for (size_t i = 0; i < c_ptr_vector_length(validator->all_of); i++) {
            struct json_schema *sschema;

            sschema = c_ptr_vector_entry(validator->all_of, i);
            if (json_schema_validate(sschema, value) == -1) {
                match = false;
                break;
            }
        }

        if (!match) {
            c_set_error("value does not match 'allOf' constraint");
            return -1;
        }
    }

    /* anyOf */
    if (validator->any_of) {
        match = false;

        for (size_t i = 0; i < c_ptr_vector_length(validator->any_of); i++) {
            struct json_schema *sschema;

            sschema = c_ptr_vector_entry(validator->any_of, i);
            if (json_schema_validate(sschema, value) == 0) {
                match = true;
                break;
            }
        }

        if (!match) {
            c_set_error("value does not match 'anyOf' constraint");
            return -1;
        }
    }

    /* oneOf */
    if (validator->one_of) {
        size_t nb_matches;

        nb_matches = 0;

        for (size_t i = 0; i < c_ptr_vector_length(validator->one_of); i++) {
            struct json_schema *sschema;
            bool is_valid;

            sschema = c_ptr_vector_entry(validator->one_of, i);
            is_valid = (json_schema_validate(sschema, value) == 0);

            if (is_valid)
                nb_matches++;
        }

        if (nb_matches != 1) {
            c_set_error("value does not match 'oneOf' constraint");
            return -1;
        }
    }

    /* not */
    if (validator->not) {
        if (json_schema_validate(validator->not, value) == 0) {
            c_set_error("value does not match 'not' constraint");
            return -1;
        }
    }

    /* TODO format */

    return 0;
}

int
json_numeric_validator_check(struct json_numeric_validator *validator,
                             struct json_value *value) {
    /* multipleOf */
    if (validator->multiple_of) {
        struct json_value *vvalue;
        bool is_valid;

        vvalue = validator->multiple_of;

        if (vvalue->type == JSON_INTEGER) {
            is_valid = json_schema_value_is_multiple_of_integer(value,
                                                                vvalue->u.integer);
        } else {
            is_valid = json_schema_value_is_multiple_of_real(value,
                                                             vvalue->u.real);
        }

        if (!is_valid) {
            c_set_error("value does not match 'multipleOf' constraint");
            return -1;
        }
    }

    /* minimum/exclusiveMinimum */
    if (validator->min) {
        struct json_value *vvalue;
        bool is_valid;

        vvalue = validator->min;

        if (vvalue->type == JSON_INTEGER) {
            is_valid = json_schema_value_gt_integer(value, vvalue->u.integer,
                                                    validator->exclusive_min);
        } else {
            is_valid = json_schema_value_gt_real(value, vvalue->u.real,
                                                 validator->exclusive_min);
        }

        if (!is_valid) {
            c_set_error("number too small");
            return -1;
        }
    }

    /* maximum/exclusiveMaximum */
    if (validator->max) {
        struct json_value *vvalue;
        bool is_valid;

        vvalue = validator->max;

        if (vvalue->type == JSON_INTEGER) {
            is_valid = json_schema_value_lt_integer(value, vvalue->u.integer,
                                                    validator->exclusive_max);
        } else {
            is_valid = json_schema_value_lt_real(value, vvalue->u.real,
                                                 validator->exclusive_max);
        }

        if (!is_valid) {
            c_set_error("number too large");
            return -1;
        }
    }

    return 0;
}

int
json_string_validator_check(struct json_string_validator *validator,
                            struct json_value *value) {
    if (validator->has_min_length || validator->has_max_length) {
        size_t length;

        /* FIXME c_utf8_nb_codepoints() stops at the first U+0000 character.
         * Add a function that counts using a pointer and size. */
        if (c_utf8_nb_codepoints(value->u.string.ptr, &length) == -1) {
            c_set_error("invalid string: %s", c_get_error());
            return -1;
        }

        /* minLength */
        if (validator->has_min_length) {
            if (length < validator->min_length) {
                c_set_error("string too short");
                return -1;
            }
        }

        /* maxLength */
        if (validator->has_max_length) {
            if (length > validator->max_length) {
                c_set_error("string too long");
                return -1;
            }
        }
    }

    /* TODO pattern */
    if (validator->pattern) {
    }

    return 0;
}

int
json_array_validator_check(struct json_array_validator *validator,
                           struct json_value *value) {
    size_t nb_elements;

    nb_elements = value->u.array.nb_elements;

    /* minItems */
    if (validator->has_min_items) {
        if (nb_elements < validator->min_items) {
            c_set_error("array contains too few elements");
            return -1;
        }
    }

    /* maxItems */
    if (validator->has_max_items) {
        if (nb_elements > validator->max_items) {
            c_set_error("array contains too many elements");
            return -1;
        }
    }

    /* uniqueItems */
    if (validator->unique_items) {
        /* XXX inefficient */
        for (size_t i = 0; i < nb_elements; i++) {
            struct json_value *el1;

            el1 = value->u.array.elements[i];

            for (size_t j = 0; j < nb_elements; j++) {
                struct json_value *el2;

                if (i == j)
                    continue;

                el2 = value->u.array.elements[j];

                if (json_value_equal(el1, el2)) {
                    c_set_error("array elements are not unique");
                    return -1;
                }
            }
        }
    }

    /* items/additionalItems */
    if (validator->items) {
        size_t nb_schemas;

        nb_schemas = c_ptr_vector_length(validator->items);

        if (validator->items_is_array) {
            for (size_t i = 0; i < value->u.array.nb_elements; i++) {
                struct json_value *evalue;
                struct json_schema *eschema;

                evalue = value->u.array.elements[i];

                if (i < nb_schemas) {
                    eschema = c_ptr_vector_entry(validator->items, i);
                } else {
                    if (!validator->additional_items) {
                        c_set_error("array contains additional items");
                        return -1;
                    }

                    eschema = validator->additional_items;
                }

                if (json_schema_validate(eschema, evalue) == -1) {
                    c_set_error("array element %zu does not match "
                                "'%s' constraint: %s", i,
                                (i < nb_schemas) ? "items" : "additionalItems",
                                c_get_error());
                    return -1;
                }
            }
        } else {
            struct json_schema *eschema;

            assert(nb_schemas == 1);
            eschema = c_ptr_vector_entry(validator->items, 0);

            for (size_t i = 0; i < value->u.array.nb_elements; i++) {
                struct json_value *evalue;

                evalue = value->u.array.elements[i];

                if (json_schema_validate(eschema, evalue) == -1) {
                    c_set_error("array element %zu does not match 'items' "
                                "constraint", i);
                    return -1;
                }
            }
        }
    }

    return 0;
}

int
json_object_validator_check(struct json_object_validator *validator,
                            struct json_value *value) {
    size_t nb_members;

    nb_members = value->u.object.nb_members;

    /* minProperties */
    if (validator->has_min_properties) {
        if (nb_members < validator->min_properties) {
            c_set_error("object contains too few members");
            return -1;
        }
    }

    /* maxProperties */
    if (validator->has_max_properties) {
        if (nb_members > validator->max_properties) {
            c_set_error("object contains too many members");
            return -1;
        }
    }

    /* required */
    if (validator->required) {
        for (size_t i = 0; i < c_ptr_vector_length(validator->required); i++) {
            const char *key;

            key = c_ptr_vector_entry(validator->required, i);

            if (!json_object_has_member(value, key)) {
                c_set_error("object does not contain required members");
                return -1;
            }
        }
    }

    /* properties/additionalProperties/patternProperties */
    for (size_t i = 0; i < value->u.object.nb_members; i++) {
        const char *key;
        struct json_value *mvalue;
        struct json_schema *mschema1, *mschema2;

        key = json_object_nth_member(value, i, &mvalue);

        /* properties */
        mschema1 = NULL;
        if (validator->properties) {
            for (size_t j = 0; j < c_vector_length(validator->properties);
                 j++) {
                struct json_object_validator_property *vprop;

                vprop = c_vector_entry(validator->properties, j);
                if (strcmp(key, vprop->string) == 0) {
                    mschema1 = vprop->schema;
                    break;
                }
            }
        }

        if (mschema1) {
            if (json_schema_validate(mschema1, mvalue) == -1) {
                c_set_error("object member %zu does not match "
                            "'properties' constraint: %s", i, c_get_error());
                return -1;
            }
        }

        /* patternProperties */
        mschema2 = NULL;
        if (validator->pattern_properties) {
            for (size_t j = 0; j < c_vector_length(validator->pattern_properties);
                 j++) {
                struct json_object_validator_pattern *vpattern;

                vpattern = c_vector_entry(validator->pattern_properties, j);
                /* TODO */
#if 0
                if (REGEX_MATCH) {
                    mschema2 = vpattern->schema;
                    break;
                }
#endif
            }
        }

        if (mschema2) {
            if (json_schema_validate(mschema2, mvalue) == -1) {
                c_set_error("object member %zu does not match "
                            "'patternProperties' constraint: %s",
                            i, c_get_error());
                return -1;
            }
        }

        if (mschema1 || mschema2)
            continue;

        /* additionalProperties */
        if (validator->additional_properties) {
            if (json_schema_validate(validator->additional_properties,
                                     mvalue) == -1) {
                c_set_error("object member %zu does not match "
                            "'additionalProperties' constraint: %s",
                            i, c_get_error());
                return -1;
            }
        } else {
            c_set_error("object contains additional members");
            return -1;
        }
    }

    return 0;
}

/* ------------------------------------------------------------------------
 *  Parsing
 * ------------------------------------------------------------------------ */
struct json_schema *
json_schema_parse(const char *data, size_t sz) {
    struct json_schema *schema;
    struct json_value *json;
    uint32_t flags;

    flags = JSON_PARSE_REJECT_DUPLICATE_KEYS
          | JSON_PARSE_REJECT_NULL_CHARACTERS;

    json = json_parse(data, sz, flags);
    if (!json)
        return NULL;

    if (json->type != JSON_OBJECT) {
        c_set_error("schema is not a json object");
        json_value_delete(json);
        return NULL;
    }

    schema = json_schema_parse_object(json);
    if (!schema) {
        json_value_delete(json);
        return NULL;
    }

    json_value_delete(json);
    return schema;
}

static struct json_schema *
json_schema_parse_object(const struct json_value *json) {
    struct json_schema *schema;
    struct json_validator *validator;
    struct json_generic_validator *generic_validator;
    struct json_numeric_validator *numeric_validator;
    struct json_string_validator *string_validator;
    struct json_array_validator *array_validator;
    struct json_object_validator *object_validator;

    schema = json_schema_new();

    if (json->type != JSON_OBJECT) {
        c_set_error("schema is not a json object");
        goto error;
    }

    validator = &schema->validator;
    generic_validator = &validator->generic;
    numeric_validator = &validator->numeric;
    string_validator = &validator->string;
    array_validator = &validator->array;
    object_validator = &validator->object;

    object_validator->additional_properties = json_schema_new();

#define JSON_CHECK_TYPE(key_, value_, type_)                    \
    if ((value_)->type != type_) {                              \
        c_set_error("invalid type for member '%s'", key_);      \
        goto error;                                             \
    }

#define JSON_CHECK_TYPE2(key_, value_, type1_, type2_)          \
    if ((value_)->type != type1_ && (value_)->type != type2_) { \
        c_set_error("invalid type for member '%s'", key_);      \
        goto error;                                             \
    }

#define JSON_CHECK_STRICTLY_POSITIVE_INTEGER(key_, value_)      \
    do {                                                        \
        if ((value_)->type != JSON_INTEGER) {                   \
            c_set_error("invalid type for member '%s'", key_);  \
            goto error;                                         \
        }                                                       \
        if ((value_)->u.integer < 0) {                          \
            c_set_error("negative value");                      \
            goto error;                                         \
        }                                                       \
    } while (0)

    for (size_t i = 0; i < json_object_nb_members(json); i++) {
        struct c_ptr_vector *ptr_vector;
        struct c_vector *vector;
        struct c_hash_table *table;
        const char *key;
        struct json_value *value, *nvalue;

        key = json_object_nth_member(json, i, &value);

        /* Base */
        if (strcmp(key, "id") == 0) {
            JSON_CHECK_TYPE(key, value, JSON_STRING);
            schema->id = c_strndup(value->u.string.ptr, value->u.string.len);

        } else if (strcmp(key, "$schema") == 0) {
            JSON_CHECK_TYPE(key, value, JSON_STRING);

            if (json_schema_uri_parse(value->u.string.ptr,
                                      &schema->schema_uri) == -1) {
                c_set_error("unknown schema uri '%s'", value->u.string.ptr);
                goto invalid_member;
            }

        } else if (strcmp(key, "$ref") == 0) {
            JSON_CHECK_TYPE(key, value, JSON_STRING);
            /* TODO */

        } else if (strcmp(key, "title") == 0) {
            JSON_CHECK_TYPE(key, value, JSON_STRING);
            schema->title = c_strndup(value->u.string.ptr,
                                      value->u.string.len);

        } else if (strcmp(key, "description") == 0) {
            JSON_CHECK_TYPE(key, value, JSON_STRING);
            schema->description = c_strndup(value->u.string.ptr,
                                            value->u.string.len);

        } else if (strcmp(key, "default") == 0) {
            schema->default_value = json_value_clone(value);

        /* Generic */
        } else if (strcmp(key, "enum") == 0) {
            ptr_vector = json_schema_parse_validator_enum(value);
            if (!ptr_vector)
                goto invalid_member;

            generic_validator->enumeration = ptr_vector;

        } else if (strcmp(key, "type") == 0) {
            generic_validator->types = json_schema_parse_validator_type(value);
            if (!generic_validator->types)
                goto invalid_member;

        } else if (strcmp(key, "allOf") == 0) {
            generic_validator->all_of = json_schema_parse_schema_array(value);
            if (!generic_validator->all_of)
                goto invalid_member;
        } else if (strcmp(key, "anyOf") == 0) {
            generic_validator->any_of = json_schema_parse_schema_array(value);
            if (!generic_validator->any_of)
                goto invalid_member;
        } else if (strcmp(key, "oneOf") == 0) {
            generic_validator->one_of = json_schema_parse_schema_array(value);
            if (!generic_validator->one_of)
                goto invalid_member;
        } else if (strcmp(key, "not") == 0) {
            generic_validator->not = json_schema_parse_object(value);
            if (!generic_validator->not)
                goto invalid_member;

        } else if (strcmp(key, "format") == 0) {
            JSON_CHECK_TYPE(key, value, JSON_STRING);

            if (json_schema_format_parse(value->u.string.ptr,
                                         &generic_validator->format) == -1) {
                c_set_error("unknown format '%s'", value->u.string.ptr);
                goto invalid_member;
            }

        /* Numeric */
        } else if (strcmp(key, "multipleOf") == 0) {
            nvalue = json_schema_parse_validator_multiple_of(value);
            if (!nvalue)
                goto invalid_member;

            numeric_validator->multiple_of = nvalue;

        } else if (strcmp(key, "maximum") == 0) {
            JSON_CHECK_TYPE2(key, value, JSON_INTEGER, JSON_REAL);
            numeric_validator->max = json_value_clone(value);
        } else if (strcmp(key, "exclusiveMaximum") == 0) {
            JSON_CHECK_TYPE(key, value, JSON_BOOLEAN);
            numeric_validator->exclusive_max = value->u.boolean;
        } else if (strcmp(key, "minimum") == 0) {
            JSON_CHECK_TYPE2(key, value, JSON_INTEGER, JSON_REAL);
            numeric_validator->min = json_value_clone(value);
        } else if (strcmp(key, "exclusiveMinimum") == 0) {
            JSON_CHECK_TYPE(key, value, JSON_BOOLEAN);
            numeric_validator->exclusive_min = value->u.boolean;

        /* String */
        } else if (strcmp(key, "maxLength") == 0) {
            JSON_CHECK_STRICTLY_POSITIVE_INTEGER(key, value);
            string_validator->has_max_length = true;
            string_validator->max_length = (size_t)value->u.integer;
        } else if (strcmp(key, "minLength") == 0) {
            JSON_CHECK_STRICTLY_POSITIVE_INTEGER(key, value);
            string_validator->has_min_length = true;
            string_validator->min_length = (size_t)value->u.integer;

        } else if (strcmp(key, "pattern") == 0) {
            JSON_CHECK_TYPE(key, value, JSON_STRING);
            string_validator->pattern = c_strndup(value->u.string.ptr,
                                                  value->u.string.len);

        /* Array */
        } else if (strcmp(key, "additionalItems") == 0) {
            struct json_schema *schema2;

            if (json_schema_parse_validator_additional_items(value,
                                                             &schema2) == -1) {
                goto invalid_member;
            }

            array_validator->additional_items = schema2;

        } else if (strcmp(key, "items") == 0) {
            ptr_vector = json_schema_parse_validator_items(value);
            if (!ptr_vector)
                goto invalid_member;

            array_validator->items = ptr_vector;
            array_validator->items_is_array = (value->type == JSON_ARRAY);

        } else if (strcmp(key, "maxItems") == 0) {
            JSON_CHECK_STRICTLY_POSITIVE_INTEGER(key, value);
            array_validator->has_max_items = true;
            array_validator->max_items = (size_t)value->u.integer;
        } else if (strcmp(key, "minItems") == 0) {
            JSON_CHECK_STRICTLY_POSITIVE_INTEGER(key, value);
            array_validator->has_min_items = true;
            array_validator->min_items = (size_t)value->u.integer;
        } else if (strcmp(key, "uniqueItems") == 0) {
            JSON_CHECK_TYPE(key, value, JSON_BOOLEAN);
            array_validator->unique_items = value->u.boolean;

        /* Object */
        } else if (strcmp(key, "maxProperties") == 0) {
            JSON_CHECK_STRICTLY_POSITIVE_INTEGER(key, value);
            object_validator->has_max_properties = true;
            object_validator->max_properties = (size_t)value->u.integer;
        } else if (strcmp(key, "minProperties") == 0) {
            JSON_CHECK_STRICTLY_POSITIVE_INTEGER(key, value);
            object_validator->has_min_properties = true;
            object_validator->min_properties = (size_t)value->u.integer;

        } else if (strcmp(key, "required") == 0) {
            ptr_vector = json_schema_parse_validator_required(value);
            if (!ptr_vector)
                goto invalid_member;

            object_validator->required = ptr_vector;

        } else if (strcmp(key, "additionalProperties") == 0) {
            struct json_schema *schema2;

            if (json_schema_parse_validator_additional_properties(value,
                                                                  &schema2) == -1) {
                goto invalid_member;
            }

            json_schema_delete(object_validator->additional_properties);
            object_validator->additional_properties = schema2;

        } else if (strcmp(key, "properties") == 0) {
            vector = json_schema_parse_validator_properties(value);
            if (!vector)
                goto invalid_member;

            object_validator->properties = vector;

        } else if (strcmp(key, "patternProperties") == 0) {
            vector = json_schema_parse_validator_pattern_properties(value);
            if (!vector)
                goto invalid_member;

            object_validator->pattern_properties = vector;

        } else if (strcmp(key, "dependencies") == 0) {
            if (json_schema_parse_validator_dependencies(object_validator,
                                                         value) == -1) {
                goto invalid_member;
            }

        /* Definitions */
        } else if (strcmp(key, "definitions") == 0) {
            table = json_schema_parse_validator_definitions(value);
            if (!table)
                goto invalid_member;

            validator->definitions = table;

        /* Unknown */
        } else {
            /* The JSON schema specification indicates that implementations
             * SHOULD ignore unknown keywords, because schemas can be extended
             * by other implementations.
             *
             * This is a bad idea. Ignoring unknown keywords means:
             *
             * - Ignoring extended keywords, therefore not enforcing
             *   constraints which are supposed to be enforced.
             *
             * - Being unable to detect typos.
             */
            c_set_error("unknown key '%s'", key);
            goto error;
        }

        continue;

invalid_member:
        c_set_error("invalid member '%s': %s", key, c_get_error());
        goto error;
    }

#undef JSON_CHECK_TYPE
#undef JSON_CHECK_TYPE2
#undef JSON_CHECK_STRICTLY_POSITIVE_INTEGER

    return schema;

error:
    json_schema_delete(schema);
    return NULL;
}

static struct c_ptr_vector *
json_schema_parse_schema_array(const struct json_value *value) {
    struct c_ptr_vector *vector;
    struct json_schema *schema;

    if (value->type != JSON_ARRAY) {
        c_set_error("value is not an array");
        return NULL;
    }

    vector = c_ptr_vector_new();

    for (size_t i = 0; i < json_array_nb_elements(value); i++) {
        schema = json_schema_parse_object(json_array_element(value, i));
        if (!schema) {
            json_schema_vector_delete(vector);
            return NULL;
        }

        c_ptr_vector_append(vector, schema);
    }

    return vector;
}

/* ------------------------------------------------------------------------
 *  Misc
 * ------------------------------------------------------------------------ */
static void
json_value_vector_delete(struct c_ptr_vector *vector) {
    if (!vector)
        return;

    for (size_t i = 0; i < c_ptr_vector_length(vector); i++)
        json_value_delete(c_ptr_vector_entry(vector, i));

    c_ptr_vector_delete(vector);
}

static void
json_schema_vector_delete(struct c_ptr_vector *vector) {
    if (!vector)
        return;

    for (size_t i = 0; i < c_ptr_vector_length(vector); i++)
        json_schema_delete(c_ptr_vector_entry(vector, i));

    c_ptr_vector_delete(vector);
}

static void
json_schema_table_delete(struct c_hash_table *table) {
    struct c_hash_table_iterator *it;
    struct json_schema *schema;
    char *name;

    if (!table)
        return;

    it = c_hash_table_iterate(table);
    while (c_hash_table_iterator_next(it, (void **)&name,
                                      (void **)&schema) == 1) {
        c_free(name);
        json_schema_delete(schema);
    }
    c_hash_table_iterator_delete(it);

    c_hash_table_delete(table);
}

static void
json_string_vector_delete(struct c_ptr_vector *vector) {
    if (!vector)
        return;

    for (size_t i = 0; i < c_ptr_vector_length(vector); i++)
        c_free(c_ptr_vector_entry(vector, i));

    c_ptr_vector_delete(vector);
}

static void
json_string_vector_table_delete(struct c_hash_table *table) {
    struct c_hash_table_iterator *it;
    struct c_ptr_vector *vector;
    char *name;

    if (!table)
        return;

    it = c_hash_table_iterate(table);
    while (c_hash_table_iterator_next(it, (void **)&name,
                                      (void **)&vector) == 1) {
        c_free(name);
        json_string_vector_delete(vector);
    }
    c_hash_table_iterator_delete(it);

    c_hash_table_delete(table);
}

static void
json_object_validator_property_vector_delete(struct c_vector *vector) {
    if (!vector)
        return;

    for (size_t i = 0; i < c_vector_length(vector); i++) {
        struct json_object_validator_property *property;

        property = c_vector_entry(vector, i);

        c_free(property->string);
        json_schema_delete(property->schema);
    }

    c_vector_delete(vector);
}

static void
json_object_validator_pattern_vector_delete(struct c_vector *vector) {
    if (!vector)
        return;

    for (size_t i = 0; i < c_vector_length(vector); i++) {
        struct json_object_validator_pattern *pattern;

        pattern = c_vector_entry(vector, i);

        c_free(pattern->string);
        json_schema_delete(pattern->schema);
    }

    c_vector_delete(vector);
}

static bool
json_string_vector_contains(struct c_ptr_vector *vector, const char *string) {
    for (size_t i = 0; i < c_ptr_vector_length(vector); i++) {
        if (strcmp(string, c_ptr_vector_entry(vector, i)) == 0)
            return true;
    }

    return false;
}

static bool
json_value_vector_contains(struct c_ptr_vector *vector,
                           struct json_value *value) {
    for (size_t i = 0; i < c_ptr_vector_length(vector); i++) {
        if (json_value_equal(value, c_ptr_vector_entry(vector, i)))
            return true;
    }

    return false;
}
