/**
 * @file dsp/xentium/include/xdma.h
 * @ingroup xen
 */

#ifndef _DSP_XENTIUM_DMA_H_
#define _DSP_XENTIUM_DMA_H_

#include <stddef.h>
#include <noc_dma.h>



int xen_noc_dma_req_xfer(struct noc_dma_channel *c,
			 void *src, void *dst, uint16_t x_elem, uint16_t y_elem,
			 enum noc_dma_elem_size elem_size,
			 int16_t x_stride_src, int16_t x_stride_dst,
			 int16_t y_stride_src, int16_t y_stride_dst,
			 enum noc_dma_priority dma_priority, uint16_t mtu);


int xen_noc_dma_req_lin_xfer(struct noc_dma_channel *c,
			     void *src, void *dst,
			     uint16_t elem, enum noc_dma_elem_size elem_size,
			     enum noc_dma_priority dma_priority, uint16_t mtu);

#endif /* _DSP_XENTIUM_DMA_H_ */
