CC = gcc
CFLAGS = -Wall -Wextra -Werror -g
SOURCES = client.c command_handlers.c player.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = player

all: $(TARGET)
	$(CC) -o $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -c $(SOURCES)

clean:
	rm -f $(OBJECTS) $(TARGET)