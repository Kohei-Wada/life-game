OBJS=life-game.o
SRCS=$(OBJS:%.o=%.c)
CFLAGS=-g -Wall -pthread
LDLIBS=
TARGET=lg
$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LDLIBS)
