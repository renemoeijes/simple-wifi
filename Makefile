# Makefile for wifi-config-ap

# Compiler and flags
CC      ?= gcc
CFLAGS  ?= -O2 -g -Wall -Isrc
LDFLAGS ?= -lmicrohttpd

# The final binary name
TARGET = wifi-config-ap

# Source files
SRCS = src/main.c src/http_server.c
OBJS = $(SRCS:.c=.o)

# Phony targets
.PHONY: all clean install

# Default target
all: $(TARGET)

# Link the object files into the final binary
$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up built files
clean:
	rm -f $(TARGET) $(OBJS)

# Install the binary for packaging
install: all
	mkdir -p $(DESTDIR)/usr/bin
	cp $(TARGET) $(DESTDIR)/usr/bin/
	chmod +x $(DESTDIR)/usr/bin/$(TARGET)
