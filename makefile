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
	cp foo.test foo
	cp bar.test bar
	cp baz.test baz
	./trimnul foo bar baz

working:
	$(GIT) tag -f 'src/c/trimnul#working'
	$(GIT) push --tags -f
