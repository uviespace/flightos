CPPFLAGS += -DCONFIG_KERNEL_PRINTK

CFLAGS += -g
CFLAGS += -I.
CFLAGS += -I../
CFLAGS += -I../shared
CFLAGS += -I../../../../include/
CFLAGS += -I../../../../include/kernel
CFLAGS += -I../../../../kernel

LDFLAGS += -Wl,--wrap=kmalloc
LDFLAGS += -Wl,--wrap=ktime_get

TEST_PROGS := edf_test


$(TEST_PROGS): edf_test.o \
	../shared/kmalloc_test_wrapper.o \
	../shared/wrap_ktime_get.o \
	../../../../kernel/time.o \
	../../../../kernel/printk.o


all: $(TEST_PROGS)


include ../lib.mk

clean:
	$(RM) $(TEST_PROGS) edf_test.o \
			    ../shared/kmalloc_test_wrapper.o \
			    ../shared/wrap_ktime_get.o \
			    ../../../../kernel/time.o \
			    ../../../../kernel/printk.o
