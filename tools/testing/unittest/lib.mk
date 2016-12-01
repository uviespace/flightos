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

define EMIT_TESTS
	@for TEST in $(TEST_PROGS); do \
		echo "(./$$TEST && echo \"selftests: $$TEST [PASS]\") || echo \"selftests: $$TEST [FAIL]\""; \
	done;
endef

emit_tests:
	$(EMIT_TESTS)

.PHONY: run_tests all clean emit_tests
