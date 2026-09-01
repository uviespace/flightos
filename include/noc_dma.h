/**
 * @file include/noc_dma.h
 * @ingroup noc_dma
 *
 * @brief High-level request and channel interface for the NoC DMA driver.
 *
 * Transfer descriptions are queued by `kernel/noc_dma.c`, which maps them to
 * platform-provided channel registers and completion IRQs. Platform addresses
 * and IRQ mapping come from `include/noc.h`; supported MPPB/SSDP details are
 * platform-specific.
 */


#ifndef _NOC_DMA_H_
#define _NOC_DMA_H_

#include <noc.h>
#include <kernel/types.h>
#include <list.h>


/** @brief number of DMA channels */
#define NOC_DMA_CHANNELS	8

/** @brief 8-bit access element size */
#define NOC_DMA_ACCESS_SIZE_8	0x0
/** @brief 16-bit access element size */
#define NOC_DMA_ACCESS_SIZE_16	0x1
/** @brief 32-bit access element size */
#define NOC_DMA_ACCESS_SIZE_32	0x2
/** @brief 64-bit access element size */
#define NOC_DMA_ACCESS_SIZE_64	0x3


/** @brief low DMA priority */
#define NOC_DMA_PRIORITY_LOW	0x0
/** @brief high DMA priority */
#define NOC_DMA_PRIORITY_HIGH	0x1

/** @brief no interrupt forwarding */
#define NOC_DMA_IRQ_FWD_NONE	0x0
/** @brief forward interrupts on input */
#define NOC_DMA_IRQ_FWD_IN	0x1
/** @brief forward interrupts on output */
#define NOC_DMA_IRQ_FWD_OUT	0x2
/** @brief forward interrupts on both input and output */
#define NOC_DMA_IRQ_FWD_BOTH	0x3


/** @brief maximum positive stride value */
#define NOC_DMA_STRIDE_MAX    32767
/** @brief minimum (most negative) stride value */
#define NOC_DMA_STRIDE_MIN   -32767

/** @brief maximum transfer size */
#define NOC_DMA_SIZE_MAX      65535

/** @brief value for the start channel control bit */
#define NOC_DMA_CHANNEL_START	0x1
/** @brief busy flag value */
#define NOC_DMA_CHANNEL_BUSY	NOC_DMA_CHANNEL_START


/** @brief bit offset of the source stride within a stride word */
#define NOC_DMA_STRIDE_SRC_OFFSET	16
/** @brief mask of the destination stride within a stride word */
#define NOC_DMA_STRIDE_DST_MASK		0xffff

/** @brief bit offset of the Y size within a size word */
#define NOC_DMA_SIZE_Y_OFFSET		16
/** @brief mask of the X size within a size word */
#define NOC_DMA_SIZE_X_MASK		0xffff


/**
 * @brief pack source and destination strides into a single word
 * @param src: source stride
 * @param dst: destination stride
 * @return packed stride word
 */
#define NOC_DMA_STRIDES(src, dst)	((src << NOC_DMA_STRIDE_SRC_OFFSET) | \
					 (dst &  NOC_DMA_STRIDE_DST_MASK))

/**
 * @brief pack X and Y sizes into a single word
 * @param x: X dimension size
 * @param y: Y dimension size
 * @return packed size word
 */
#define NOC_DMA_SIZES(x, y)		((y << NOC_DMA_SIZE_Y_OFFSET) | \
					 (x &  NOC_DMA_SIZE_X_MASK))


#ifdef CONFIG_NOC_DMA_TRANSFER_QUEUE_SIZE
#define NOC_DMA_TRANSFER_QUEUE_SIZE CONFIG_NOC_DMA_TRANSFER_QUEUE_SIZE
#else
#define NOC_DMA_TRANSFER_QUEUE_SIZE 32
#endif

/**
 * @brief DMA element sizes
 */
