CC := gcc
CFLAGS += -Wall -std=gnu99

TARGET := SnailShell
SRCS := SnailShell.c Parse.c Run.c
OBJS := $(SRCS:.c=.o)

$(TARGET): $(SRCS:.c=.o)
	$(CC) $(CFLAGS) -o $(TARGET) $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) *.o *.so
	rm -f *.txt