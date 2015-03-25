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
static void *json_read_file(const char *, size_t *);
static void json_validate_file(const char *, const char *);

int
main(int argc, char **argv) {
    struct c_command_line *cmdline;
    const char *filename, *schema_path;

    cmdline = c_command_line_new();

    c_command_line_add_option(cmdline, "s", "schema",
                              "the schema to validate against", "file",
                              NULL);
    c_command_line_add_argument(cmdline, "the file to validate", "file");

    if (c_command_line_parse(cmdline, argc, argv) == -1)
        json_die("%s", c_get_error());

    schema_path = c_command_line_option_value(cmdline, "schema");
    filename = c_command_line_argument_value(cmdline, 0);

    json_validate_file(filename, schema_path);

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

static void
json_validate_file(const char *filename, const char *schema_path) {
    struct json_schema *schema;
    struct json_value *value;
    int fd;

    if (schema_path) {
        char *schema_string;
        size_t schema_size;

        schema_string = json_read_file(schema_path, &schema_size);
        if (!schema_string)
            json_die("cannot load schema: %s", c_get_error());

        schema = json_schema_parse(schema_string, schema_size);
        if (!schema)
            json_die("cannot parse schema: %s", c_get_error());

        c_free(schema_string);
    } else {
        schema = NULL;
    }

    if (strcmp(filename, "-") == 0) {
        filename = "stdin";
        fd = STDIN_FILENO;
    } else {
        fd = open(filename, O_RDONLY);
        if (fd == -1)
            json_die("cannot open %s: %s", filename, strerror(errno));
    }

    value = json_parse_fd(fd, JSON_PARSE_DEFAULT);
    if (!value)
        json_die("%s", c_get_error());

    if (schema) {
        if (json_schema_validate(schema, value) == -1)
            json_die("%s", c_get_error());

        json_schema_delete(schema);
    }

    json_value_delete(value);
    close(fd);
}

void *
json_read_file(const char *path, size_t *psize) {
    struct c_buffer *buf;
    void *data;
    int fd;

    buf = c_buffer_new();

    fd = open(path, O_RDONLY);
    if (fd == -1) {
        c_set_error("cannot open file: %s", strerror(errno));
        c_buffer_delete(buf);
        return NULL;
    }

    for (;;) {
        ssize_t ret;

        ret = c_buffer_read(buf, fd, BUFSIZ);
        if (ret == -1) {
            c_set_error("cannot read file: %s", c_get_error());
            c_buffer_delete(buf);
            close(fd);
            return NULL;
        } else if (ret == 0) {
            break;
        }
    }

    close(fd);

    data = c_buffer_extract_string(buf, psize);
    c_buffer_delete(buf);

    return data;
}
