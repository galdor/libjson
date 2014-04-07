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
    __attribute__ ((noreturn));
static void json_usage(const char *, int);
static struct json_value *json_load_file(const char *);
static void json_write_to_file(const struct json_value *, const char *,
                               uint32_t);

int
main(int argc, char **argv) {
    const char *ifilename, *ofilename;
    struct json_value *value;
    int opt, nb_opts;
    uint32_t format_opts;

    format_opts = 0;

    opterr = 0;
    while ((opt = getopt(argc, argv, "chis")) != -1) {
        switch (opt) {
        case 'c':
            format_opts |= JSON_FORMAT_COLOR_ANSI;
            break;

        case 'h':
            json_usage(argv[0], 0);
            break;

        case 'i':
            format_opts |= JSON_FORMAT_INDENT;
            break;

        case 's':
            format_opts |= JSON_FORMAT_ESCAPE_SOLIDUS;
            break;

        case '?':
            json_usage(argv[0], 1);
        }
    }

    nb_opts = argc - optind;

    if (nb_opts == 0) {
        ifilename = "-";
        ofilename = "-";
    } else if (nb_opts == 1) {
        ifilename = argv[optind];
        ofilename = "-";
    } else {
        ifilename = argv[optind];
        ofilename = argv[optind + 1];
    }

    value = json_load_file(ifilename);
    json_write_to_file(value, ofilename, format_opts);
    json_value_delete(value);

    return 0;
}

static void
json_usage(const char *argv0, int exit_code) {
    printf("Usage: %s [-chis] <filename>\n"
            "\n"
            "Options:\n"
            "  -c colorize output using ansi escape sequences\n"
            "  -h display help\n"
            "  -i indent output\n"
            "  -s escape solidus characters\n",
            argv0);
    exit(exit_code);
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

    value = json_parse(data, len);
    if (!value)
        json_die("%s", json_get_error());

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
        json_die("cannot format value: %s", json_get_error());

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

    json_free(buf);
}
