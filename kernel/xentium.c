/**
 * @file kernel/xentium.c
 *
 * TODO This is still quite a mess...
 *
 * TODO Currently, parallel nodes are not supported unless multiple kernels
 *	of the same type are loaded. We can fix that if we link the kernel
 *	object code ourselves, since xentium-clang cannot currently produce
 *	relocatable executables.
 *
 * TODO We either need a custom allocator or some kind of physical address
 *	(i.e. DMA-capable) solution for all memory that is used during
 *	Xentium processing. This obviously includes the memory where
 *	the actual task data is stored. Currently, this is not an issue, because
 *	the MPPB(v2) does not have an MMU, but the SSDP probably will.
 *
 *
 * TODO there's an issue with the xentium apparently raising multiple irqs on
 * exit
 *
 */

#ifdef CONFIG_MMU
#error "MMU kernels require a DMA-comptible allocation scheme"
#endif


#include <kernel/printk.h>
#include <kernel/err.h>
#include <kernel/xentium.h>
#include <kernel/module.h>
#include <kernel/kmem.h>
#include <kernel/kernel.h>
#include <kernel/irq.h>
#include <asm/irq.h>

#include <asm-generic/swab.h>
#include <kernel/string.h>
#include <elf.h>



#define MSG "XEN: "



/* XXX Argh...not so great. If it's possible to determine the platform's
 * Xentium configuration at runtime from AMBA PnP at some point, implement that.
 */

#define XEN_0_EIRQ	30
#define XEN_1_EIRQ	31

#define XEN_BASE_0	0x20000000
#define XEN_BASE_1	0x20100000

#define XENTIUMS	2

/* this is where we keep track of loaded kernels and Xentium activity */
static struct {
	struct proc_net *pn;
	struct xen_kernel **x;
	struct xen_kernel_cfg **cfg;
	int   *kernel_in_use;
	int    sz;
	int    cnt;

	int pt_pending;

	struct proc_tracker *pt[XENTIUMS];
	struct xen_dev_mem *dev[XENTIUMS];

} _xen = {.dev  = {(struct xen_dev_mem *) (XEN_BASE_0 + XEN_DEV_OFFSET),
		   (struct xen_dev_mem *) (XEN_BASE_1 + XEN_DEV_OFFSET)}
	  };
;

/* if we need to expand space for more kernels, this is how many entries we will
 * add per reallocation attempt
 */
#define KERNEL_REALLOC 10


static int xentium_init(void);


static int xen_idx_from_irl(int irq)
{
	return irq - XEN_0_EIRQ;
}

/**
 * @brief find an unused Xentium
 *
 * @return -1 on error, other index of free Xentium in system
 */

static int xen_get_idx_unused(void)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(_xen.pt); i++) {
		if (!_xen.pt[i])
			return i;
	}

	return -1;
}


/*
 * @brief check if a tracker node is already processing
 *
 * @return 1 if processing, 0 otherwise
 */

static int xen_node_is_processing(struct proc_tracker *pt)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(_xen.pt); i++) {
		if (_xen.pt[i] == pt)
			return 1;
	}

	return 0;
}

/**
 * @brief see if a task is pending
 *
 * @returns 0 if not pending
 */

static int xen_get_tracker_pending(void)
{
	return _xen.pt_pending;
}


/**
 * @brief set pending task flag
 */

static void xen_set_tracker_pending(void)
{
	_xen.pt_pending = 1;
}


/**
 * @brief clear pending task flag
 */

static void xen_clear_tracker_pending(void)
{
	_xen.pt_pending = 0;
}

/**
 * @brief set the current processing task node active in a xentium
 * @param idx the index of the xentium
 * @param pt the processing task (set NULL to mark a Xentium as unused)
 */

static void xen_set_tracker(size_t idx, struct proc_tracker *pt)
{
	if (idx < ARRAY_SIZE(_xen.pt))
		_xen.pt[idx] = pt;
}

