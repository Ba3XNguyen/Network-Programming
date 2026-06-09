CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
TARGET = pop3_client
SRC = src/pop3_client.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
