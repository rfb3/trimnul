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
	cp fixtures/foo.test foo
	cp fixtures/bar.test bar
	cp fixtures/baz.test baz
	./trimnul foo bar baz
	cmp foo fixtures/foo.expected
	cmp bar fixtures/bar.expected
	cmp baz fixtures/baz.expected

working:
	$(GIT) tag -f working
	$(GIT) push --tags -f
