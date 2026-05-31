CC=gcc
CFLAGS=-Wall -Wextra -O2
TARGET=ftp_client
SRC=src/ftp_client.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) downloaded_* *.o
