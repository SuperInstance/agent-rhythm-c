CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -O2 -Iinclude

.PHONY: all test clean

all: build/agent_rhythm.a

build:
	mkdir -p build

build/agent_rhythm.o: src/agent_rhythm.c include/agent_rhythm.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/agent_rhythm.a: build/agent_rhythm.o
	ar rcs $@ $<

build/test_agent_rhythm: build/agent_rhythm.o tests/test_agent_rhythm.c
	$(CC) $(CFLAGS) build/agent_rhythm.o tests/test_agent_rhythm.c -lm -o $@

test: build/test_agent_rhythm
	./build/test_agent_rhythm

clean:
	rm -rf build
