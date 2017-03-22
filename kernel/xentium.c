/**
 * @file kernel/xentium.c
 *
 */


#include <kernel/printk.h>
#include <kernel/err.h>
#include <kernel/xentium.h>
#include <kernel/module.h>
#include <kernel/kmem.h>
#include <kernel/kernel.h>
#include <asm-generic/swab.h>
#include <kernel/string.h>
#include <elf.h>

#include <data_proc_net.h>




#define BANK_SIZE               (   8*1024 )
#define TCM_SIZE                (  32*1024 )
#define XEN_MAILBOX_OFFSET      ( 512*1024 )

//=============================================================
// XENTIUM structure
//=============================================================
typedef struct {
	// TCM
	unsigned int tcm[TCM_SIZE/4];
	unsigned int dummy[(XEN_MAILBOX_OFFSET-TCM_SIZE)/4];
	// Status bits + control registers
	unsigned int mlbx[4];      //0x00080000 .. 0x0008000C
	unsigned int signal[8];    //0x00080010 .. 0x0008002C
	unsigned int dma;          //0x00080030 .. 0x00080030
	unsigned int dummy1;       //0x00080034 .. 0x00080034
	unsigned int timer[2];     //0x00080038 .. 0x0008003C
	unsigned int irq;          //0x00080040 .. 0x00080040
	unsigned int dummy2;       //0x00080044 .. 0x00080044
	unsigned int status;       //0x00080048 .. 0x00080048
	unsigned int pc;           //0x0008004C .. 0x0008004C
	unsigned int fsm_state;    //0x00080050 .. 0x00080050
} volatile S_xen;

typedef struct S_xdev{
	/* Status bits + control registers */
	volatile unsigned int mlbx[4];      //0x00080000 .. 0x0008000C
	volatile unsigned int signal[8];    //0x00080010 .. 0x0008002C
	volatile unsigned int dma;          //0x00080030 .. 0x00080030
	volatile unsigned int dummy1;       //0x00080034 .. 0x00080034
	volatile unsigned int timer[2];     //0x00080038 .. 0x0008003C
	volatile unsigned int irq;          //0x00080040 .. 0x00080040
	volatile unsigned int dummy2;       //0x00080044 .. 0x00080044
	volatile unsigned int status;       //0x00080048 .. 0x00080048
	volatile unsigned int pc;           //0x0008004C .. 0x0008004C
	volatile unsigned int fsm_state;    //0x00080050 .. 0x00080050
} S_xdev;

typedef volatile unsigned int S_tcm;
S_xen*       p_xen0                                             = (S_xen*)              (0x20000000);
S_xen*       p_xen1               = (S_xen*)        (0x20100000);



#define MSG "XEN: "


/* if we need more space, this is how many entries we will add */
#define KERNEL_REALLOC 10

/* this is where we keep track of loaded modules */
static struct {
	struct proc_net *pn;
	struct xen_kernel **x;
	struct xen_kernel_cfg **cfg;
	int    sz;
	int    cnt;
} _xen;



/* this is scheduler, but we need to do the manual processing dance
 * until we have threads, remmber:
 * 
 *  * pt = pn_get_next_pending_tracker(pn)
 *
 * while (1) {
 *	t = pn_get_next_pending_task(pt)
 *	ret = pt->op(pt_get_pend_step_op_code(t), t);
 *	if (!pn_eval_task_status(pn, pt, t, ret))
 *		pn_node_to_queue_tail(pn, pt);
 *		abort_processing:
 *	}
 * }
 *
 * etc...
 */

static int op_xen_schedule_kernel(unsigned long op_code, struct proc_task *t)
{
	printk(MSG "scheduling kernel with op code %x\n", op_code);

	return PN_TASK_SUCCESS;
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
	m->refcnt   = 0;
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

	struct xen_module_section *s;

	uint32_t *p;

	int i;


	pr_debug(MSG "\n" MSG "\n"
	       MSG "Loading kernel run-time sections\n");

	x->num_sec = elf_get_num_alloc_sections(x->ehdr);
	x->sec = (struct xen_module_section *)
		kcalloc(sizeof(struct xen_module_section), x->num_sec);

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

	printk(MSG "Configuration of kernel %s:\n"
	       MSG "\top code     :          %x\n"
	       MSG "\tcritical buffer level: %d\n",
	       cfg->name, cfg->op_code, cfg->crit_buf_lvl);


	return cfg;
}

static int xentium_init(void);

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





#if 0
	printk("starting xentium with ep %x\n", x->ep);
	p_xen1->mlbx[0] = x->ep;
#endif


	if (_xen.cnt == _xen.sz) {
		_xen.x = krealloc(_xen.x, (_xen.sz + KERNEL_REALLOC) *
				   sizeof(struct xen_kernel **));
		_xen.cfg = krealloc(_xen.cfg, (_xen.sz + KERNEL_REALLOC) * 
				    sizeof(struct xen_kernel_cfg **));

		bzero(&_xen.x[_xen.sz], sizeof(struct xen_kernel **) *
		      KERNEL_REALLOC);
		
		bzero(&_xen.cfg[_xen.sz], sizeof(struct xen_kernel_cfg **) *
		      KERNEL_REALLOC);

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
 * @brief add a new to the network
 */

void xentium_input_task(struct proc_task *t)
{
	pn_input_task(_xen.pn, t);
	pn_process_inputs(_xen.pn);
}


/**
 * @brief run a processing cycle 
 */

void xentium_schedule_next(void)
{
	pn_process_next(_xen.pn);

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
		printk(MSG "initialising\n");
	       _xen.pn = pn_create();
	}

	if (!_xen.pn)
		return -ENOMEM;


	return 0;
}

module_init(xentium_init);
module_exit(xentium_exit);
