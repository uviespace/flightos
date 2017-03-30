/**
 * @file include/noc_dma.h
 */


#ifndef _NOC_DMA_H_
#define _NOC_DMA_H_

#include <noc.h>
#include <kernel/types.h>
#include <list.h>


#define NOC_DMA_CHANNELS	8

#define NOC_DMA_ACCESS_SIZE_8	0x0
#define NOC_DMA_ACCESS_SIZE_16	0x1
#define NOC_DMA_ACCESS_SIZE_32	0x2
#define NOC_DMA_ACCESS_SIZE_64	0x3


#define NOC_DMA_PRIORITY_LOW	0x0
#define NOC_DMA_PRIORITY_HIGH	0x1

#define NOC_DMA_IRQ_FWD_NONE	0x0
#define NOC_DMA_IRQ_FWD_IN	0x1
#define NOC_DMA_IRQ_FWD_OUT	0x2
#define NOC_DMA_IRQ_FWD_BOTH	0x3


#define NOC_DMA_STRIDE_MAX    32767
#define NOC_DMA_STRIDE_MIN   -32767

#define NOC_DMA_SIZE_MAX      65535

#define NOC_DMA_CHANNEL_START	0x1
#define NOC_DMA_CHANNEL_BUSY	NOC_DMA_CHANNEL_START


#define NOC_DMA_STRIDE_SRC_OFFSET	16
#define NOC_DMA_STRIDE_DST_MASK		0xffff

#define NOC_DMA_SIZE_Y_OFFSET		16
#define NOC_DMA_SIZE_X_MASK		0xffff


#define NOC_DMA_STRIDES(src, dst)	((src << NOC_DMA_STRIDE_SRC_OFFSET) | \
					 (dst &  NOC_DMA_STRIDE_DST_MASK))

#define NOC_DMA_SIZES(x, y)		((y << NOC_DMA_SIZE_Y_OFFSET) | \
					 (x &  NOC_DMA_SIZE_X_MASK))


#ifdef CONFIG_NOC_DMA_TRANSFER_QUEUE_SIZE
#define NOC_DMA_TRANSFER_QUEUE_SIZE CONFIG_NOC_DMA_TRANSFER_QUEUE_SIZE
#else
#define NOC_DMA_TRANSFER_QUEUE_SIZE 32
#endif

enum noc_dma_elem_size {BYTE       = NOC_DMA_ACCESS_SIZE_8,
			HALFWORD   = NOC_DMA_ACCESS_SIZE_16,
			WORD       = NOC_DMA_ACCESS_SIZE_32,
			DOUBLEWORD = NOC_DMA_ACCESS_SIZE_64};

enum noc_dma_priority {LOW  = NOC_DMA_PRIORITY_LOW,
		       HIGH = NOC_DMA_PRIORITY_HIGH};

enum noc_dma_irq_fwd {OFF  = NOC_DMA_IRQ_FWD_NONE,
		      IN   = NOC_DMA_IRQ_FWD_IN,
		      OUT  = NOC_DMA_IRQ_FWD_OUT,
		      BOTH = NOC_DMA_IRQ_FWD_BOTH};

struct noc_dma_transfer {

	void *src;
	void *dst;

	uint16_t x_elem;		/* number of elements in x */
	uint16_t y_elem;		/* number of elements in y */

	int16_t  x_stride_src;		/* width of stride in source x */
	int16_t  y_stride_src;		/* width of stride in source y */

	int16_t  x_stride_dst;		/* width of stride in destination x */
	int16_t  y_stride_dst;		/* width of stride in destination y */


	uint16_t mtu;				/* maximum packet size */
	enum noc_dma_elem_size elem_size;	/* the element type size */
	enum noc_dma_irq_fwd irq_fwd;		/* irq notification mode */
	enum noc_dma_priority priority;		/* transfer priority */

	/* the caller may specify a callback with user data that is executed
	 * on transfer completion
	 */
	int (*callback)(void *);
	void *userdata;

	struct list_head node;
};

struct noc_dma_channel *noc_dma_reserve_channel(void);
void noc_dma_release_channel(struct noc_dma_channel *c);

int noc_dma_req_xfer(void *src, void *dst, uint16_t x_elem, uint16_t y_elem,
		     enum noc_dma_elem_size elem_size,
		     int16_t x_stride_src, int16_t x_stride_dst,
		     int16_t y_stride_src, int16_t y_stride_dst,
		     enum noc_dma_priority dma_priority, uint16_t mtu,
		     int (*callback)(void *), void *userdata);

int noc_dma_req_lin_xfer(void *src, void *dst,
			 uint16_t elem, enum noc_dma_elem_size elem_size,
			 enum noc_dma_priority dma_priority, uint16_t mtu,
			 int (*callback)(void *), void *userdata);


#endif /* _NOC_DMA_H_ */
