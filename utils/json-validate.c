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
static void json_usage(const char *, int)
    __attribute__ ((noreturn));
static void json_validate_file(const char *);

int
main(int argc, char **argv) {
    const char *filename;
    int opt, nb_opts;

    opterr = 0;
    while ((opt = getopt(argc, argv, "h")) != -1) {
        switch (opt) {
        case 'h':
            json_usage(argv[0], 0);
            break;

        case '?':
            json_usage(argv[0], 1);
        }
    }

    nb_opts = argc - optind;
    if (nb_opts >= 1) {
        filename = argv[optind];
    } else {
        filename = "-";
    }

    json_validate_file(filename);
    return 0;
}

static void
json_usage(const char *argv0, int exit_code) {
    printf("Usage: %s [-h] <filename>\n"
            "\n"
            "Options:\n"
            "  -h display help\n",
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

static void
json_validate_file(const char *filename) {
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
        json_die("%s", json_get_error());

    json_value_delete(value);
    free(data);
}
