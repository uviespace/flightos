TARGETS += sysctl

#Please keep the TARGETS list alphabetically sorted

# Clear LDFLAGS and MAKEFLAGS if called from main
# Makefile to avoid test build failures when test
# Makefile doesn't have explicit build rules.

CFLAGS += -fprofile-arcs -ftest-coverage

ifeq (1,$(MAKELEVEL))
override LDFLAGS =
override MAKEFLAGS =
else
export CFLAGS
export LDFLAGS = -lgcov --coverage
endif


all:
	for TARGET in $(TARGETS); do \
		make -C $$TARGET; \
	done;

run_tests: all
	for TARGET in $(TARGETS); do \
		make -C $$TARGET run_tests; \
	done;
clean:
	for TARGET in $(TARGETS); do \
		make -C $$TARGET clean; \
	done;

.PHONY: install
