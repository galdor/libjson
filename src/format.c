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

#include <alloca.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "json.h"
#include "internal.h"

#include <buffer.h>

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

static int json_format_value(const struct json_value *, struct bf_buffer *,
                             struct json_format_ctx *);

static int json_format_object(const struct json_object *, struct bf_buffer *,
                              struct json_format_ctx *);
static int json_format_array(const struct json_array *, struct bf_buffer *,
                             struct json_format_ctx *);
static int json_format_integer(int64_t, struct bf_buffer *,
                               struct json_format_ctx *);
static int json_format_real(double, struct bf_buffer *,
                            struct json_format_ctx *);
static int json_format_string(const char *, size_t, struct bf_buffer *,
                              struct json_format_ctx *);
static int json_format_boolean(bool, struct bf_buffer *,
                               struct json_format_ctx *);
static int json_format_null(struct bf_buffer *,
                            struct json_format_ctx *);

static int json_format_indent(struct bf_buffer *, struct json_format_ctx *);

static bool json_utf8_is_leading_byte(unsigned char);
static bool json_utf8_is_continuation_byte(unsigned char);
static size_t json_utf8_sequence_length(unsigned char);
static int json_utf8_decode_codepoint(const char *, uint32_t *, size_t *);

#define JSON_SET_ANSI_COLOR(ctx_, buf_, color_)                        \
    if (ctx_->opts & JSON_FORMAT_COLOR_ANSI) {                         \
        if (bf_buffer_add_printf(buf_, "\e[%dm", 30 + color_) == -1) { \
            json_set_error("%s", bf_get_error());                      \
            return 1;                                                  \
        }                                                              \
    }

#define JSON_CLEAR_ANSI_COLOR(ctx_, buf_)                \
    if (ctx_->opts & JSON_FORMAT_COLOR_ANSI) {           \
        if (bf_buffer_add_string(buf_, "\e[0m") == -1) { \
            json_set_error("%s", bf_get_error());        \
            return 1;                                    \
        }                                                \
    }

char *
json_value_format(const struct json_value *value, uint32_t opts, size_t *plen) {
    struct json_format_ctx ctx;
    struct bf_buffer *buf;
    char *data, *text;
    size_t length;

    buf = bf_buffer_new(0);
    if (!buf) {
        json_set_error("%s", bf_get_error());
        return NULL;
    }

    memset(&ctx, 0, sizeof(struct json_format_ctx));
    ctx.opts = opts;
    ctx.indent = 0;

    if (json_format_value(value, buf, &ctx) == -1)
        goto error;

    data = bf_buffer_extract(buf, &length);
    if (!data) {
        json_set_error("%s", bf_get_error());
        goto error;
    }

    text = json_malloc(length + 1);
    if (!text)
        goto error;

    memcpy(text, data, length);
    text[length] = '\0';

    bf_free(data);

    if (plen)
        *plen = length;

    bf_buffer_delete(buf);
    return text;

error:
    bf_buffer_delete(buf);
    return NULL;
}

static int
json_format_value(const struct json_value *value, struct bf_buffer *buf,
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

    json_set_error("unknown json value type %d", value->type);
    return -1;
}

