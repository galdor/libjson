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

void
json_set_error_invalid_character(char c, const char *fmt, ...) {
    char buf[C_ERROR_BUFSZ];
    va_list ap;
    char *ptr;
    size_t len, prefix_len;
    int ret;

    if (isprint(c)) {
        ret = snprintf(buf, C_ERROR_BUFSZ, "invalid character '%c'", c);
    } else {
        ret = snprintf(buf, C_ERROR_BUFSZ, "invalid character \\%hhu", c);
    }

    prefix_len = (size_t)ret;
    if (prefix_len >= C_ERROR_BUFSZ)
        return;

    ptr = buf + prefix_len;
    len = C_ERROR_BUFSZ - prefix_len;

    va_start(ap, fmt);
    ret = vsnprintf(ptr, len, fmt, ap);
    va_end(ap);

    c_set_error("%s", buf);
}
