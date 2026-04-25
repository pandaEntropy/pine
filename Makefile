BINARY=moss
CODEDIRS=src
INCDIRS=include
BAR=bar/bar

CC=gcc

# add -I to each include directory
CFLAGS=-Wall -Wextra -MMD -MP $(foreach D,$(INCDIRS),-I$(D))
LDFLAGS=-lX11

CFILES=$(foreach D,$(CODEDIRS),$(wildcard $(D)/*.c))
OBJECTS=$(patsubst %.c,%.o,$(CFILES))

all: $(BINARY) $(BAR)

$(BAR): bar/bar.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(BINARY): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $<

-include $(OBJECTS:.o=.d)

clean:
	rm -rf $(BINARY) $(OBJECTS) $(OBJJECTS:.o=.d)