/**
 * @brief get the current processing task node active in a xentium
 * @param idx the index of the xentium
 * @param pt the processing task (NULL if unused)
 */

static struct proc_tracker *xen_get_tracker(size_t idx)
{
	if (idx < ARRAY_SIZE(_xen.pt))
		return _xen.pt[idx];

	return NULL;
}

/**
 * @brief check if a kernel is in use
 *
 * @param idx the index of the kernel
 *
 * @returns 0 if free, bits set if busy or index out of range
 */

static int xen_kernel_busy(size_t idx)
{
	if (idx < _xen.cnt)
		return _xen.kernel_in_use[idx];

	return 1;
}


/**
 * @brief set a kernel's is in-use flag
 * @param idx the index of the kernel
 */

static void xen_kernel_set_busy(size_t idx)
{
	if (idx < _xen.cnt)
		 _xen.kernel_in_use[idx] = 1;
}

/**
 * @brief clear a kernel's is in-use flag
 *
 * @param idx the index of the kernel
 */

static void xen_kernel_set_unused(size_t idx)
{
	if (idx < _xen.cnt)
		 _xen.kernel_in_use[idx] = 0;
}


/**
 * @brief get the index of a kernel matching an op code
 *
 * @param op_code the op code to look for
 *
 * @return -ENOENT if no matching op code,
 *	   -EBUSY if kernel is busy,
 *	   otherwise index of kernel
 */

static int xen_get_kernel_idx_with_op_code(unsigned long op_code)
{
	size_t i;

	int busy = 0;


	for (i = 0; i < _xen.cnt; i++) {
		if (_xen.cfg[i]->op_code == op_code) {
			if (!xen_kernel_busy(i))
				return i;
			else
				busy = 1;	/* found one, but busy */
		}
	}

	if (busy)
		return -EBUSY;

	return -ENOENT;
}


/**
 * @brief get the Xentium device structure for a Xentium device index
 *
 * @param idx the Xentium device index
 *
 * @return a pointer to the Xentium device structure or NULL if not found
 */

static struct xen_dev_mem *xen_get_device(int idx)
{
	if (idx < ARRAY_SIZE(_xen.dev))
		return _xen.dev[idx];

	return NULL;
}


/**
 * @brief get the Xentium kernel reference for a particular index
 *
 * @param idx the kernel index
 *
 * @return a pointer to the Xentium kernel structure or NULL if not found
 */

static struct xen_kernel *xen_get_kernel(int idx)
{
	if (idx < _xen.cnt)
		return _xen.x[idx];

	return NULL;
}


/**
 * @brief set the entry point mailbox to load a xentium program
 *
 * @param xen a Xentium device
 * @param ep the entry point address
 *
 */

static void xen_set_ep(struct xen_dev_mem *xen, unsigned long ep)
{
	if (!xen)
		return;

	xen->mbox[XEN_EP_MBOX] = ep;
}


/**
 * @brief set the message mailbox with the address of the data message
 *
 * @param xen a Xentium device
 * @param m a data message
 */

static void xen_set_msg(struct xen_dev_mem *xen, struct xen_msg_data *m)
{
	if (!xen)
		return;

	if (!m)
		return;

	xen->mbox[XEN_MSG_MBOX] = (unsigned long) m;
}


/**
 * @brief get the message mailbox with the address of the data message
 *
 * @param xen a Xentium device
 *
 * @returns the data message
 */

static struct xen_msg_data *xen_get_msg(struct xen_dev_mem *xen)
{
	if (!xen)
		return NULL;

	return (struct xen_msg_data *) xen->mbox[XEN_MSG_MBOX];
}

/**
 *
 * @brief load and start a kernel with a particular op code
 *
 * @note this is the "op func" called by pn_process_next() we would use to start
 *	 and wait for kernels to run if we had threads but until then we need to
 *	 do our own step-by-step processing
 */

