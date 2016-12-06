CFLAGS += -g -I../ -I../shared -I../../../../include/ -I../../../../lib
LDFLAGS+= -Wl,--wrap=malloc

TEST_PROGS := sysctl_test


$(TEST_PROGS): sysctl_test.o ../shared/malloc_test_wrapper.o


all: $(TEST_PROGS)


include ../lib.mk

clean:
	$(RM) $(TEST_PROGS) sysctl_test.o ../shared/malloc_test_wrapper.o
