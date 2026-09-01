/**
 * @file include/xentium_demo.h
 */

#ifndef _XENTIUM_DEMO_H_
#define _XENTIUM_DEMO_H_

/**
 * @brief op info for the ramp operation
 *
 * @note some implementation dependent op info passed by whatever created the
 *       task; this could also just exist in the <data> buffer as an
 *       interpretable structure. This is really up to the user.
 *       Note: the xentium kernel processing a task must know the same structure
 */
struct ramp_op_info {
	unsigned int ramplen;	/*!< ramp length */
};

/**
 * @brief op info for the deglitch operation
 */
struct deglitch_op_info {
	unsigned int sigclip;	/*!< signal clip level */
};

/**
 * @brief op info for the stack operation
 */
struct stack_op_info {
	unsigned int stackframes;	/*!< number of stack frames */
};

#endif /* _XENTIUM_DEMO_H_ */
