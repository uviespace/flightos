TARGETS += edf sysctl

#Please keep the TARGETS list alphabetically sorted

# Clear LDFLAGS and MAKEFLAGS if called from main
# Makefile to avoid test build failures when test
# Makefile doesn't have explicit build rules.


export CFLAGS = -fprofile-arcs -ftest-coverage
export LDFLAGS = -lgcov --coverage

ifeq (1,$(MAKELEVEL))
override MAKEFLAGS =
else
endif


all:
	for TARGET in $(TARGETS); do \
		make -C $$TARGET; \
	done;

run_tests: all
	for TARGET in $(TARGETS); do \
		make -C $$TARGET run_tests; \
	done;

coverage_tests: all
	for TARGET in $(TARGETS); do \
		make -C $$TARGET coverage_tests; \
	done;

clean_coverage:
	for TARGET in $(TARGETS); do \
		make -C $$TARGET clean_coverage; \
	done;
	rm shared/*.gcno shared/*.gcda

clean: clean_coverage
	for TARGET in $(TARGETS); do \
		make -C $$TARGET clean; \
	done;

.PHONY: 
