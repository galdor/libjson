# Common
prefix= /usr/local
libdir= $(prefix)/lib
incdir= $(prefix)/include
bindir= $(prefix)/bin

CC=   clang

CFLAGS+= -std=c99
CFLAGS+= -Wall -Wextra -Werror -Wsign-conversion
CFLAGS+= -Wno-unused-parameter -Wno-unused-function

LDFLAGS=

PANDOC_OPTS= -s --toc --email-obfuscation=none

# Platform specific
platform= $(shell uname -s)

ifeq ($(platform), Linux)
	CFLAGS+= -DJSON_PLATFORM_LINUX
	CFLAGS+= -D_POSIX_C_SOURCE=200809L -D_BSD_SOURCE
endif

# Debug
debug=0
ifeq ($(debug), 1)
	CFLAGS+= -g -ggdb
else
	CFLAGS+= -O2
endif

# Coverage
coverage?= 0
ifeq ($(coverage), 1)
	CC= gcc
	CFLAGS+= -fprofile-arcs -ftest-coverage
	LDFLAGS+= --coverage
endif

# Target: libjson
libjson_LIB= libjson.a
libjson_SRC= $(wildcard src/*.c)
libjson_INC= $(wildcard src/*.h)
libjson_PUBINC= src/json.h
libjson_OBJ= $(subst .c,.o,$(libjson_SRC))

$(libjson_LIB): CFLAGS+=

# Target: tests
tests_SRC= $(wildcard tests/*.c)
tests_OBJ= $(subst .c,.o,$(tests_SRC))
tests_BIN= $(subst .o,,$(tests_OBJ))

$(tests_BIN): CFLAGS+= -Isrc
$(tests_BIN): LDFLAGS+= -L.
$(tests_BIN): LDLIBS+= -ljson

# Target: utils
utils_SRC= $(wildcard utils/*.c)
utils_OBJ= $(subst .c,.o,$(utils_SRC))
utils_BIN= $(subst .o,,$(utils_OBJ))

$(utils_BIN): CFLAGS+= -Isrc
$(utils_BIN): LDFLAGS+= -L.
$(utils_BIN): LDLIBS+= -ljson

# Target: doc
doc_SRC= $(wildcard doc/*.mkd)
doc_HTML= $(subst .mkd,.html,$(doc_SRC))

# Rules
all: lib tests utils doc

lib: $(libjson_LIB)

tests: lib $(tests_BIN)

utils: lib $(utils_BIN)

doc: $(doc_HTML)

$(libjson_LIB): $(libjson_OBJ)
	$(AR) cr $@ $(libjson_OBJ)

$(tests_OBJ): $(libjson_LIB) $(libjson_INC)
tests/%: tests/%.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(utils_OBJ): $(libjson_LIB) $(libjson_INC)
utils/%: utils/%.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

doc/%.html: doc/*.mkd
	pandoc $(PANDOC_OPTS) -t html5 -o $@ $<

clean:
	$(RM) $(libjson_LIB) $(wildcard src/*.o)
	$(RM) $(tests_BIN) $(wildcard tests/*.o)
	$(RM) $(utils_BIN) $(wildcard utils/*.o)
	$(RM) $(wildcard **/*.gc??)
	$(RM) -r coverage
	$(RM) -r $(doc_HTML)

coverage:
	lcov -o /tmp/libjson.info -c -d . -b .
	genhtml -o coverage -t libjson /tmp/libjson.info
	rm /tmp/libjson.info

install: lib
	mkdir -p $(libdir) $(incdir) $(bindir)
	install -m 644 $(libjson_LIB) $(libdir)
	install -m 644 $(libjson_PUBINC) $(incdir)
	install -m 755 $(utils_BIN) $(bindir)

uninstall:
	$(RM) $(addprefix $(libdir)/,$(libjson_LIB))
	$(RM) $(addprefix $(incdir)/,$(libjson_PUBINC))
	$(RM) $(addprefix $(bindir)/,$(utils_BIN))

tags:
	ctags -o .tags -a $(wildcard src/*.[hc])

.PHONY: all lib tests doc clean coverage install uninstall tags