static int op_xen_schedule_kernel(unsigned long op_code, struct proc_task *t)
{
	pr_err(MSG "Not implemented\n");

	return PN_TASK_SUCCESS;
}


/**
 * @brief load a kernel to process a tracker node
 *
 * @note as a reminder: the pending step in process tracker always matches the
 *	 op code of that tracker, unless there's a bug
 */

static int xen_load_task(struct proc_tracker *pt)
{
	int k_idx;
	int x_idx;

	unsigned long op_code;

	struct xen_kernel   *k;
	struct xen_dev_mem  *xen;
	struct xen_msg_data *m;


	if (!pt)
		return -EINVAL;

	x_idx = xen_get_idx_unused();
	if (x_idx < 0)
		return -EBUSY;

	/* allocate new message data to pass to the Xentium */
	m = (struct xen_msg_data *) kzalloc(sizeof(struct xen_msg_data));
	if (!m) {
		pr_err(MSG "Cannot allocate message data memory, "
			    "rescheduling\n");
		return -ENOMEM;
	}

	m->t = pn_get_next_pending_task(pt);
	if (!m->t) {
		kfree(m);
		return -ENOENT;
	}

	/**
	 * @note op code checks should also be done in the Xentium kernel,
	 * if a non-matching task step ended up in the wrong kernel, it can
	 * just return TASK_SUCCESS and the task will be moved to the matching
	 * node in the network
	 */
	op_code = pt_get_pend_step_op_code(m->t);

	k_idx = xen_get_kernel_idx_with_op_code(op_code);
	if (k_idx < 0) {
		kfree(m);
		return k_idx;
	}

	xen = xen_get_device(x_idx);
	BUG_ON(!xen);

	k = xen_get_kernel(k_idx);
	BUG_ON(!k);


	printk(MSG "Starting xentium %d with ep %x (%s)\n",
		    x_idx, k->ep, _xen.cfg[k_idx]->name);

	xen_set_tracker(x_idx, pt);

	xen_set_ep(xen, k->ep);

	m->xen_id = x_idx;
	xen_set_msg(xen, m);

	return 0;
}


static void xen_handle_cmds(int x_idx, struct xen_msg_data *m, int pn_ret_code)
{
	int ret = 0;

	struct xen_dev_mem *xen;
	struct proc_tracker *pt;


	if (!m)
		return;

	xen = xen_get_device(x_idx);
	BUG_ON(!xen);

	pt = xen_get_tracker(x_idx);
	
	if (pt)
		ret = pn_eval_task_status(_xen.pn, pt, m->t, pn_ret_code);

	/* abort */
	if (!ret) {
		pr_debug(MSG "Task aborted.\n");
		kfree(m);
		xen_set_tracker(x_idx, NULL);
		xentium_schedule_next();
		return;
	}


	if (xen_get_tracker_pending()) {
		pr_debug(MSG "Pending tracker, commanding abort.\n");
		m->cmd = TASK_STOP;
		xen_set_msg(xen, m);
		return;
	}

	m->t = pn_get_next_pending_task(pt);

	if (!m->t) {
		pr_debug(MSG "No more tasks, commanding abort.\n");
		m->cmd = TASK_STOP;
	} else {
		pr_debug(MSG "Next task\n");
	}

	/* signal next */
	xen_set_msg(xen, m);

}




/**
 * @brief handle Xentium interrupts
 */

