/*
 * kselftest.h:	kselftest framework return codes to include from
 *		selftests.
 *
 * Copyright (c) 2014 Shuah Khan <shuahkh@osg.samsung.com>
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 * Copyright (c) 2016 Armin Luntzer <armin.luntzer@univie.ac.at>
 *
 * This file is released under the GPLv2.
 */
#ifndef __KSELFTEST_H
#define __KSELFTEST_H

#include <stdlib.h>
#include <unistd.h>

/* define kselftest exit codes */
#define KSFT_PASS  0
#define KSFT_FAIL  1
#define KSFT_XFAIL 2
#define KSFT_XPASS 3
#define KSFT_SKIP  4

/* counters */
struct ksft_count {
	unsigned int ksft_pass;
	unsigned int ksft_fail;
	unsigned int ksft_xfail;
	unsigned int ksft_xpass;
	unsigned int ksft_xskip;
};

static struct ksft_count ksft_cnt;

static inline void ksft_inc_pass_cnt(void) { ksft_cnt.ksft_pass++; }
static inline void ksft_inc_fail_cnt(void) { ksft_cnt.ksft_fail++; }
static inline void ksft_inc_xfail_cnt(void) { ksft_cnt.ksft_xfail++; }
static inline void ksft_inc_xpass_cnt(void) { ksft_cnt.ksft_xpass++; }
static inline void ksft_inc_xskip_cnt(void) { ksft_cnt.ksft_xskip++; }

static inline void ksft_print_cnts(void)
{
	printf("\t\tPass: %d Fail: %d Xfail: %d Xpass: %d, Xskip: %d\n\n",
		ksft_cnt.ksft_pass, ksft_cnt.ksft_fail,
		ksft_cnt.ksft_xfail, ksft_cnt.ksft_xpass,
		ksft_cnt.ksft_xskip);
}

static inline int ksft_exit_pass(void)
{
	exit(KSFT_PASS);
}
static inline int ksft_exit_fail(void)
{
	exit(KSFT_FAIL);
}
static inline int ksft_exit_xfail(void)
{
	exit(KSFT_XFAIL);
}
static inline int ksft_exit_xpass(void)
{
	exit(KSFT_XPASS);
}
static inline int ksft_exit_skip(void)
{
	exit(KSFT_SKIP);
}

static void ksft_assert(int res, unsigned int line,
		 const char *cond, const char *file)
{
	if (!res) {
		printf("\t\tTest failed: %s : %d -> %s\n", file, line, cond);
		ksft_inc_fail_cnt();
	}

	ksft_inc_pass_cnt();
}

#define KSFT_ASSERT(value)					\
{								\
	ksft_assert((int) (value), __LINE__, #value, __FILE__); \
};

#define KSFT_ASSERT_PTR_NOT_NULL(value)					\
{									\
	ksft_assert((int) (value != NULL), __LINE__, #value, __FILE__); \
};

#define KSFT_ASSERT_PTR_NULL(value)					\
{									\
	ksft_assert((int) (value == NULL), __LINE__, #value, __FILE__); \
};


/* yes ... srsly */
#define KSFT_RUN_TEST(str, func)				\
{								\
	printf("Test %s\n", (str));				\
	func();							\
};

#endif /* __KSELFTEST_H */
