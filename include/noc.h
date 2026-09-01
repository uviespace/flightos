/**
 * @file include/noc.h
 * @ingroup noc_dma
 *
 * @brief Platform selection and address/IRQ mapping for the NoC DMA driver.
 *
 * MPPB and SSDP configuration headers provide the NoC DMA base, scratch SRAM,
 * and channel interrupt mapping selected by the build. The SSDP values are
 * currently an MPPB baseline and need review.
 */

#ifndef _NOC_H_
#define _NOC_H_


#ifdef CONFIG_MPPB

#include <mppb.h>

/** @brief NOC DMA base address (MPPB configuration) */
#define NOC_DMA_BASE_ADDR	MPPB_NOC_DMA_BASE_ADDR

/** @brief NOC scratch buffer base address (MPPB configuration) */
#define NOC_SCRATCH_BUFFER_BASE	MPPB_SRAM_NOC_BASE

/** @brief NOC scratch buffer size (MPPB configuration) */
#define NOC_SCRATCH_BUFFER_SIZE	MPPB_SRAM_NOC_SIZE

/** @brief get the DMA interrupt request level for a channel (MPPB configuration) */
#define NOC_GET_DMA_IRL(chan) MPPB_NOC_GET_DMA_IRL(chan)

#endif /* CONFIG_MPPB */


#ifdef CONFIG_SSDP

#include <ssdp.h>

/** @brief NOC DMA base address (SSDP configuration) */
#define NOC_DMA_BASE_ADDR	SSDP_NOC_DMA_BASE_ADDR

/** @brief NOC scratch buffer base address (SSDP configuration) */
#define NOC_SCRATCH_BUFFER_BASE	SSDP_SRAM_NOC_BASE

/** @brief NOC scratch buffer size (SSDP configuration) */
#define NOC_SCRATCH_BUFFER_SIZE	SSDP_SRAM_NOC_SIZE

/** @brief get the DMA interrupt request level for a channel (SSDP configuration) */
#define NOC_GET_DMA_IRL(chan) SSDP_NOC_GET_DMA_IRL(chan)

#endif /* CONFIG_SSDP */


#endif /* _NOC_H_ */
