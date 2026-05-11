CC=gcc
CFLAGS=-Wall -Wextra -O2
TARGET=ftp_client
SRC=src/ftp_client.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

list: $(TARGET)
	./$(TARGET) 127.0.0.1 21 user pass list

get: $(TARGET)
	./$(TARGET) 127.0.0.1 21 user pass get hello.txt

clean:
	rm -f $(TARGET) downloaded_*