static irqreturn_t xen_irq_handler(unsigned int irq, void *userdata)
{
	int x_idx;

	struct xen_dev_mem *xen;

	struct xen_msg_data *m;




	x_idx = xen_idx_from_irl(irq);
	xen = xen_get_device(x_idx);
	BUG_ON(!xen);


	/* If this is invalid, the Xentium must have failed somehow.
	 * In the MPPBv2, the Xentiums apparently cannot be reset, so
	 * we just bail, because there's nothing we can do at this point
	 */
	m = xen_get_msg(xen);

	if (!m) {
		pr_err(MSG "invalid message pointer received, ignoring.");
		return IRQ_HANDLED;
	}

	printk(MSG "Interrupt from Xentium %d, sequence number %d\n",
	       x_idx, pt_get_seq(m->t));


	switch (m->cmd) {

	case TASK_SUCCESS:
		/* Step complete, go to next one. If the same step was scheduled
		 * multiple times in a task, it end up back in the tracker all
		 * by itself
		 */
		printk(MSG "TASK_SUCCESS\n");
		xen_handle_cmds(x_idx, m, PN_TASK_SUCCESS);
		break;

	case TASK_STOP:
		/* task processing stopped */
		printk(MSG "TASK_STOP\n");
		xen_handle_cmds(x_idx, m, PN_TASK_STOP);
		break;

	case TASK_DETACH:
		/* task tracking is done in kernel */
		printk(MSG "TASK_DETACH\n");
		xen_handle_cmds(x_idx, m, PN_TASK_STOP);
		break;

	case TASK_RESCHED:
		/* reschedule task and abort  */
		printk(MSG "TASK_RESCHED\n");
		xen_handle_cmds(x_idx, m, PN_TASK_RESCHED);
		break;

	case TASK_SORTSEQ:
		/* reschedule task, sort pending tasks by sequence and abort */
		printk(MSG "TASK_SORTSEQ\n");
		xen_handle_cmds(x_idx, m, PN_TASK_SORTSEQ);
		break;

	case TASK_DESTROY:
		/* destroy this task */
		printk(MSG "TASK_DESTROY\n");
		xen_handle_cmds(x_idx, m, PN_TASK_DESTROY);
		break;

	case TASK_NEW:
		/* create a new task an pass it to the xentium */
		pr_crit(MSG "TASK_NEW: unimplemented\n");
		BUG();
		break;

	case TASK_DATA_REALLOC:
		pr_debug(MSG "TASK_DATA_REALLOC: %d bytes\n", m->cmd_param);

		/* TODO: this should make use of a custom allocator, see note
		 * on top of file
		 */

		/* if realloc() fails, it's up to the Xentium to detect it */
		m->t->data = krealloc(m->t->data, m->cmd_param);

		/* signal back */
		xen_set_msg(xen, m);
		break;

	default:
		pr_err(MSG "unknown command received %d\n", m->cmd);
		/* Ignore. Would reset Xentium here */
		break;
	};


	return IRQ_HANDLED;
}









/**
 * @brief setup the module structure
 *
 * @return -1 on error
 */

static int xentium_setup_kernel(struct xen_kernel *m)
{
	int i;

	Elf_Shdr *shdr;


	/* initialise module configuration */
	m->size     = 0;
	m->align    = sizeof(void *);
	m->sec      = NULL;
	m->ep       = m->ehdr->e_entry;


	for (i = 0; i < m->ehdr->e_shnum; i++) {

		shdr = elf_get_sec_by_idx(m->ehdr, i);
		if (!shdr)
			return -1;

		if (shdr->sh_flags & SHF_ALLOC) {
			pr_debug(MSG "found alloc section: %s, size %ld\n",
			       elf_get_shstrtab_str(m->ehdr, i),
			       shdr->sh_size);

			m->size += shdr->sh_size;

			if (shdr->sh_addralign > m->align) {
				m->align = shdr->sh_addralign;
				pr_debug(MSG "align: %d\n", m->align);
			}
		}
	}

	return 0;
}



/**
 * @brief check if this is a valid Xentium ELF binary
 * @note this requires elf32-big instead of elf32-xentium in linker script
 *	 target
 */

