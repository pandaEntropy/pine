PREFIX ?= /usr/local
CONFDIR ?= /etc

CODEDIRS=src
INCDIRS=include
OPT = -O2

CC=gcc

INCS = $(foreach D,$(INCDIRS),-I$(D))
LDFLAGS = -lX11

CPPFLAGS = -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=700
CFLAGS= -std=c99 -Wall -Wextra -MD -MP $(OPT) $(INCS) $(CPPFLAGS)

PINESRC=$(foreach D,$(CODEDIRS),$(wildcard $(D)/*.c))
PINEOBJ=$(PINESRC:.c=.o)

MSGSRC = ./client/pinemsg.c
MSGOBJ = $(MSGSRC:.c=.o)

DEPFILES = $(PINESRC:.c=.d)

all: pine pinemsg

pine: $(PINEOBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $<

pinemsg: $(MSGOBJ)
	$(CC) -o $@ $^

$(MSGOBJ): $(MSGSRC)
	$(CC) -std=c99 -Wall -Wextra $(CPPFLAGS)  -c -o $@ $<

clean:
	rm -rf $(PINEOBJ) $(DEPFILES) pine pinemsg $(MSGOBJ)

install:
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -d $(DESTDIR)/usr/share/xsessions/
	
	install -m 755 pine $(DESTDIR)$(PREFIX)/bin/
	install -m 755 pinemsg $(DESTDIR)$(PREFIX)/bin/
	install -m 644 pine.desktop $(DESTDIR)/usr/share/xsessions/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/pine
	rm -f $(DESTDIR)$(PREFIX)/bin/pinemsg
	rm -f $(DESTDIR)/usr/share/xsessions/pine.desktop

-include $(DEPFILES)

.PHONY: all clean install uninstall
