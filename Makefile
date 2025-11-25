CC = gcc
CFLAGS = -Wall -pthread
TARGET = skate_session

all: $(TARGET)

$(TARGET): skate_best_trick.c
	$(CC) $(CFLAGS) -o $(TARGET) skate_best_trick.c

clean:
	rm -f $(TARGET)
