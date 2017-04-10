/**
 * @file include/xentium_demo.h
 */

#ifndef _XENTIUM_DEMO_H_
#define _XENTIUM_DEMO_H_

/**
 * Some implementation dependent op info passed by whatever created the task
 * this could also just exist in the <data> buffer as a interpretable structure.
 * This is really up to the user...
 * Note: the xentium kernel processing a task must know the same structure
 */

struct ramp_op_info {
	unsigned int ramplen;
};

struct deglitch_op_info {
	unsigned int sigclip;
};

struct stack_op_info {
	unsigned int stackframes;
};

#endif /* _XENTIUM_DEMO_H_ */
