.PHONY:	all clean distclean install test working

CC=gcc
GIT=git

CFLAGS=-O2 -g -Wall -pedantic -pedantic-errors
CPPFLAGS=
LDFLAGS=

all:	trimnul

trimnul:	trimnul.o
	$(LINK.c) -o trimnul trimnul.o

clean:
	$(RM) foo bar baz\
          trimnul trimnul.o *~ \#*

distclean:	clean

install:	trimnul
	install trimnul ~/bin

test:	trimnul
	@set -e; \
	for f in foo bar baz; do \
		cp fixtures/$$f.test $$f; \
		./trimnul $$f > /dev/null; \
		cmp $$f fixtures/$$f.expected; \
	done; \
	echo "All tests passed."

working:
	$(GIT) tag -f working
	$(GIT) push --tags -f
