EMACS_ROOT ?= ../..
EMACS ?= emacs

CC = gcc
LD = gcc
CPPFLAGS = -I$(EMACS_ROOT)/src -Ideps
CFLAGS = -O2 -std=gnu99 -ggdb3 -Wall -fPIC $(CPPFLAGS)
LDFLAGS =

.PHONY : test clean format

all: quickjs-core.so

quickjs-core.so: quickjs-core.o deps/quickjs/libquickjs.a
	$(CC) -shared $(LDFLAGS) -o $@ $^

quickjs-core.o: quickjs-core.c
	$(CC) $(CFLAGS) -c -o $@ $<

deps/quickjs/libquickjs.a:
	(cd deps/quickjs && make libquickjs.a)

test:
	$(EMACS) -Q -batch -L . \
		-l test/test-quickjs.el \
		-f ert-run-tests-batch-and-exit

format:
	-clang-format -i quickjs-core.c

clean:
	-(cd deps/quickjs && make clean)
	-rm -f quickjs-core.so quickjs-core.o
