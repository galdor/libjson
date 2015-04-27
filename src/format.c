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

#include "internal.h"

enum json_ansi_color {
    JSON_ANSI_COLOR_BLACK   = 0,
    JSON_ANSI_COLOR_RED     = 1,
    JSON_ANSI_COLOR_GREEN   = 2,
    JSON_ANSI_COLOR_YELLOW  = 3,
    JSON_ANSI_COLOR_BLUE    = 4,
    JSON_ANSI_COLOR_MAGENTA = 5,
    JSON_ANSI_COLOR_CYAN    = 6,
    JSON_ANSI_COLOR_WHITE   = 7,
    JSON_ANSI_COLOR_GRAY    = 8,
    JSON_ANSI_COLOR_DEFAULT = 9,
};

struct json_format_ctx {
    uint32_t opts;
    size_t indent;
};

static int json_format_value(const struct json_value *, struct c_buffer *,
                             struct json_format_ctx *);

static int json_format_object(const struct json_object *, struct c_buffer *,
                              struct json_format_ctx *);
static int json_format_array(const struct json_array *, struct c_buffer *,
                             struct json_format_ctx *);
static int json_format_integer(int64_t, struct c_buffer *,
                               struct json_format_ctx *);
static int json_format_real(double, struct c_buffer *,
                            struct json_format_ctx *);
static int json_format_string(const char *, size_t, struct c_buffer *,
                              struct json_format_ctx *);
static int json_format_boolean(bool, struct c_buffer *,
                               struct json_format_ctx *);
static int json_format_null(struct c_buffer *,
                            struct json_format_ctx *);

static int json_format_indent(struct c_buffer *, struct json_format_ctx *);

#define JSON_SET_ANSI_COLOR(ctx_, buf_, color_)                        \
    if (ctx_->opts & JSON_FORMAT_COLOR_ANSI) {                         \
        if (c_buffer_add_printf(buf_, "\e[%dm", 30 + color_) == -1) {  \
            c_set_error("%s", c_get_error());                          \
            return 1;                                                  \
        }                                                              \
    }

#define JSON_CLEAR_ANSI_COLOR(ctx_, buf_)                \
    if (ctx_->opts & JSON_FORMAT_COLOR_ANSI) {           \
        if (c_buffer_add_string(buf_, "\e[0m") == -1) {  \
            c_set_error("%s", c_get_error());            \
            return 1;                                    \
        }                                                \
    }

char *
json_value_format(struct json_value *value, uint32_t opts, size_t *plen) {
    struct json_format_ctx ctx;
    struct c_buffer *buf;
    char *data;
    size_t length;

    json_value_sort_objects_by_index(value);

    buf = c_buffer_new();
    if (!buf) {
        c_set_error("%s", c_get_error());
        return NULL;
    }

    memset(&ctx, 0, sizeof(struct json_format_ctx));
    ctx.opts = opts;
    ctx.indent = 0;

    if (json_format_value(value, buf, &ctx) == -1)
        goto error;

    data = c_buffer_extract_string(buf, &length);
    if (!data) {
        c_set_error("%s", c_get_error());
        goto error;
    }

    if (plen)
        *plen = length;

    c_buffer_delete(buf);
    return data;

error:
    c_buffer_delete(buf);
    return NULL;
}

static int
json_format_value(const struct json_value *value, struct c_buffer *buf,
                  struct json_format_ctx *ctx) {
    switch (value->type) {
    case JSON_OBJECT:
        return json_format_object(&value->u.object, buf, ctx);

    case JSON_ARRAY:
        return json_format_array(&value->u.array, buf, ctx);

    case JSON_INTEGER:
        return json_format_integer(value->u.integer, buf, ctx);

    case JSON_REAL:
        return json_format_real(value->u.real, buf, ctx);

    case JSON_STRING:
        return json_format_string(value->u.string.ptr, value->u.string.len,
                                  buf, ctx);

    case JSON_BOOLEAN:
        return json_format_boolean(value->u.boolean, buf, ctx);

    case JSON_NULL:
        return json_format_null(buf, ctx);
    }

    c_set_error("unknown json value type %d", value->type);
    return -1;
}

