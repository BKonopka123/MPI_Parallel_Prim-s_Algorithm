CC = mpicc

CFLAGS = -Wall -Wextra -O2

SRCS = main.c
OBJS = $(SRCS:.c=.o)

TARGET = main_program

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)