/**
 * @file dsp/xentium/lib/xdma.c
 *
 * These are mostly just duplicates of the noc_dma access functions...
 */

#include <errno.h>
#include <dma.h>


#define ioread32(X)                   __raw_readl(X)
#define iowrite32(val,X)              __raw_writel(val,X)


static inline uint32_t __raw_readl(const volatile void *addr)
{
        return (*(const volatile uint32_t *) addr);
}


static inline void __raw_writel(uint32_t l, volatile void *addr)
{
        (*(volatile uint32_t *) addr) = l;
}


/**
 * NoC DMA channel control register layout
 * @see MPPB datasheet v4.03, p64
 */

__extension__
struct noc_dma_channel_ctrl {
	union {
		uint32_t ctrl;
		struct {
			uint32_t priority   :1;
			uint32_t irq_fwd    :2;
			uint32_t acc_sz     :2;
			uint32_t reserved   :11;
			uint32_t max_pkt_sz :16;
		}__attribute__((packed));
	};
};


/**
 * NoC DMA channel configuration register layout
 * @see MPPB datasheet v4.03, p63
 */
__extension__
struct noc_dma_channel {

	union {
		uint32_t start;
		uint32_t status;
		struct {
			uint32_t start_status:1;
			uint32_t res01	     :31;	/* reserved */
		}__attribute__((packed));
	};

	union {
		uint32_t control;
		struct noc_dma_channel_ctrl ctrl;
	};

	uint32_t dst;

	uint32_t src;

	union {
		uint32_t sizes;
		struct {
			uint16_t sz_y;
			uint16_t sz_x;
		}__attribute__((packed));
	};

	union {
		uint32_t strides_x;
		struct {
			uint16_t str_x_src;
			uint16_t str_x_dst;
		}__attribute__((packed));
	};

	union {
		uint32_t strides_y;
		struct {
			uint16_t str_y_src;
			uint16_t str_y_dst;
		}__attribute__((packed));
	};

	uint32_t res02[9];

}__attribute__((packed));





/**
 * @brief check if a NoC DMA channel is in use
 *
 * @param chan a struct noc_dma_channel
 *
 * @returns 0 if not busy
 */

static int noc_dma_channel_busy(struct noc_dma_channel *chan)
{
	return ioread32(&chan->status) & NOC_DMA_CHANNEL_BUSY;
}


/**
 * @brief set source and destination strides in x
 *
 * @param chan a struct noc_dma_channel
 * @param stride_src the source stride
 * @param stride_dst the destination stride
 */

static void noc_dma_set_strides_x(struct noc_dma_channel *chan,
				  int16_t stride_src, int16_t stride_dst)
{
	iowrite32(NOC_DMA_STRIDES(stride_src, stride_dst), &chan->strides_x);
}


/**
 * @brief set source and destination strides in y
 *
 * @param chan a struct noc_dma_channel
 * @param stride_src the source stride
 * @param stride_dst the destination stride
 */

static void noc_dma_set_strides_y(struct noc_dma_channel *chan,
				  int16_t stride_src, int16_t stride_dst)
{
	iowrite32(NOC_DMA_STRIDES(stride_src, stride_dst), &chan->strides_y);
}


/**
 * @brief set transfer sizes in x and y
 *
 * @param chan a struct noc_dma_channel
 * @param size_x the number of elements to transfer in x
 * @param size_y the number of elements to transfer in y
 */

static void noc_dma_set_sizes(struct noc_dma_channel *chan,
			      int16_t size_x, uint16_t size_y)
{
	iowrite32(NOC_DMA_SIZES(size_x, size_y), &chan->sizes);
}


/**
 * @brief set transfer source address
 *
 * @param chan a struct noc_dma_channel
 * @param src the source memory location
 */

static void noc_dma_set_src(struct noc_dma_channel *chan, void *src)
{
	iowrite32((uint32_t) src, &chan->src);
}


/**
 * @brief set transfer destination address
 *
 * @param chan a struct noc_dma_channel
 * @param dst the destination memory location
 */

static void noc_dma_set_dst(struct noc_dma_channel *chan, void *dst)
{
	iowrite32((uint32_t) dst, &chan->dst);
}


/**
 * @brief start a DMA transfer
 * @param chan a struct noc_dma_channel
 *
 * @returns 0 on success, -EBUSY if channel is active
 */

static int noc_dma_start_transfer(struct noc_dma_channel *chan)
{
	if (noc_dma_channel_busy(chan))
		return -EBUSY;

	iowrite32(NOC_DMA_CHANNEL_START, &chan->start);

	/* XXX remove once we figure out how to properly use the Xentium's
	 * DMA status bits
	 */
	while (noc_dma_channel_busy(chan));

	return 0;
}