static int
json_format_object(const struct json_object *object, struct c_buffer *buf,
                   struct json_format_ctx *ctx) {
    if (c_buffer_add_string(buf, "{") == -1) {
        c_set_error("%s", c_get_error());
        return -1;
    }

    if (ctx->opts & JSON_FORMAT_INDENT) {
        if (c_buffer_add_string(buf, "\n") == -1) {
            c_set_error("%s", c_get_error());
            return -1;
        }

        ctx->indent += 2;
    }

    for (size_t i = 0; i < object->nb_members; i++) {
        struct json_object_member *member;

        member = object->members + i;

        if (i > 0) {
            if (c_buffer_add_string(buf, ", ") == -1) {
                c_set_error("%s", c_get_error());
                return -1;
            }

            if (ctx->opts & JSON_FORMAT_INDENT) {
                if (c_buffer_add_string(buf, "\n") == -1) {
                    c_set_error("%s", c_get_error());
                    return -1;
                }
            }
        }

        if (ctx->opts & JSON_FORMAT_INDENT) {
            if (json_format_indent(buf, ctx) == -1)
                return -1;
        }

        if (json_format_value(member->key, buf, ctx) == -1)
            return -1;

        if (c_buffer_add_string(buf, ": ") == -1) {
            c_set_error("%s", c_get_error());
            return -1;
        }

        if (json_format_value(member->value, buf, ctx) == -1)
            return -1;
    }

    if (ctx->opts & JSON_FORMAT_INDENT) {
        if (c_buffer_add_string(buf, "\n") == -1) {
            c_set_error("%s", c_get_error());
            return -1;
        }

        ctx->indent -= 2;

        if (json_format_indent(buf, ctx) == -1)
            return -1;
    }

    if (c_buffer_add_string(buf, "}") == -1) {
        c_set_error("%s", c_get_error());
        return -1;
    }

    return 0;
}

static int
json_format_array(const struct json_array *array, struct c_buffer *buf,
                  struct json_format_ctx *ctx) {
    if (c_buffer_add_string(buf, "[") == -1) {
        c_set_error("%s", c_get_error());
        return -1;
    }

    if (ctx->opts & JSON_FORMAT_INDENT) {
        if (c_buffer_add_string(buf, "\n") == -1) {
            c_set_error("%s", c_get_error());
            return -1;
        }

        ctx->indent += 2;
    }

    for (size_t i = 0; i < array->nb_elements; i++) {
        struct json_value *child;

        child = array->elements[i];

        if (i > 0) {
            if (c_buffer_add_string(buf, ", ") == -1) {
                c_set_error("%s", c_get_error());
                return -1;
            }

            if (ctx->opts & JSON_FORMAT_INDENT) {
                if (c_buffer_add_string(buf, "\n") == -1) {
                    c_set_error("%s", c_get_error());
                    return -1;
                }
            }
        }

        if (ctx->opts & JSON_FORMAT_INDENT) {
            if (json_format_indent(buf, ctx) == -1)
                return -1;
        }

        if (json_format_value(child, buf, ctx) == -1)
            return -1;
    }

    if (ctx->opts & JSON_FORMAT_INDENT) {
        if (c_buffer_add_string(buf, "\n") == -1) {
            c_set_error("%s", c_get_error());
            return -1;
        }

        ctx->indent -= 2;

        if (json_format_indent(buf, ctx) == -1)
            return -1;
    }

    if (c_buffer_add_string(buf, "]") == -1) {
        c_set_error("%s", c_get_error());
        return -1;
    }

    return 0;
}

static int
json_format_integer(int64_t integer, struct c_buffer *buf,
                    struct json_format_ctx *ctx) {
    JSON_SET_ANSI_COLOR(ctx, buf, JSON_ANSI_COLOR_RED);

    if (c_buffer_add_printf(buf, "%"PRIi64, integer) == -1) {
        c_set_error("%s", c_get_error());
        JSON_CLEAR_ANSI_COLOR(ctx, buf);
        return -1;
    }

    JSON_CLEAR_ANSI_COLOR(ctx, buf);
    return 0;
}

static int
json_format_real(double real, struct c_buffer *buf,
                 struct json_format_ctx *ctx) {
    JSON_SET_ANSI_COLOR(ctx, buf, JSON_ANSI_COLOR_RED);

    if (c_buffer_add_printf(buf, "%.17g", real) == -1) {
        c_set_error("%s", c_get_error());
        JSON_CLEAR_ANSI_COLOR(ctx, buf);
        return -1;
    }

    JSON_CLEAR_ANSI_COLOR(ctx, buf);
    return 0;
}

