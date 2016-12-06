# This mimics the top-level Makefile. We do it explicitly here so that this
# Makefile can operate with or without the kbuild infrastructure.
CC := $(CROSS_COMPILE)gcc

# TODO add coverage analysis
#		lcov --rc lcov_branch_coverage=1 --capture --directory ./ --output-file coverage.info
#		genhtml --branch-coverage coverage.info --output-directory out
#

define RUN_TESTS
	@for TEST in $(TEST_PROGS); do \
		(./$$TEST && echo "selftests: $$TEST [PASS]") || echo "selftests: $$TEST [FAIL]"; \
	done;
endef

run_tests: all
	$(RUN_TESTS)

define COVERAGE_TESTS
	@for TEST in $(TEST_PROGS); do \
		lcov --rc lcov_branch_coverage=1 --capture --directory ./ --output-file coverage.info; \
		genhtml --branch-coverage coverage.info --output-directory out; \
	done;
endef

coverage_tests: run_tests
	$(COVERAGE_TESTS)


define CLEAN_COVERAGE
	@for TEST in $(TEST_PROGS); do \
		$(RM) -r $$TEST $$TEST.gcno $$TEST.gcda coverage.info out/; \
	done;
endef

clean_coverage:
	$(CLEAN_COVERAGE)

.PHONY: run_tests all clean coverage_tests clean_coverage
