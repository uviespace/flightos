/**
 * @file kernel/noc_dma.c
 *
 * @note parameter limits are usually not checked, as they are implicit by type
 *
 * @todo stuck channel detection (needs threading or at least a timer service)
 *	 and reset (if possible, transfer size 0 maybe? need to test...)
 */


#include <kernel/module.h>
#include <kernel/kernel.h>
#include <kernel/printk.h>
#include <kernel/string.h>
#include <kernel/kmem.h>
#include <kernel/sysctl.h>
#include <kernel/types.h>
#include <kernel/export.h>


#include <asm/irq.h>
#include <kernel/irq.h>

#include <asm-generic/io.h>
#include <errno.h>
#include <list.h>

#include <noc_dma.h>


#if !defined(CONFIG_MPPB) && !defined(CONFIG_SSDP)
#error "Unsupported platform"
#endif

#define MSG "NOC DMA: "

/**
 * NoC DMA channel control register layout
 * @see MPPB datasheet v4.03, p64
 */

__extension__
struct noc_dma_channel_ctrl {
	union {
		uint32_t ctrl;
#if defined(__BIG_ENDIAN_BITFIELD)
		struct {
			uint32_t max_pkt_sz :16;
			uint32_t reserved   :11;
			uint32_t acc_sz     :2;
			uint32_t irq_fwd    :2;
			uint32_t priority   :1;
		}__attribute__((packed));
#endif
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
#if defined(__BIG_ENDIAN_BITFIELD)
		struct {
			uint32_t res01	     :31;	/* reserved */
			uint32_t start_status:1;
		}__attribute__((packed));
#endif
	};

	union {
		uint32_t control;
#if defined(__BIG_ENDIAN_BITFIELD)
		struct noc_dma_channel_ctrl ctrl;
#endif
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

/* make sure the structure has the same size as the register */
compile_time_assert(sizeof(struct noc_dma_channel) == 0x40,
		    NOC_DMA_CHANNEL_CONFIG_SIZE_INVALID);

/* we use this to track channels in our channel pool */
struct noc_dma_channel_node {

	unsigned int irl;	/* the interrupt line of this channel */

	struct noc_dma_channel	*channel;
	struct noc_dma_transfer	*transfer;

	struct list_head node;
};



/*
 * XXX this is statically assigned for now. If we have an MMU or if we can
 * detect the from something like AMBA PnP, we perhaps want
 * to mmap/configure the address on initialisation
 */
static struct noc_dma_channels {
	struct noc_dma_channel chan[NOC_DMA_CHANNELS];
} *noc_dma = (struct noc_dma_channels *) NOC_DMA_BASE_ADDR;


static struct list_head	dma_channel_pool_head;
static struct list_head	dma_transfer_pool_head;
static struct list_head	dma_transfer_queue_head;

/* we could set this up at runtime, but what for? */
static struct noc_dma_transfer dma_transfer_pool[NOC_DMA_TRANSFER_QUEUE_SIZE];

static struct noc_dma_channel_node dma_channel_pool[NOC_DMA_CHANNELS];


#ifdef CONFIG_NOC_DMA_STATS_COLLECT
static struct {
	unsigned int transfers_queued;
	unsigned int transfers_requested;
	unsigned int transfers_completed;

