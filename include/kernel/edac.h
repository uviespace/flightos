/**
 * @file    include/kernel/edac.h
 * @ingroup kmem
 * @author  Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 * @ingroup edacsys
 *
 * @brief kernel-side EDAC (Error Detection And Correction) interface
 *
 * Declares the application-facing API for the EDAC subsystem, i.e. the
 * high-level abstracted interface implemented in kernel/edac.c. Operations are
 * forwarded to an architecture-specific backend described by struct edac_dev.
 *
 * The struct edac_dev type is the abstraction boundary between the generic
 * layer and a platform backend. Each member is a function pointer that a
 * backend (e.g. arch/sparc/kernel/edac.c) must implement; the generic layer
 * dispatches its exported edac_*() functions to these pointers. See the
 * @ref edacsys group page for the overall architecture.
 *
 * @copyright GPLv2
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef _KERNEL_EDAC_H_
#define _KERNEL_EDAC_H_

#include <kernel/types.h>



/**
 * @brief EDAC backend implementation
 *
 * Function pointers to the architecture-specific EDAC implementation that the
 * kernel-level edac.c forwards its calls to.
 */
struct edac_dev {
        void           (*enable)            (void);	/*!< enable EDAC */
        void           (*disable)           (void);	/*!< disable EDAC */
        int            (*crit_seg_add)      (void *begin, void *end); /*!< add a critical segment */
        int            (*crit_seg_rem)	    (void *begin, void *end); /*!< remove a critical segment */
	int            (*error_detected)    (void);	/*!< report whether an error was detected */
	unsigned long  (*get_error_addr)    (void);	/*!< return the address of the last error */
	void           (*error_clear)       (void);	/*!< clear the error status */
        void           (*inject_fault)      (void *addr, uint32_t mem_value, uint32_t edac_value); /*!< inject an EDAC fault */
        unsigned long  (*bypass_read)       (void *addr);	/*!< read memory bypassing EDAC checkbits */
	void           (*set_reset_handler) (void (*handler)(void *), void *data); /*!< install a reset handler */
};


/**
 * @brief inject a synthetic EDAC error at an address
 *
 * @param addr the address to inject the fault at
 * @param mem_value the value written to memory
 * @param edac_value the value written to the checkbit storage
 */
void edac_inject_fault(void *addr, uint32_t mem_value, uint32_t edac_value);

/**
 * @brief read a memory location bypassing the EDAC checkbit logic
 *
 * @param addr the address to read
 *
 * @return the value read from memory
 */
unsigned long edac_bypass_read(void *addr);

/**
 * @brief set a reset handler callback in case a double bit error occurs
 *
 * @param handler pointer to the reset handler function to be called
 * @param userdata pointer passed as argument to @p handler when invoked
 */
void edac_set_reset_callback(void (*handler)(void *), void *userdata);

/**
 * @brief add a critical memory segment
 *
 * @param begin pointer to the start of the segment
 * @param end pointer to the end of the segment
 *
 * @return 0 on success, negative error code otherwise
 */
int edac_critical_segment_add(void *begin, void *end);

/**
 * @brief remove a critical memory segment
 *
 * @param begin pointer to the start of the segment
 * @param end pointer to the end of the segment
 *
 * @return 0 on success, negative error code otherwise
 */
int edac_critical_segment_rem(void *begin, void *end);

/**
 * @brief report whether an EDAC error was detected
 *
 * @return nonzero if an error was detected, 0 otherwise
 */
int edac_error_detected(void);

/**
 * @brief return the address of the detected EDAC error
 *
 * @return the address of the error
 */
unsigned long edac_get_error_addr(void);

/**
 * @brief clear the EDAC error status
 */
void edac_error_clear(void);

/**
 * @brief enable EDAC
 */
void edac_enable(void);

/**
 * @brief disable EDAC
 */
void edac_disable(void);

/**
 * @brief initialise the EDAC subsystem with a backend
 *
 * @param dev the struct edac_dev backend to use
 */
void edac_init(struct edac_dev *dev);

#endif /* _KERNEL_EDAC_H_ */