static int
json_format_object(const struct json_object *object, struct bf_buffer *buf,
                   struct json_format_ctx *ctx) {
    if (bf_buffer_add_string(buf, "{") == -1) {
        json_set_error("%s", bf_get_error());
        return -1;
    }

    if (ctx->opts & JSON_FORMAT_INDENT) {
        if (bf_buffer_add_string(buf, "\n") == -1) {
            json_set_error("%s", bf_get_error());
            return -1;
        }

        ctx->indent += 2;
    }

    for (size_t i = 0; i < object->nb_members; i++) {
        struct json_object_member *member;

        member = object->members + i;

        if (i > 0) {
            if (bf_buffer_add_string(buf, ", ") == -1) {
                json_set_error("%s", bf_get_error());
                return -1;
            }

            if (ctx->opts & JSON_FORMAT_INDENT) {
                if (bf_buffer_add_string(buf, "\n") == -1) {
                    json_set_error("%s", bf_get_error());
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

        if (bf_buffer_add_string(buf, ": ") == -1) {
            json_set_error("%s", bf_get_error());
            return -1;
        }

        if (json_format_value(member->value, buf, ctx) == -1)
            return -1;
    }

    if (ctx->opts & JSON_FORMAT_INDENT) {
        if (bf_buffer_add_string(buf, "\n") == -1) {
            json_set_error("%s", bf_get_error());
            return -1;
        }

        ctx->indent -= 2;

        if (json_format_indent(buf, ctx) == -1)
            return -1;
    }

    if (bf_buffer_add_string(buf, "}") == -1) {
        json_set_error("%s", bf_get_error());
        return -1;
    }

    return 0;
}

static int
json_format_array(const struct json_array *array, struct bf_buffer *buf,
                  struct json_format_ctx *ctx) {
    if (bf_buffer_add_string(buf, "[") == -1) {
        json_set_error("%s", bf_get_error());
        return -1;
    }

    if (ctx->opts & JSON_FORMAT_INDENT) {
        if (bf_buffer_add_string(buf, "\n") == -1) {
            json_set_error("%s", bf_get_error());
            return -1;
        }

        ctx->indent += 2;
    }

    for (size_t i = 0; i < array->nb_elements; i++) {
        struct json_value *child;

        child = array->elements[i];

        if (i > 0) {
            if (bf_buffer_add_string(buf, ", ") == -1) {
                json_set_error("%s", bf_get_error());
                return -1;
            }

            if (ctx->opts & JSON_FORMAT_INDENT) {
                if (bf_buffer_add_string(buf, "\n") == -1) {
                    json_set_error("%s", bf_get_error());
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
        if (bf_buffer_add_string(buf, "\n") == -1) {
            json_set_error("%s", bf_get_error());
            return -1;
        }

        ctx->indent -= 2;

        if (json_format_indent(buf, ctx) == -1)
            return -1;
    }

    if (bf_buffer_add_string(buf, "]") == -1) {
        json_set_error("%s", bf_get_error());
        return -1;
    }

    return 0;
}

static int
json_format_integer(int64_t integer, struct bf_buffer *buf,
                    struct json_format_ctx *ctx) {
    JSON_SET_ANSI_COLOR(ctx, buf, JSON_ANSI_COLOR_RED);

    if (bf_buffer_add_printf(buf, "%"PRIi64, integer) == -1) {
        json_set_error("%s", bf_get_error());
        JSON_CLEAR_ANSI_COLOR(ctx, buf);
        return -1;
    }

    JSON_CLEAR_ANSI_COLOR(ctx, buf);
    return 0;
}

static int
json_format_real(double real, struct bf_buffer *buf,
                 struct json_format_ctx *ctx) {
    JSON_SET_ANSI_COLOR(ctx, buf, JSON_ANSI_COLOR_RED);

    if (bf_buffer_add_printf(buf, "%.17g", real) == -1) {
        json_set_error("%s", bf_get_error());
        JSON_CLEAR_ANSI_COLOR(ctx, buf);
        return -1;
    }

    JSON_CLEAR_ANSI_COLOR(ctx, buf);
    return 0;
}

static int
json_format_string(const char *string, size_t length, struct bf_buffer *buf,
                   struct json_format_ctx *ctx) {
    const char *ptr;
    size_t len;

    if (bf_buffer_add_string(buf, "\"") == -1) {
        json_set_error("%s", bf_get_error());
        return -1;
    }

    JSON_SET_ANSI_COLOR(ctx, buf, JSON_ANSI_COLOR_YELLOW);

    ptr = string;
    len = length;

    while (len > 0) {
        int ret;

        if (*ptr == '"') {
            ret = bf_buffer_add_string(buf, "\\\"");
        } else if (*ptr == '\\') {
            ret = bf_buffer_add_string(buf, "\\\\");
        } else if (*ptr == '/' && (ctx->opts & JSON_FORMAT_ESCAPE_SOLIDUS)) {
            ret = bf_buffer_add_string(buf, "\\/");
        } else if (*ptr == '\b') {
            ret = bf_buffer_add_string(buf, "\\b");
        } else if (*ptr == '\f') {
            ret = bf_buffer_add_string(buf, "\\f");
        } else if (*ptr == '\n') {
            ret = bf_buffer_add_string(buf, "\\n");
        } else if (*ptr == '\r') {
            ret = bf_buffer_add_string(buf, "\\r");
        } else if (*ptr == '\t') {
            ret = bf_buffer_add_string(buf, "\\t");
        } else if (isprint((unsigned char)*ptr)) {
            ret = bf_buffer_add(buf, ptr, 1);
        } else if (json_utf8_is_leading_byte((unsigned char)*ptr)) {
            char tmp[13]; /* \uxxxx\uxxxx */
            uint32_t codepoint;
            size_t sequence_length;

            if (json_utf8_decode_codepoint(ptr, &codepoint,
                                           &sequence_length) == -1) {
                goto error;
            }

            if (codepoint <= 0xffff) {
                /* \uxxxx */
                snprintf(tmp, sizeof(tmp), "\\u%04x", codepoint);
                ret = bf_buffer_add(buf, tmp, 6);
            } else {
                uint32_t hi, lo;

                /* UTF-16 surrogate pair */
                codepoint -= 0x010000;
                hi = (codepoint >> 10) + 0xd800;
                lo = (codepoint & 0x3ff) + 0xdc00;

                /* \uxxxx\uxxxx */
                snprintf(tmp, sizeof(tmp), "\\u%04x\\u%04x", hi, lo);
                ret = bf_buffer_add(buf, tmp, 12);
            }

            ptr += sequence_length;
            len -= sequence_length;
            continue;
        } else {
            json_set_error("invalid byte \\%hhu in utf8 string",
                           (unsigned char)*ptr);
            goto error;
        }

        if (ret == -1) {
            json_set_error("%s", bf_get_error());
            goto error;
        }

        ptr++;
        len--;
    }

    JSON_CLEAR_ANSI_COLOR(ctx, buf);

    if (bf_buffer_add_string(buf, "\"") == -1) {
        json_set_error("%s", bf_get_error());
        return -1;
    }

    return 0;

error:
    JSON_CLEAR_ANSI_COLOR(ctx, buf);
    return -1;
}

static int
json_format_boolean(bool boolean, struct bf_buffer *buf,
                    struct json_format_ctx *ctx) {
    JSON_SET_ANSI_COLOR(ctx, buf, JSON_ANSI_COLOR_GREEN);

    if (bf_buffer_add_string(buf, boolean ? "true" : "false") == -1) {
        json_set_error("%s", bf_get_error());
        JSON_CLEAR_ANSI_COLOR(ctx, buf);
        return -1;
    }

    JSON_CLEAR_ANSI_COLOR(ctx, buf);
    return 0;
}

static int
json_format_null(struct bf_buffer *buf, struct json_format_ctx *ctx) {
    JSON_SET_ANSI_COLOR(ctx, buf, JSON_ANSI_COLOR_GREEN);

    if (bf_buffer_add_string(buf, "null") == -1) {
        json_set_error("%s", bf_get_error());
        JSON_CLEAR_ANSI_COLOR(ctx, buf);
        return -1;
    }

    JSON_CLEAR_ANSI_COLOR(ctx, buf);
    return 0;
}

static bool
json_utf8_is_leading_byte(unsigned char c) {
    return ((c & 0x80) == 0x00) /* one byte character */
        || ((c & 0xc0) == 0xc0);
}

static bool
json_utf8_is_continuation_byte(unsigned char c) {
    return (c & 0xc0) == 0x80;
}

static size_t
json_utf8_sequence_length(unsigned char c) {
    if ((c & 0x80) == 0x0) {
        return 1;
    } else if ((c & 0xe0) == 0xc0) {
        return 2;
    } else if ((c & 0xf0) == 0xe0) {
        return 3;
    } else if ((c & 0xf8) == 0xf0) {
        return 4;
    } else if ((c & 0xfc) == 0xf8) {
        return 5;
    } else if ((c & 0xfe) == 0xfc) {
        return 6;
    } else {
        return 0;
    }
}

static int
json_utf8_decode_codepoint(const char *ptr, uint32_t *pcodepoint,
                           size_t *p_sequence_length) {
    uint32_t codepoint;
    size_t length;

    length = json_utf8_sequence_length((unsigned char)*ptr);
    if (length == 0) {
        json_set_error("invalid leading byte \\%hhu in utf8 string",
                       (unsigned char)*ptr);
        return -1;
    }

    for (size_t i = 0; i < length; i++) {
        if (ptr[i] == '\0') {
            json_set_error("truncated sequence in utf8 string");
            return -1;
        }
    }

    if (length == 1) {
        /* 0xxxxxxx */
        codepoint = (uint32_t)((ptr[0] & 0x7f));
    } else if (length == 2) {
        /* 110xxxxx 10xxxxxx */
        codepoint = (uint32_t)(((ptr[0] & 0x1f) << 6) | (ptr[1] & 0x3f));
    } else if (length == 3) {
        /* 1110xxxx 10xxxxxx 10xxxxxx */
        codepoint = (uint32_t)(((ptr[0] & 0x0f) << 12) | ((ptr[1] & 0x3f) << 6)
                             |  (ptr[2] & 0x3f));
    } else if (length == 4) {
        /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
        codepoint = (uint32_t)(((ptr[0] & 0x0d) << 18) | ((ptr[1] & 0x3f) << 12)
                             | ((ptr[2] & 0x3f) <<  6) |  (ptr[3] & 0x3f));
    } else if (length == 5) {
        /* 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
        codepoint = (uint32_t)(((ptr[0] & 0x03) << 24) | ((ptr[1] & 0x3f) << 18)
                             | ((ptr[2] & 0x3f) << 12) | ((ptr[3] & 0x3f) <<  6)
                             |  (ptr[4] & 0x3f));
    } else if (length == 6) {
        /* 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
        codepoint = (uint32_t)(((ptr[0] & 0x01) << 30) | ((ptr[1] & 0x3f) << 24)
                             | ((ptr[2] & 0x3f) << 18) | ((ptr[3] & 0x3f) << 12)
                             | ((ptr[4] & 0x3f) <<  6) |  (ptr[5] & 0x3f));
    } else {
        json_set_error("invalid sequence in utf8 string");
        return -1;
    }

    *pcodepoint = codepoint;
    *p_sequence_length = length;
    return 0;
}

static int
json_format_indent(struct bf_buffer *buf, struct json_format_ctx *ctx) {
    char *tmp;

    if (ctx->indent == 0) {
        return 0;
    } else if (ctx->indent > 0xffff) {
        json_set_error("cannot indent text: nesting depth too high");
        return -1;
    }

    tmp = alloca(ctx->indent);
    memset(tmp, ' ', ctx->indent);

    if (bf_buffer_add(buf, tmp, ctx->indent) == -1) {
        json_set_error("%s", bf_get_error());
        return -1;
    }

    return 0;
}
