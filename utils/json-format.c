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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include "json.h"

static void json_die(const char *, ...)
    __attribute__ ((format(printf, 1, 2), noreturn));

static struct json_value *json_load_file(const char *);
static void json_write_to_file(const struct json_value *, const char *,
                               uint32_t);

int
main(int argc, char **argv) {
    struct c_command_line *cmdline;
    const char *ifilename, *ofilename;
    struct json_value *value;
    uint32_t format_opts;

    cmdline = c_command_line_new();

    c_command_line_add_flag(cmdline, "c", "color", "colorize output");
    c_command_line_add_flag(cmdline, "i", "indent", "indent output");
    c_command_line_add_flag(cmdline, "s", "escape-solidus",
                            "escape solidus characters");

    c_command_line_add_option(cmdline, "o", "output",
                              "write the output to a file", "file", "-");

    c_command_line_add_argument(cmdline, "the file to format", "file");

    if (c_command_line_parse(cmdline, argc, argv) == -1)
        json_die("%s", c_get_error());

    format_opts = JSON_FORMAT_DEFAULT;

    if (c_command_line_is_option_set(cmdline, "color"))
        format_opts |= JSON_FORMAT_COLOR_ANSI;

    if (c_command_line_is_option_set(cmdline, "indent"))
        format_opts |= JSON_FORMAT_INDENT;

    if (c_command_line_is_option_set(cmdline, "escape-solidus"))
        format_opts |= JSON_FORMAT_ESCAPE_SOLIDUS;

    ifilename = c_command_line_argument_value(cmdline, 0);
    ofilename = c_command_line_option_value(cmdline, "output");

    value = json_load_file(ifilename);
    json_write_to_file(value, ofilename, format_opts);
    json_value_delete(value);

    c_command_line_delete(cmdline);
    return 0;
}

void
json_die(const char *fmt, ...) {
    va_list ap;

    fprintf(stderr, "fatal error: ");

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    putc('\n', stderr);
    exit(1);
}

static struct json_value *
json_load_file(const char *filename) {
    struct json_value *value;
    char *data;
    size_t len, sz;
    int fd;

    if (strcmp(filename, "-") == 0) {
        filename = "stdin";
        fd = STDIN_FILENO;
    } else {
        fd = open(filename, O_RDONLY);
        if (fd == -1)
            json_die("cannot open %s: %s", filename, strerror(errno));
    }

    data = NULL;
    len = 0;
    sz = 0;

    for (;;) {
        ssize_t ret;

        sz += BUFSIZ;
        if (sz == 0) {
            data = malloc(sz);
            if (!data) {
                json_die("cannot allocate %zu bytes: %s",
                         sz, strerror(errno));
            }
        } else {
            data = realloc(data, sz);
            if (!data) {
                json_die("cannot reallocate %zu bytes: %s",
                         sz, strerror(errno));
            }
        }

        ret = read(fd, data + len, BUFSIZ);
        if (ret == -1)
            json_die("cannot read %s: %s", filename, strerror(errno));
        if (ret == 0)
            break;

        len += (size_t)ret;
    }

    close(fd);

    value = json_parse(data, len, JSON_PARSE_DEFAULT);
    if (!value)
        json_die("%s", c_get_error());

    free(data);

    return value;
}

static void
json_write_to_file(const struct json_value *value, const char *filename,
                   uint32_t opts) {
    const char *ptr;
    size_t len;
    char *buf;
    int fd;

    buf = json_value_format(value, opts, &len);
    if (!buf)
        json_die("cannot format value: %s", c_get_error());

    if (strcmp(filename, "-") == 0) {
        filename = "stdout";
        fd = STDOUT_FILENO;
    } else {
        fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1)
            json_die("cannot open %s: %s", filename, strerror(errno));
    }

    ptr = buf;

    while (len > 0) {
        ssize_t ret;

        ret = write(fd, ptr, len);
        if (ret == -1)
            json_die("cannot write to %s: %s", filename, strerror(errno));

        ptr += (size_t)ret;
        len -= (size_t)ret;
    }

    if (write(fd, "\n", 1) == -1)
        json_die("cannot write to %s: %s", filename, strerror(errno));

    if (fd != STDOUT_FILENO) {
        if (fsync(fd) == -1)
            json_die("cannot sync %s: %s", filename, strerror(errno));
    }

    close(fd);

    c_free(buf);
}
