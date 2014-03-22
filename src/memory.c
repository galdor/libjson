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
#include "internal.h"

#define JSON_DEFAULT_ALLOCATOR \
    {                        \
        .malloc = malloc,    \
        .free = free,        \
        .calloc = calloc,    \
        .realloc = realloc   \
    }

static const struct json_memory_allocator json_default_allocator =
    JSON_DEFAULT_ALLOCATOR;

static struct json_memory_allocator json_allocator = JSON_DEFAULT_ALLOCATOR;

const struct json_memory_allocator *
json_default_memory_allocator = &json_default_allocator;

void
json_set_memory_allocator(const struct json_memory_allocator *allocator) {
    if (allocator) {
        json_allocator = *allocator;
    } else {
        json_allocator = json_default_allocator;
    }
}

void *
json_malloc(size_t sz) {
    return json_allocator.malloc(sz);
}

void
json_free(void *ptr) {
    json_allocator.free(ptr);
}

void *
json_calloc(size_t nb, size_t sz) {
    return json_allocator.calloc(nb, sz);
}

void *
json_realloc(void *ptr, size_t sz) {
    return json_allocator.realloc(ptr, sz);
}

