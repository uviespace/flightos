/**
 * @file include/noc.h
 */

#ifndef _NOC_H_
#define _NOC_H_


#ifdef CONFIG_MPPB

#include <mppb.h>

#define NOC_DMA_BASE_ADDR	MPPB_NOC_DMA_BASE_ADDR

#define NOC_SCRATCH_BUFFER_BASE	MPPB_SRAM_NOC_BASE
#define NOC_SCRATCH_BUFFER_SIZE	MPPB_SRAM_NOC_SIZE

#define NOC_GET_DMA_IRL(chan) MPPB_NOC_GET_DMA_IRL(chan)

#endif /* CONFIG_MPPB */


#ifdef CONFIG_SSDP

#include <ssdp.h>

#define NOC_DMA_BASE_ADDR	SSDP_NOC_DMA_BASE_ADDR

#define NOC_SCRATCH_BUFFER_BASE	SSDP_SRAM_NOC_BASE
#define NOC_SCRATCH_BUFFER_SIZE	SSDP_SRAM_NOC_SIZE

#define NOC_GET_DMA_IRL(chan) SSDP_NOC_GET_DMA_IRL(chan)

#endif /* CONFIG_SSDP */


#endif /* _NOC_H_ */