int xentium_elf_header_check(Elf_Ehdr *ehdr)
{
	if (ehdr->e_ident[EI_DATA] != ELFDATA2MSB) {
		pr_debug(MSG "swapping kernel ELF header endianess");
		elf_hdr_endianess_swap(ehdr);
	}

	if (ehdr->e_shentsize != sizeof(Elf_Shdr))
		return -1;

	if (!(ehdr->e_ident[EI_MAG0] == ELFMAG0))
		return -1;

	if (!(ehdr->e_ident[EI_MAG1] == ELFMAG1))
		return -1;

	if (!(ehdr->e_ident[EI_MAG2] == ELFMAG2))
		return -1;

	if (!(ehdr->e_ident[EI_MAG3] == ELFMAG3))
		return -1;

	if (!(ehdr->e_ident[EI_CLASS] == ELFCLASS32))
		if (!(ehdr->e_ident[EI_CLASS] == ELFCLASS64))
			return -1;


	if (!(ehdr->e_ident[EI_VERSION] == EV_CURRENT))
		return -1;

#if 0
	/* the Xentium toolchain cannot produce files with relocations,
	 * which is very sad...
	 */
	if (!(ehdr->e_type == ET_REL)) {
		return -1;
	}

	if(!ehdr->e_machine != XXX)
		return -1;
#endif

	if (!(ehdr->e_version == EV_CURRENT))
		return -1;

	return 0;
}


/**
 * @brief load a xentium kernel
 *
 * @note since xentium programs are not relocatable, this just dumps
 * them wherever they say they want to be. The user is responsible for the
 * proper memory configuration and offsets, I really can't do much about that at
 * the moment, because I simply do not have the time to just work with the
 * object files and do all relocations by hand, because that would also require
 * me to take care of the Xentium's runtime wrapper code, which again would take
 * more time that I can currently spare, but this would be the preferred
 * solution.
 */

static int xentium_load_kernel(struct xen_kernel *x)
{
	unsigned int idx = 0;

	char *src;

	Elf_Shdr *sec;

	struct xen_kern_section *s;

	uint32_t *p;

	int i;


	pr_debug(MSG "\n" MSG "\n"
	       MSG "Loading kernel run-time sections\n");

	x->num_sec = elf_get_num_alloc_sections(x->ehdr);
	x->sec = (struct xen_kern_section *)
		kcalloc(sizeof(struct xen_kern_section), x->num_sec);

	if (!x->sec)
		goto error;

	s = x->sec;

	while (1) {

		idx = elf_find_shdr_alloc_idx(x->ehdr, idx + 1);


		if (!idx)
			break;

		sec = elf_get_sec_by_idx(x->ehdr, idx);

		if (!sec->sh_size)	/* don't need those */
			continue;

		s->size = sec->sh_size;

		src = elf_get_shstrtab_str(x->ehdr, idx);
		s->name = kmalloc(strlen(src));

		if (!s->name)
			goto error;

		strcpy(s->name, src);

		if (sec->sh_type & SHT_NOBITS) {
			pr_debug(MSG "\tZero segment %10s at %p size %ld\n",
			       s->name, (char *) sec->sh_addr,
			       sec->sh_size);

			bzero((void *) sec->sh_addr, s->size);
		} else {
			pr_debug(MSG "\tcopy segment %10s from %p to %p size %ld\n",
			       s->name,
			       (char *) x->ehdr + sec->sh_offset,
			       (char *) sec->sh_addr,
			       sec->sh_size);

			memcpy((void *) sec->sh_addr,
			       (char *) x->ehdr + sec->sh_offset,
			       sec->sh_size);


			/* we byte-swap all loadable sections, because the DMA
			 * of the Xentium reverses everything back into little
			 * endian words
			 */

			p = (uint32_t *) sec->sh_addr;
			for (i = 0; i < sec->sh_size / sizeof(uint32_t); i++)
				p[i] = swab32(p[i]);



		}

		s->addr = sec->sh_addr;

		s++;

		if (s > &x->sec[x->num_sec]) {
			pr_debug(MSG "Error out of section memory\n");
			goto error;
		}
	}

	return 0;

error:
	if (x->sec)
		for (idx = 0; idx < x->num_sec; idx++)
			kfree(x->sec[idx].name);

	kfree(x->sec);

	return -ENOMEM;
}



