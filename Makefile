CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
AR = ar

SRC = src/agent_rhythm.c
OBJ = $(SRC:.c=.o)
LIB = libagent_rhythm.a
HDR = include/agent_rhythm.h

.PHONY: all lib test clean

all: lib

lib: $(LIB)

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

src/%.o: src/%.c $(HDR)
	$(CC) $(CFLAGS) -Iinclude -c -o $@ $<

test: $(LIB) tests/test_rhythm.c $(HDR)
	$(CC) $(CFLAGS) -Iinclude -o test_rhythm tests/test_rhythm.c $(LIB) -lm
	./test_rhythm

clean:
	rm -f src/*.o $(LIB) test_rhythm
