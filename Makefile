AR ?= ar
CC ?= gcc
CFLAGS = -Ideps -pedantic -std=c99 -v -Wall -Wextra -g

ifeq ($(APP_DEBUG),true)
	CFLAGS += -g -O0
else
	CFLAGS += -O2
endif

PREFIX ?= /usr/local

SRCS += $(wildcard src/*.c)

OBJS += $(SRCS:.c=.o)

all: build

%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@

build: build/lib/libjsw_rbtree.a
	mkdir -p build/include/jsw_rbtree
	cp -f src/jsw_rbtree.h build/include/jsw_rbtree/jsw_rbtree.h

build/lib/libjsw_rbtree.a: $(OBJS)
	mkdir -p build/lib
	$(AR) -crs $@ $^

clean:
	rm -fr *.o build test-centroid test-tdigest *.dSYM src/*.o

test-centroid: build
	$(CC) $(CFLAGS) -Ibuild/include -o test-centroid test-centroid.c -Lbuild/lib -ljsw_rbtree

test-tdigest: build
	$(CC) $(CFLAGS) -Ibuild/include -o test-tdigest test-tdigest.c -Lbuild/lib -ljsw_rbtree

test-leaks-tdigest: test-tdigest
	# to install valgrind on Macos: brew install --HEAD valgrind
	valgrind --leak-check=yes ./test-tdigest

tests: test-centroid test-tdigest

install: all
	mkdir -p $(PREFIX)/include/jsw_rbtree
	mkdir -p $(PREFIX)/lib
	cp -f src/jsw_rbtree.h $(PREFIX)/include/jsw_rbtree/jsw_rbtree.h
	cp -f build/lib/libjsw_rbtree.a $(PREFIX)/lib/libjsw_rbtree.a

uninstall:
	rm -fr $(PREFIX)/include/jsw_rbtree/jsw_rbtree.h
	rm -fr $(PREFIX)/lib/libjsw_rbtree.a

.PHONY: build clean example install uninstall
