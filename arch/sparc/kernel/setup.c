/**
 * @file arch/sparc/kernel/setup.c
 */

#include <string.h> /* memset() */

#include <init.h>
#include <mm.h>
#include <compiler.h>

#include <page.h>
#include <stack.h>
#include <kernel/kmem.h>

void *_kernel_stack_top;
void *_kernel_stack_bottom;


/**
 * @brief reserve a stack area for the kernel
 *
 * @warn Since we allocate the kernel stack using kmalloc instead of placing it
 *	 in a custom area, there is a real change of violating the bottom
 *	 boundary, so make sure you allocate enough pages to fit your needs.
 *	 Note that since kmalloc() works via kernel_sbrk() and moving the system
 *	 break does not actually reserve pages until they are accessed, you
 *	 have to initialise the stack area, i.e. actually reserve the pages,
 *	 unless you have a custom interrupt/trap stack. Otherwise the kernel
 *	 cannot perform stack access to an unmapped page, because that would
 *	 require a mapped page...
 *
 * XXX this needs to be addressed at some point, but probably only after we
 *     have a kernel bootstrap implemented and we may define custom reserved
 *     areas more freely.
 */

#warning "Using fixed-size kernel stack"
static void reserve_kernel_stack(void)
{
	const size_t k_stack_sz = KERNEL_STACK_PAGES * PAGE_SIZE;


	/* the bottom of the stack */
	_kernel_stack_bottom = kcalloc(k_stack_sz + STACK_ALIGN, sizeof(char));
	BUG_ON(!_kernel_stack_bottom);

	/* the (aligned) top of the stack */
	_kernel_stack_top = (void *) (char *) _kernel_stack_bottom + k_stack_sz;
	_kernel_stack_top = ALIGN_PTR(_kernel_stack_top, STACK_ALIGN);
}


/**
 * @brief configure available memory banks
 *
 * TODO the memory layout should either be presented in a separate
 *	board configuration file or, preferably, be derived from an AMBA
 *	bus scan.
 */

static void mem_init(void)
{
	sp_banks[0].base_addr = 0x40000000;
	sp_banks[0].num_bytes = 0x00800000;

#if (SPARC_PHYS_BANKS > 0)
	sp_banks[1].base_addr = 0x60000000;
	sp_banks[1].num_bytes = 0x04000000;
#else
#warning "Configuration error: SPARC_PHYS_BANKS size insufficient."
#endif

}


/**
 * @brief architecture setup entry point
 */

void setup_arch(void)
{
	mem_init();

	paging_init();

	BUG_ON(!kmem_init());

	reserve_kernel_stack();

	BUG_ON(stack_migrate(NULL, _kernel_stack_top));
}