/**
 * load the kernels configuration data
 */

struct xen_kernel_cfg *xentium_config_kernel(struct xen_kernel *x)
{
	unsigned long symaddr;

	struct xen_kernel_cfg *cfg;
	size_t len = 0;
	uint32_t *p;

	char *name;

	if (!elf_get_symbol_value(x->ehdr, "_xen_kernel_param", &symaddr)) {
		pr_warn(MSG "Error, _xen_kernel_param not found\n");
		return NULL;
	}



	cfg = (struct xen_kernel_cfg *) kzalloc(sizeof(struct xen_kernel_cfg));
	if (!cfg)
		return NULL;


	memcpy(cfg, (void *) symaddr, sizeof(struct xen_kernel));


	/* if the kernel needs permanent storage for internal use, allocate it
	 * here and overwrite the configuration structure at the kernel's
	 * location
	 */

	if (cfg->size) {
		cfg->data = kzalloc(cfg->size);
		if (!cfg->data) {
			kfree(cfg);
			return NULL;
		}
		pr_debug (MSG "allocated kernel data storage of %d bytes "
			      "at %p\n", cfg->size, cfg->data);
		memcpy((void *) symaddr, cfg, sizeof(struct xen_kernel));
	}



	/* everything but the "name" entry (which is mashed up from our
	 * perspective) is already in correct (big endian) order.
	 * Since the string stored in .rodata is aligned to 4 bytes,
	 * we'll locate the first occurence of '\0', round up to the next word
	 * boundary, then swab32() on the buffer to restore the string.
	 * The Xentium DMA will reverse endianess to little endian on the
	 * relevant run time sections, we don't need to care about that.
	 *
	 * yes, this may "leak" a few (3) bytes, so don't store plain text
	 * passwords there :D :D
	 * XXX: alloc second char buffer of actual string size, then strncpy()
	 *      and free the initial one (or just krealloc() >:-))
	 */

	while (cfg->name[len] != '\0') len++;

	len = ALIGN(len + sizeof(uint32_t), sizeof(uint32_t));

	name = kmalloc(len);
	if (!name) {
		kfree(cfg);
		return NULL;
	}

	p = (uint32_t *) name;

	memcpy(name, cfg->name, len);

	len = len / sizeof(uint32_t);

	do {
		len--;
		p[len] = swab32(p[len]);
	} while (len);

	cfg->name = name;

	pr_info(MSG "Configuration of kernel %s:\n"
	        MSG "\top code     :          %x\n"
	        MSG "\tcritical buffer level: %d\n",
	        cfg->name, cfg->op_code, cfg->crit_buf_lvl);


	return cfg;
}