enum noc_dma_elem_size {BYTE       = NOC_DMA_ACCESS_SIZE_8,
			HALFWORD   = NOC_DMA_ACCESS_SIZE_16,
			WORD       = NOC_DMA_ACCESS_SIZE_32,
			DOUBLEWORD = NOC_DMA_ACCESS_SIZE_64};

/**
 * @brief DMA transfer priorities
 */
enum noc_dma_priority {LOW  = NOC_DMA_PRIORITY_LOW,
		       HIGH = NOC_DMA_PRIORITY_HIGH};

/**
 * @brief DMA interrupt forwarding modes
 */
enum noc_dma_irq_fwd {OFF  = NOC_DMA_IRQ_FWD_NONE,
		      IN   = NOC_DMA_IRQ_FWD_IN,
		      OUT  = NOC_DMA_IRQ_FWD_OUT,
		      BOTH = NOC_DMA_IRQ_FWD_BOTH};

/**
 * @brief a single NOC DMA transfer request
 */
struct noc_dma_transfer {

	void *src;		/*!< source address */
	void *dst;		/*!< destination address */

	uint16_t x_elem;	/*!< number of elements in x */
	uint16_t y_elem;	/*!< number of elements in y */

	int16_t  x_stride_src;	/*!< width of stride in source x */
	int16_t  y_stride_src;	/*!< width of stride in source y */

	int16_t  x_stride_dst;	/*!< width of stride in destination x */
	int16_t  y_stride_dst;	/*!< width of stride in destination y */


	uint16_t mtu;				/*!< maximum packet size */
	enum noc_dma_elem_size elem_size;	/*!< the element type size */
	enum noc_dma_irq_fwd irq_fwd;		/*!< irq notification mode */
	enum noc_dma_priority priority;		/*!< transfer priority */

	int (*callback)(void *);	/*!< completion callback */
	void *userdata;			/*!< user data passed to callback */

	struct list_head node;		/*!< node in transfer queue */
};

/**
 * @brief reserve a DMA channel
 * @return pointer to the reserved channel, or NULL on failure
 */
struct noc_dma_channel *noc_dma_reserve_channel(void);

/**
 * @brief release a previously reserved DMA channel
 * @param c: the channel to release
 */
void noc_dma_release_channel(struct noc_dma_channel *c);

/**
 * @brief request a multi-dimensional DMA transfer
 * @param src: source address
 * @param dst: destination address
 * @param x_elem: number of elements in x
 * @param y_elem: number of elements in y
 * @param elem_size: element size
 * @param x_stride_src: source x stride
 * @param x_stride_dst: destination x stride
 * @param y_stride_src: source y stride
 * @param y_stride_dst: destination y stride
 * @param dma_priority: transfer priority
 * @param mtu: maximum packet size
 * @param callback: completion callback (or NULL)
 * @param userdata: user data passed to callback
 * @return 0 on success, negative error code on failure
 */
int noc_dma_req_xfer(void *src, void *dst, uint16_t x_elem, uint16_t y_elem,
		     enum noc_dma_elem_size elem_size,
		     int16_t x_stride_src, int16_t x_stride_dst,
		     int16_t y_stride_src, int16_t y_stride_dst,
		     enum noc_dma_priority dma_priority, uint16_t mtu,
		     int (*callback)(void *), void *userdata);

/**
 * @brief request a linear (1D) DMA transfer
 * @param src: source address
 * @param dst: destination address
 * @param elem: number of elements
 * @param elem_size: element size
 * @param dma_priority: transfer priority
 * @param mtu: maximum packet size
 * @param callback: completion callback (or NULL)
 * @param userdata: user data passed to callback
 * @return 0 on success, negative error code on failure
 */
int noc_dma_req_lin_xfer(void *src, void *dst,
			 uint16_t elem, enum noc_dma_elem_size elem_size,
			 enum noc_dma_priority dma_priority, uint16_t mtu,
			 int (*callback)(void *), void *userdata);


#endif /* _NOC_DMA_H_ */