	unsigned int bytes_requested;
	unsigned int bytes_transferred;
} noc_dma_stats;



__extension__
static ssize_t noc_dma_show(struct sysobj *sobj __attribute__((unused)),
			    struct sobj_attribute *sattr,
			    char *buf)
{
	if (!strcmp(sattr->name, "transfers_queued"))
		return sprintf(buf, "%d", noc_dma_stats.transfers_queued);

	if (!strcmp(sattr->name, "transfers_requested"))
		return sprintf(buf, "%d", noc_dma_stats.transfers_requested);

	if (!strcmp(sattr->name, "transfers_completed"))
		return sprintf(buf, "%d", noc_dma_stats.transfers_completed);

	if (!strcmp(sattr->name, "bytes_requested"))
		return sprintf(buf, "%d", noc_dma_stats.bytes_requested);

	if (!strcmp(sattr->name, "bytes_transferred"))
		return sprintf(buf, "%d", noc_dma_stats.bytes_transferred);

	return 0;
}

__extension__
static ssize_t noc_dma_store(struct sysobj *sobj __attribute__((unused)),
			     struct sobj_attribute *sattr,
			     const char *buf __attribute__((unused)),
			     size_t len __attribute__((unused)))
{
	if (!strcmp(sattr->name, "reset_stats")) {
		noc_dma_stats.transfers_requested = 0;
		noc_dma_stats.transfers_completed = 0;
		noc_dma_stats.bytes_requested     = 0;
		noc_dma_stats.bytes_transferred   = 0;
	}

	return 0;
}

__extension__
static struct sobj_attribute noc_dma_attr[] = {
	__ATTR(transfers_queued,    noc_dma_show, NULL),
	__ATTR(transfers_requested, noc_dma_show, NULL),
	__ATTR(transfers_completed, noc_dma_show, NULL),
	__ATTR(bytes_requested,     noc_dma_show, NULL),
	__ATTR(bytes_transferred,   noc_dma_show, NULL),
	__ATTR(reset_stats,         NULL,         noc_dma_store)
};

__extension__
static struct sobj_attribute *noc_dma_attributes[] = {
	&noc_dma_attr[0], &noc_dma_attr[1], &noc_dma_attr[2],
	&noc_dma_attr[3], &noc_dma_attr[4], &noc_dma_attr[5],
	NULL};

#endif /* CONFIG_NOC_DMA_STATS_COLLECT */




/**
 * @brief check if a NoC DMA channel is in use
 *
 * @param chan a struct noc_dma_channel
 *
 * @returns 0 if not busy
 */

static int noc_dma_channel_busy(struct noc_dma_channel *chan)
{
	return ioread32be(&chan->status) & NOC_DMA_CHANNEL_BUSY;
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
	iowrite32be(NOC_DMA_STRIDES(stride_src, stride_dst), &chan->strides_x);
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
	iowrite32be(NOC_DMA_STRIDES(stride_src, stride_dst), &chan->strides_y);
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
	iowrite32be(NOC_DMA_SIZES(size_x, size_y), &chan->sizes);
}


/**
 * @brief set transfer source address
 *
 * @param chan a struct noc_dma_channel
 * @param src the source memory location
 */

static void noc_dma_set_src(struct noc_dma_channel *chan, void *src)
{
	iowrite32be((uint32_t) src, &chan->src);
}


/**
 * @brief set transfer destination address
 *
 * @param chan a struct noc_dma_channel
 * @param dst the destination memory location
 */

static void noc_dma_set_dst(struct noc_dma_channel *chan, void *dst)
{
	iowrite32be((uint32_t) dst, &chan->dst);
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

	iowrite32be(NOC_DMA_CHANNEL_START, &chan->start);

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

#if defined(__BIG_ENDIAN_BITFIELD)
	ctrl.max_pkt_sz = max_pkt_size;
	ctrl.acc_sz     = elem_size;
	ctrl.irq_fwd    = irq_fwd;
	ctrl.priority   = priority;
#else
#error "Not implemented."
#endif

	iowrite32be(ctrl.ctrl, &chan->ctrl);

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
	/* TODO implement workaround via scratch buffer */
	if (((int) t->dst & 0xF0000000) == ((int) t->src & 0xF0000000)) {
		pr_err(MSG "Cannot transfer withing same memory\n");
		return -EINVAL;
	}

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
 * @brief return a reserved channel
 */

static void noc_dma_release_channel_node(struct noc_dma_channel_node *c)
{
	c->transfer = NULL;

	list_add_tail(&c->node, &dma_channel_pool_head);
}


/**
 * @brief release a transfer item to the pool and return the reserved channel
 */

static void noc_dma_release_transfer(struct noc_dma_transfer *t)
{

	t->callback = NULL;
	t->userdata = NULL;

	list_add_tail(&t->node, &dma_transfer_pool_head);

}


/**
 * @brief initialises the data parameters for a one dimensional DMA transfer
 *
 * @param chan a struct noc_dma_channel
 * @param chan a struct noc_dma_transfer
 *
 * @returns 0 on success, EINVAL on error, EBUSY if channel is busy
 */

static int noc_dma_execute_transfer(struct noc_dma_channel  *c,
				    struct noc_dma_transfer *t)
{
	int ret;

	struct noc_dma_channel_node *n;


	ret = noc_dma_init_transfer(c, t);
	if (ret) {
		n = container_of((struct noc_dma_channel **) &c,
				 struct noc_dma_channel_node, channel);
		noc_dma_release_transfer(t);
		noc_dma_release_channel_node(n);
		return ret;
	}

	ret = noc_dma_start_transfer(c);
	if (ret)
		return ret;

	return 0;
}


/**
 * @brief reserve a NoC DMA channel
 *
 * @param t	a pointer to a struct noc_dma_transfer
 *
 * @warning	channel will never be released when a transfer gets stuck, but
 * @todo	either rely on an (external) timer or write a timer management routine... (probably the best idea)
 * @todo	(if so, make it tickless...)
 */

static inline
struct noc_dma_channel *noc_dma_request_channel(struct noc_dma_transfer *t)
{
	struct noc_dma_channel_node *c;


	if(unlikely(list_empty(&dma_channel_pool_head)))
		return NULL;

	c = list_entry((&dma_channel_pool_head)->next,
		       struct noc_dma_channel_node, node);

	list_del(&c->node);

	/* link transfer to node of this DMA channel */
	c->transfer = t;

#ifdef CONFIG_NOC_DMA_STATS_COLLECT
	noc_dma_stats.transfers_requested++;
#endif
	return c->channel;
}


/**
 * @brief	program queued DMA transfers
 * @note will program transfers until all available DMA channels are in use
 *	 or queue is empty
 */

static inline void noc_dma_program_transfer(void)
{
	struct noc_dma_channel *chan;

	struct noc_dma_transfer *p_elem = NULL;
	struct noc_dma_transfer *p_tmp;


	if(list_empty(&dma_transfer_queue_head))
		return;


	list_for_each_entry_safe(p_elem, p_tmp,
				 &dma_transfer_queue_head, node) {

		chan = noc_dma_request_channel(p_elem);

		if (!chan)
			break;

		list_del(&p_elem->node);

		noc_dma_execute_transfer(chan, p_elem);

#ifdef CONFIG_NOC_DMA_STATS_COLLECT
		noc_dma_stats.transfers_queued--;
#endif
	}
}


/**
 * @brief callback function for NoC DMA IRQs
 *
 * @param userdata userdata pointer supplied by the IRQ system
 *
 * @note will execute a caller-provideable callback notification function
 *
 * @return always 0
 *
 *
 */

/* XXX the user-supplied callbacks should not be executed in the ISR
 *     for obvious reasons. Need to fix that once we have threads.
 */
#warning "NoC DMA: user callbacks are executed in interrupt mode"

static irqreturn_t noc_dma_service_irq_handler(unsigned int irq, void *userdata)
{
	struct noc_dma_transfer *t;
	struct noc_dma_channel_node  *c;



	/* every actual channel has its own node registered to the IRQ system */
	c = (struct noc_dma_channel_node *) userdata;

	t = c->transfer;

	if (t->callback)
		t->callback(t->userdata);

	noc_dma_release_transfer(t);
	noc_dma_release_channel_node(c);

#ifdef CONFIG_NOC_DMA_STATS_COLLECT
	noc_dma_stats.transfers_completed++;
#endif
	/* activate queued */
	noc_dma_program_transfer();


	return 0;
}


/**
 * @brief add a DMA transfer item to the transfer queue
 */

static void noc_dma_register_transfer(struct noc_dma_transfer *t)
{
	list_add_tail(&t->node, &dma_transfer_queue_head);
	noc_dma_program_transfer();

#ifdef CONFIG_NOC_DMA_STATS_COLLECT
	noc_dma_stats.transfers_queued++;
#endif
}



/**
 * @brief reserve a transfer item from the transfer pool
 */

static struct noc_dma_transfer *noc_dma_get_transfer_slot(void)
{
	struct noc_dma_transfer *t;


	if(unlikely(list_empty(&dma_transfer_pool_head)))
		return NULL;

	t = list_entry((&dma_transfer_pool_head)->next,
		       struct noc_dma_transfer, node);

	list_del(&t->node);

	return t;
}



#ifdef CONFIG_NOC_DMA_STATS_COLLECT
/**
 * @brief sysctl node setup
 */

static int noc_dma_sysctl_setup(void)
{
	struct sysobj *sobj;


	sobj = sysobj_create();
	if (!sobj)
		return -ENOMEM;

	sobj->sattr = noc_dma_attributes;
	sysobj_add(sobj, NULL, driver_set, "noc_dma");

	return 0;
}
#endif /* CONFIG_NOC_DMA_STATS_COLLECT */


/**
 * @brief driver cleanup function
 */

static void noc_dma_exit(void)
{
	printk(MSG "module_exit() not implemented\n");
}


/**
 * @brief driver initialisation
 */

static int noc_dma_init(void)
{
	unsigned int i;



	pr_info(MSG "initialising\n");


	INIT_LIST_HEAD(&dma_channel_pool_head);
	INIT_LIST_HEAD(&dma_transfer_pool_head);
	INIT_LIST_HEAD(&dma_transfer_queue_head);

	for(i = 0; i < NOC_DMA_CHANNELS; i++) {

		dma_channel_pool[i].channel = &noc_dma->chan[i];
		dma_channel_pool[i].irl     = NOC_GET_DMA_IRL(i);

		list_add_tail(&dma_channel_pool[i].node,
			      &dma_channel_pool_head);

#ifdef CONFIG_SSDP
#warning "SSDP DMA channel IRQ numbers unknown, using MPPB as baseline"
#endif
		/* DMA EIRQ numbers are 0...NOC_DMA_CHANNELS */
		irq_request(LEON_WANT_EIRQ(i), ISR_PRIORITY_NOW,
			    &noc_dma_service_irq_handler,
			    &dma_channel_pool[i]);
	}

	for (i = 0; i < NOC_DMA_TRANSFER_QUEUE_SIZE; i++) {
		list_add_tail(&dma_transfer_pool[i].node,
			      &dma_transfer_pool_head);
	}

#ifdef CONFIG_NOC_DMA_STATS_COLLECT
	if (noc_dma_sysctl_setup())
		return -1;
#endif

	return 0;
}

module_init(noc_dma_init);
module_exit(noc_dma_exit);


/* XXX define_initcall(noc_dma_init); */


/**
 * @brief reserve a NoC DMA channel for external use
 */

struct noc_dma_channel *noc_dma_reserve_channel(void)
{
	return noc_dma_request_channel(NULL);
}
EXPORT_SYMBOL(noc_dma_reserve_channel);


/**
 * @brief return a reserved channel
 */

void noc_dma_release_channel(struct noc_dma_channel *c)
{
	struct noc_dma_channel_node *n;

	if (!c)
		return;

	n = container_of((struct noc_dma_channel **) &c,
			 struct noc_dma_channel_node, channel);

	noc_dma_release_channel_node(n);
}
EXPORT_SYMBOL(noc_dma_release_channel);


/**
 * @brief request an arbitrary DMA transfer
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
 * @param callback a user-supplied callback function, executed on successful
 *	           transfer
 * @param userdata a pointer to arbitrary userdata, passed to callback function
 */

int noc_dma_req_xfer(void *src, void *dst, uint16_t x_elem, uint16_t y_elem,
		     enum noc_dma_elem_size elem_size,
		     int16_t x_stride_src, int16_t x_stride_dst,
		     int16_t y_stride_src, int16_t y_stride_dst,
		     enum noc_dma_priority dma_priority, uint16_t mtu,
		     int (*callback)(void *), void *userdata)
{
	struct noc_dma_transfer *t;



	if (!src)
		return -EINVAL;

	if (!dst)
		return -EINVAL;

	if(!x_elem)
		return -EINVAL;

	if(!y_elem)
		return -EINVAL;

#ifdef CONFIG_NOC_DMA_SAME_MASTER_TRANSFER_WORKAROUND
	if (((int) dst & 0xF0000000) == ((int) src & 0xF0000000)) {
		int ret;

		/* at least catch that -.- */
		if (((int) src & 0xF0000000) ==
		    (NOC_SCRATCH_BUFFER_BASE & 0xF0000000))
			return -EINVAL;

		pr_crit(MSG "Transfer withing same memory requested, enabling "
			    "workaround via scratch buffer. The result of this "
			    "transfer may not be as desired. You have been "
			    "warned.\n");
		ret = noc_dma_req_xfer(src, (void *) NOC_SCRATCH_BUFFER_BASE,
				       x_elem, y_elem, elem_size,
				       x_stride_src, x_stride_dst,
				       y_stride_src, y_stride_dst,
				       HIGH, mtu, NULL, NULL);
		if (ret)
		    return ret;

		/* this would actually work fine, if we had enough scratch
		 * buffer for all 8 channels or at least some management of it
		 * let's mark this as TODO
		 */
		src = (void *) NOC_SCRATCH_BUFFER_BASE;
		dma_priority = LOW;
	}
#endif

	t = noc_dma_get_transfer_slot();
	if(!t)
		return -EBUSY;

	t->src = src;
	t->dst = dst;

	t->x_elem = x_elem;
	t->y_elem = y_elem;

	t->elem_size = elem_size;

	t->x_stride_src = x_stride_src;
	t->x_stride_dst = x_stride_dst;

	t->y_stride_src = y_stride_src;
	t->y_stride_dst = y_stride_dst;

	t->mtu = mtu;

	/* transfers registered here will be auto-released */
	t->irq_fwd = IN;

	t->priority = dma_priority;

	t->callback = callback;
	t->userdata = userdata;

	noc_dma_register_transfer(t);


	return 0;
}
EXPORT_SYMBOL(noc_dma_req_xfer);


/**
 * @brief request a linear array DMA transfer
 *
 * @param src  the source address
 * @param dst the destination address
 *
 * @param elem the number of elements
 * @param size the element size (BYTE, HALFWORD, WORD, DOUBLEWORD)
 *
 * @param mtu the maximum transfer unit of a NoC packet
 *
 * @param callback a user-supplied callback function, executed on successful
 *	           transfer
 * @param userdata a pointer to arbitrary userdata, passed to callback function
 */

int noc_dma_req_lin_xfer(void *src, void *dst,
			 uint16_t elem, enum noc_dma_elem_size elem_size,
			 enum noc_dma_priority dma_priority, uint16_t mtu,
			 int (*callback)(void *), void *userdata)
{
	return noc_dma_req_xfer(src, dst, elem, 1, elem_size,
				1, 1, 1, 1, LOW, mtu, callback, userdata);
}
EXPORT_SYMBOL(noc_dma_req_lin_xfer);
