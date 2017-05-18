CFLAGS += -g -I../ -I../shared -I../../../../include/ -I../../../../lib
LDFLAGS+= -Wl,--wrap=kmalloc -Wl,--wrap=kcalloc

TEST_PROGS := sysctl_test


$(TEST_PROGS): sysctl_test.o \
	../shared/kmalloc_test_wrapper.o \
	../shared/kcalloc_test_wrapper.o


all: $(TEST_PROGS)


include ../lib.mk

clean:
	$(RM) $(TEST_PROGS) sysctl_test.o \
				../shared/kmalloc_test_wrapper.o \
				../shared/kcalloc_test_wrapper.o
