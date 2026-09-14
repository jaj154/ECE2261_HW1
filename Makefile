CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2
LDFLAGS = -lm
SRCS    = main.c io_unit.c memory_unit.c compute_unit.c sensor_hw.c instrumentation.c
OBJS    = $(SRCS:.c=.o)
TARGET  = critter_pipeline

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
