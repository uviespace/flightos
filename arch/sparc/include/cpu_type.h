/**
 * @brief supported CPU models
 * @author Armin Luntzer (armin.luntzer@univie.ac.at)
 */

#ifndef _ASM_CPU_TYPE_H_
#define _ASM_CPU_TYPE_H_


/*
 * supported CPU models
 */

enum sparc_cpus {
	sparc_leon  = 0x06,
};

extern enum sparc_cpus sparc_cpu_model;


#endif /* _ASM_CPU_TYPE_H_ */
