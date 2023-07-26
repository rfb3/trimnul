.PHONY:	all clean distclean install test

CFLAGS=-O2 -g -Wall -pedantic -pedantic-errors
CPPFLAGS=
LDFLAGS=

all:	eliminate_terminal_nulls

eliminate_terminal_nulls:	eliminate_terminal_nulls.o
	$(LINK.c) -o eliminate_terminal_nulls eliminate_terminal_nulls.o

clean:
	$(RM) eliminate_terminal_nulls eliminate_terminal_nulls.o *~ \#*

install:	eliminate_terminal_nulls
	install eliminate_terminal_nulls ~/bin

test:	eliminate_terminal_nulls
	cp foo.test foo
	cp bar.test bar
	cp baz.test baz
	./eliminate_terminal_nulls foo bar baz