int xentium_kernel_add(void *p)
{
	struct xen_kernel *x;
	struct xen_kernel_cfg *cfg = NULL;
	struct proc_tracker *pt = NULL;


	xentium_init();	/* XXX */

	x = (struct xen_kernel *) kzalloc(sizeof(struct xen_kernel));
	if (!x)
		goto error;


	/* the ELF binary starts with the ELF header */
	x->ehdr = (Elf_Ehdr *) p;

	pr_debug(MSG "Checking ELF header\n");



	if (xentium_elf_header_check(x->ehdr))
		goto error;

	pr_debug(MSG "Setting up module configuration\n");

	if (xentium_setup_kernel(x))
		goto error;

	if (xentium_load_kernel(x))
		goto cleanup;


	cfg = xentium_config_kernel(x);
	if (!cfg)
		goto cleanup;

	x->ehdr = NULL;	/* not used anymore */




	pt = pt_track_create(op_xen_schedule_kernel,
			     cfg->op_code,
			     cfg->crit_buf_lvl);
	if (!pt)
		goto cleanup;

	if (pn_add_node(_xen.pn, pt))
		goto cleanup;


	if (_xen.cnt == _xen.sz) {
		_xen.x = krealloc(_xen.x, (_xen.sz + KERNEL_REALLOC) *
				   sizeof(struct xen_kernel **));
		_xen.cfg = krealloc(_xen.cfg, (_xen.sz + KERNEL_REALLOC) *
				    sizeof(struct xen_kernel_cfg **));

		_xen.kernel_in_use = krealloc(_xen.kernel_in_use,
				      (_xen.sz + KERNEL_REALLOC) * sizeof(int));

		bzero(&_xen.x[_xen.sz], sizeof(struct xen_kernel **) *
		      KERNEL_REALLOC);

		bzero(&_xen.cfg[_xen.sz], sizeof(struct xen_kernel_cfg **) *
		      KERNEL_REALLOC);

		bzero(&_xen.kernel_in_use[_xen.sz], sizeof(int) * KERNEL_REALLOC);

		_xen.sz += KERNEL_REALLOC;
	}

	_xen.x[_xen.cnt] = x;
	_xen.cfg[_xen.cnt] = cfg;

	_xen.cnt++;



	return 0;
cleanup:
	pr_err("cleanup\n");
	kfree(x);
	kfree(cfg);
	pt_track_destroy(pt);
#if 0
	xentium_kernel_unload(m);
#endif
error:
	pr_err("error\n");
	return -1;
}


/**
 * define the output for the network
 */

int xentium_config_output_node(op_func_t op_output)
{
	if (pn_create_output_node(_xen.pn, op_output))
		return -1;

	return 0;
}


/**
 * @brief add a new task to the network
 */

void xentium_input_task(struct proc_task *t)
{
	pn_input_task(_xen.pn, t);
	pn_process_inputs(_xen.pn);

	/* try to poke processing, in case it stopped */
	xentium_schedule_next();
}


/**
 * @brief schedule a processing cycle
 */

void xentium_schedule_next(void)
{
	int ret;

	struct proc_tracker *pt;


	/* move critical trackers to top of queue */
	pn_queue_critical_trackers(_xen.pn);

	pt = pn_get_next_pending_tracker(_xen.pn);
	if (!pt)
		return;

	/* tracker node is already processing */
	if (xen_node_is_processing(pt))
		return;

	/* try to load new node for processing */
	ret = xen_load_task(pt);

	switch (ret) {
	case -EBUSY:
		/* mark new task */
		xen_set_tracker_pending();
		break;
	default:	/* XXX other retvals ignored */
		xen_clear_tracker_pending();
		break;
	}

}


/**
 * @brief process the outputs of the network
 */

void xentium_output_tasks(void)
{
	pn_process_outputs(_xen.pn);
}


/**
 * @brief driver cleanup function
 */

static int xentium_exit(void)
{
	printk(MSG "module_exit() not implemented\n");

	return 0;
}


/**
 * @brief driver initialisation
 */

static int xentium_init(void)
{


	if (!_xen.pn) {
		pr_info(MSG "initialising\n");
	       _xen.pn = pn_create();

		if (irq_request(LEON_WANT_EIRQ(XEN_0_EIRQ), ISR_PRIORITY_NOW,
			    &xen_irq_handler, NULL)) {
			pr_err(MSG "Cannot register interrupt handler!\n");
		}

		if (irq_request(LEON_WANT_EIRQ(XEN_1_EIRQ), ISR_PRIORITY_NOW,
			    &xen_irq_handler, NULL)) {
			pr_err(MSG "Cannot register interrupt handler!\n");
		}
	}

	if (!_xen.pn)
		return -ENOMEM;


	return 0;
}

module_init(xentium_init);
module_exit(xentium_exit);
