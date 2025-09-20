CC=gcc
CFLAGS=-Wall -g -std=c99
LDFLAGS=-lmicrohttpd

# Executable name
TARGET=simple-wifi

# Source files
SRCS = main.c http_server.c

# Object files
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