static int
json_format_string(const char *string, size_t length, struct c_buffer *buf,
                   struct json_format_ctx *ctx) {
    const char *ptr;
    size_t len;

    if (c_buffer_add_string(buf, "\"") == -1) {
        c_set_error("%s", c_get_error());
        return -1;
    }

    JSON_SET_ANSI_COLOR(ctx, buf, JSON_ANSI_COLOR_YELLOW);

    ptr = string;
    len = length;

    while (len > 0) {
        int ret;

        if (*ptr == '"') {
            ret = c_buffer_add_string(buf, "\\\"");
        } else if (*ptr == '\\') {
            ret = c_buffer_add_string(buf, "\\\\");
        } else if (*ptr == '/' && (ctx->opts & JSON_FORMAT_ESCAPE_SOLIDUS)) {
            ret = c_buffer_add_string(buf, "\\/");
        } else if (*ptr == '\b') {
            ret = c_buffer_add_string(buf, "\\b");
        } else if (*ptr == '\f') {
            ret = c_buffer_add_string(buf, "\\f");
        } else if (*ptr == '\n') {
            ret = c_buffer_add_string(buf, "\\n");
        } else if (*ptr == '\r') {
            ret = c_buffer_add_string(buf, "\\r");
        } else if (*ptr == '\t') {
            ret = c_buffer_add_string(buf, "\\t");
        } else if (isprint((unsigned char)*ptr)) {
            ret = c_buffer_add(buf, ptr, 1);
        } else {
            uint32_t codepoint;
            size_t sequence_length;

            if (c_utf8_read_codepoint(ptr, &codepoint,
                                      &sequence_length) == -1) {
                goto error;
            }

            if (codepoint <= 0xffff) {
                /* \uxxxx */
                ret = c_buffer_add_printf(buf, "\\u%04x", codepoint);
            } else {
                uint32_t hi, lo;

                /* UTF-16 surrogate pair */
                codepoint -= 0x010000;
                hi = (codepoint >> 10) + 0xd800;
                lo = (codepoint & 0x3ff) + 0xdc00;

                /* \uxxxx\uxxxx */
                ret = c_buffer_add_printf(buf, "\\u%04x\\u%04x", hi, lo);
            }

            ptr += sequence_length;
            len -= sequence_length;
            continue;
        }

        if (ret == -1) {
            c_set_error("%s", c_get_error());
            goto error;
        }

        ptr++;
        len--;
    }

    JSON_CLEAR_ANSI_COLOR(ctx, buf);

    if (c_buffer_add_string(buf, "\"") == -1) {
        c_set_error("%s", c_get_error());
        return -1;
    }

    return 0;

error:
    JSON_CLEAR_ANSI_COLOR(ctx, buf);
    return -1;
}

static int
json_format_boolean(bool boolean, struct c_buffer *buf,
                    struct json_format_ctx *ctx) {
    JSON_SET_ANSI_COLOR(ctx, buf, JSON_ANSI_COLOR_GREEN);

    if (c_buffer_add_string(buf, boolean ? "true" : "false") == -1) {
        c_set_error("%s", c_get_error());
        JSON_CLEAR_ANSI_COLOR(ctx, buf);
        return -1;
    }

    JSON_CLEAR_ANSI_COLOR(ctx, buf);
    return 0;
}

static int
json_format_null(struct c_buffer *buf, struct json_format_ctx *ctx) {
    JSON_SET_ANSI_COLOR(ctx, buf, JSON_ANSI_COLOR_GREEN);

    if (c_buffer_add_string(buf, "null") == -1) {
        c_set_error("%s", c_get_error());
        JSON_CLEAR_ANSI_COLOR(ctx, buf);
        return -1;
    }

    JSON_CLEAR_ANSI_COLOR(ctx, buf);
    return 0;
}

static int
json_format_indent(struct c_buffer *buf, struct json_format_ctx *ctx) {
    char *tmp;

    if (ctx->indent == 0) {
        return 0;
    } else if (ctx->indent > 0xffff) {
        c_set_error("cannot indent text: nesting depth too high");
        return -1;
    }

    tmp = alloca(ctx->indent);
    memset(tmp, ' ', ctx->indent);

    if (c_buffer_add(buf, tmp, ctx->indent) == -1) {
        c_set_error("%s", c_get_error());
        return -1;
    }

    return 0;
}
