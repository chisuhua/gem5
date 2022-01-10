
CPP := g++

TEST_OBJS := hello.o hello_cpu.o hello_opu.o
TEST_PROGS := $(TEST_OBJS:.o=)

.PHONY: default clean

default: $(TEST_PROGS)

clean:
	$(RM) $(TEST_OBJS) $(TEST_PROGS)

$(TEST_PROGS): $(TEST_OBJS)
	$(CPP) -static -o $@ $@.o

%.o: %.c Makefile
	$(CPP) -c -o $@ $*.c -msse3
