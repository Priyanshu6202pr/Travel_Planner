CC     = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -g
TARGET = travel_planner

SRCS   = main.c utils.c avl.c trip.c advanced.c persistence.c menu.c
OBJS   = $(SRCS:.c=.o)
HDRS   = utils.h avl.h trip.h advanced.h persistence.h menu.h

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

run: all
	./$(TARGET)
