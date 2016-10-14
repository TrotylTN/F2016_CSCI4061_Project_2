CC=gcc
CFLAGS=-g -Wall `pkg-config --cflags gtk+-2.0 webkit-1.0`
LDFLAGS+=`pkg-config --libs gtk+-2.0 webkit-1.0`

LIB=/usr/lib/
SOURCES=wrapper.c wrapper.h browser.c
OBJ=browser

all:  $(SOURCES) $(OBJ)

$(OBJ): $(SOURCES)
	$(CC) $(SOURCES) -L ${LIB} $(CFLAGS) $(LDFLAGS) -o $(OBJ)

clean:
	rm -rf $(OBJ)

