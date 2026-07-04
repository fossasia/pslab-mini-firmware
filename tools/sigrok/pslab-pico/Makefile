CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -O2

.PHONY: test clean

test: test_unpack
	./test_unpack

test_unpack: test_unpack.c protocol.c protocol.h
	$(CC) $(CFLAGS) -D_DEFAULT_SOURCE -o $@ test_unpack.c protocol.c

clean:
	rm -f test_unpack