/**
 * @brief sets the channel configuration for a transfer
 *
 * @param chan a struct noc_dma_channel
 * @param max_pkt_size the maximum packet size to use during the transfer
 * @param elem_size the size of a transfer element
 * @param irq_fwd the irq notification mode on transfer completion
 * @param priority the transfer priority
 *
 * @returns 0 on success, -EBUSY if channel is active
 */

static int noc_dma_set_config(struct noc_dma_channel *chan,
			      int16_t max_pkt_size,
			      enum noc_dma_elem_size elem_size,
			      enum noc_dma_irq_fwd irq_fwd,
			      enum noc_dma_priority priority)
{
	struct noc_dma_channel_ctrl ctrl;


	if (noc_dma_channel_busy(chan))
		return -EBUSY;

	ctrl.max_pkt_sz = max_pkt_size;
	ctrl.acc_sz     = elem_size;
	ctrl.irq_fwd    = irq_fwd;
	ctrl.priority   = priority;

	iowrite32(ctrl.ctrl, &chan->ctrl);

	return 0;
}


/**
 * @brief initialises the data parameters for a one dimensional DMA transfer
 *
 * @param chan a struct noc_dma_channel
 * @param chan a struct noc_dma_transfer
 *
 * @returns 0 on success, EINVAL on error, EBUSY if channel is busy
 */

static int noc_dma_init_transfer(struct noc_dma_channel  *c,
				 struct noc_dma_transfer *t)
{

	if (noc_dma_channel_busy(c))
		return -EBUSY;

	/* src == dst memory will result in a stuck DMA channel */
	if (((int) t->dst & 0xF0000000) == ((int) t->src & 0xF0000000))
		return -EINVAL;

	/* no need to verify stride/size limits, they are implicity by type */

	noc_dma_set_src(c, t->src);
	noc_dma_set_dst(c, t->dst);

	noc_dma_set_strides_x(c, t->x_stride_src, t->x_stride_dst);
	noc_dma_set_strides_y(c, t->y_stride_src, t->y_stride_dst);

	noc_dma_set_sizes(c, t->x_elem, t->y_elem);

	noc_dma_set_config(c, t->mtu, t->elem_size, t->irq_fwd, t->priority);

	return 0;
}



/**
 * @brief request an arbitrary DMA transfer
 *
 * @param c the DMA channel to use
 *
 * @param src  the source address
 * @param dst the destination address
 *
 * @param x_elem the number of elements in x
 * @param y_elem the number of elements in y
 * @param size the element size (BYTE, HALFWORD, WORD, DOUBLEWORD)
 *
 * @param x_stride_src the width of stride in source x
 * @param x_stride_dst the width of stride in destination x
 *
 * @param y_stride_src the width of stride in source y
 * @param y_stride_dst the width of stride in destination y
 *
 * @param mtu the maximum transfer unit of a NoC packet
 *
 * @returns <0 on error
 */

int xen_noc_dma_req_xfer(struct noc_dma_channel *c,
			 void *src, void *dst, uint16_t x_elem, uint16_t y_elem,
			 enum noc_dma_elem_size elem_size,
			 int16_t x_stride_src, int16_t x_stride_dst,
			 int16_t y_stride_src, int16_t y_stride_dst,
			 enum noc_dma_priority dma_priority, uint16_t mtu)
{
	int ret;
	struct noc_dma_transfer t;



	if (!src)
		return -EINVAL;

	if (!dst)
		return -EINVAL;

	if(!x_elem)
		return -EINVAL;

	if(!y_elem)
		return -EINVAL;


	t.src = src;
	t.dst = dst;

	t.x_elem = x_elem;
	t.y_elem = y_elem;

	t.elem_size = elem_size;

	t.x_stride_src = x_stride_src;
	t.x_stride_dst = x_stride_dst;

	t.y_stride_src = y_stride_src;
	t.y_stride_dst = y_stride_dst;

	t.mtu = mtu;

	t.irq_fwd = IN;

	t.priority = dma_priority;


	ret = noc_dma_init_transfer(c, &t);
	if (ret)
		return ret;

	return noc_dma_start_transfer(c);
}


/**
 * @brief request a linear array DMA transfer
 *
 * @param c the DMA channel to use
 *
 * @param src  the source address
 * @param dst the destination address
 *
 * @param elem the number of elements
 * @param size the element size (BYTE, HALFWORD, WORD, DOUBLEWORD)
 *
 * @param mtu the maximum transfer unit of a NoC packet
 *
 * @returns <0 on error
 */

int xen_noc_dma_req_lin_xfer(struct noc_dma_channel *c,
			     void *src, void *dst,
			     uint16_t elem, enum noc_dma_elem_size elem_size,
			     enum noc_dma_priority dma_priority, uint16_t mtu)
{
	return xen_noc_dma_req_xfer(c, src, dst, elem, 1, elem_size,
				    1, 1, 1, 1, LOW, mtu);
}
