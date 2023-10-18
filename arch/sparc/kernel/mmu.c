/**
 * @file arch/sparc/kernel/mmu.c
 *
 * @ingroup sparc
 *
 * @brief SPARC MMU initialisation, context, trap handling and sbrk()
 *
 */

#include <mm.h>

#include <kernel/kernel.h>
#include <kernel/printk.h>

#include <page.h>
#include <mmu.h>
#include <srmmu.h>
#include <srmmu_access.h>
#include <traps.h>
#include <cpu_type.h>
#include <errno.h>
#include <stack.h>
#include <asm/io.h>


/**
 * for now, this is our primitive per-process (we really have only one...)
 * system break tracker
 *
 * sbrk: the current program break
 * addr_low the legal lower boundary
 * addr_hi the legal upper boundary
 *
 * note: we don't have user/kernel memory segmentation yet
 */

static struct mm_sbrk {
	unsigned long sbrk;

	unsigned long addr_lo;
	unsigned long addr_hi;
} mm_proc_mem [SRMMU_CONTEXTS];

static unsigned long _mmu_ctx;



/* XXX: dummy, move out of here */
enum sparc_cpus sparc_cpu_model;



/**
 * @brief if the SRMMU is not supported
 */

static void mm_srmmu_is_bad(void)
{
        pr_emerg("MM: No supported SRMMU type found.\n");
        pr_emerg("MM: SRMMU CTRL 0x%08x VER: %d IMPL %d\n",
		 srmmu_get_mmu_ctrl(),
		 srmmu_get_mmu_ver(),
		 srmmu_get_mmu_impl());
	BUG();
}


/**
 * @brief configure MMU on a LEON
 *
 * TODO stuff...
 */

static void mm_init_leon(void)
{
	trap_handler_install(0x9, data_access_exception_trap);
	srmmu_init(&bootmem_alloc, &bootmem_free);

	/* 1:1 map full range of highmem */
	srmmu_do_large_mapping_range(0, HIGHMEM_START, HIGHMEM_START,
				(0xFFFFFFFF - HIGHMEM_START) /
				SRMMU_LARGE_PAGE_SIZE + 1,
				(SRMMU_CACHEABLE | SRMMU_ACC_S_RWX_2));

	mm_set_mmu_ctx(0);

	srmmu_enable_mmu();
}


/**
 * @brief probe for supported mmu type
 */

static void get_srmmu_type(void)
{
	/* we only support the GRLIB SRMMU, its implementation and version
	 * fields are set to 0, so we only check for a matching CPU model
	 * else we assume a bad SRMMU
	 */

	if (srmmu_get_mmu_ver())	/* GR712RC == 0 */
		goto bad;

	if (srmmu_get_mmu_impl())	/* GR712RC == 0 */
		goto bad;

	if (sparc_cpu_model == sparc_leon) {
		mm_init_leon();
		return;
	}

bad:
	mm_srmmu_is_bad();
}


/**
 * @brief load MMU support
 * @note this is called from setup_arch() for LEON
 */

static void mm_load_mmu(void)
{
	get_srmmu_type();

	/* XXX: we also might want to:
	 *	- load TLB operations if needed (for SMP)
	 *	- IOMMU setup if needed
	 *	- initialise SMP
	 */
}


/**
 * @brief set the active MMU context
 *
 * @param ctx the MMU context to set
 *
 * @returns 0 on success, otherwise error
 */

int mm_set_mmu_ctx(unsigned long ctx)
{

	if (srmmu_select_ctx(ctx))
		return -EINVAL;

	/* if necessary, initialise the program break */
	if (!mm_proc_mem[ctx].sbrk) {
		mm_proc_mem[ctx].sbrk    = VMALLOC_START;
		mm_proc_mem[ctx].addr_lo = VMALLOC_START;
		mm_proc_mem[ctx].addr_hi = VMALLOC_END;
	}

	_mmu_ctx = ctx;

	return 0;
}


/**
 * @brief get the active MMU context
 *
 * @returns the active MMU context number
 */

unsigned long mm_get_mmu_ctx(void)
{
	return _mmu_ctx;
}

void mm_release_mmu_mapping(unsigned long va_start, unsigned long va_stop)
{
	unsigned long ctx;


	ctx = mm_get_mmu_ctx();

	srmmu_release_pages(ctx, va_start, va_stop, page_free);
}

void *kernel_sbrk(intptr_t increment)
{
	long brk;
	long oldbrk;

	unsigned long ctx;


	ctx = mm_get_mmu_ctx();

	if (!increment)
		 return (void *) mm_proc_mem[ctx].sbrk;

	oldbrk = mm_proc_mem[ctx].sbrk;

	/* make sure we are always double-word aligned, otherwise we can't
	 * use ldd/std instructions without trapping */
	increment = ALIGN(increment, STACK_ALIGN);

	brk = oldbrk + increment;

	if (brk < mm_proc_mem[ctx].addr_lo)
		return (void *) -1;

	if (brk >  mm_proc_mem[ctx].addr_hi)
		return (void *) -1;

	/* try to release pages if we decremented below a page boundary */
	if (increment < 0) {
		if (PAGE_ALIGN(brk) < PAGE_ALIGN(oldbrk - PAGE_SIZE)) {

			pr_debug("SBRK: release %lx (va %lx, pa%lx) to va %lx pa %lx, %d pages\n",
				 brk, PAGE_ALIGN(brk), virt_to_phys(PAGE_ALIGN(brk)),
				 oldbrk, virt_to_phys(oldbrk),
				 (PAGE_ALIGN(oldbrk) - PAGE_ALIGN(brk)) / PAGE_SIZE);

			if (PAGE_ALIGN(oldbrk) >  PAGE_ALIGN(brk))
				mm_release_mmu_mapping(PAGE_ALIGN(brk), PAGE_ALIGN(oldbrk));
		}
	}

	mm_proc_mem[ctx].sbrk = brk;

	pr_debug("SBRK: moved %08lx -> %08lx\n", oldbrk, brk);

	return (void *) oldbrk;
}


unsigned long mm_get_physical_addr(unsigned long va)
{
	return srmmu_get_pa_page(mm_get_mmu_ctx(), va) | (va & 0xFFFUL);
}


void mm_mmu_trap(void)
{
	unsigned long ctx;

	struct srmmu_fault_status status;
	unsigned int addr;
	unsigned int alloc;
	static int last;

	status = srmmu_get_mmu_fault_status();

	pr_debug("MM: MMU Status:\n"
	       "MM: ===========\n");

	pr_debug("MM:\tAccess type: ");
	switch(status.access_type)  {
	case SRMMU_FLT_AT_R__U_DATA:
		pr_debug("Read from User data\n"); break;
	case SRMMU_FLT_AT_R__S_DATA:
		pr_debug("Read from SuperUser data\n"); break;
	case SRMMU_FLT_AT_RX_U_INST:
		pr_debug("Read/Exec from User instruction\n"); break;
	case SRMMU_FLT_AT_RX_S_INST:
		pr_debug("Read/Exec from SuperUser instruction\n"); break;
	case SRMMU_FLT_AT_W__U_DATA:
		pr_debug("Write to User data\n"); break;
	case SRMMU_FLT_AT_W__S_DATA:
		pr_debug("Write to SuperUser instruction\n"); break;
	case SRMMU_FLT_AT_W__U_INST:
		pr_debug("Write to User instruction\n"); break;
	case SRMMU_FLT_AT_W__S_INST:
		pr_debug("Write to SuperUser instruction\n"); break;
	default:
		pr_debug("Unknown\n"); break;
	}

	pr_debug("MM:\tFault type: ");
	switch(status.fault_type)  {
	case SRMMU_FLT_FT_NONE:
		pr_debug("None\n"); break;
	case SRMMU_FLT_FT_INVALID_ADDR:
		if (status.fault_address_vaild) {

			addr = srmmu_get_mmu_fault_address();

			if (!addr) {
__diag_push();
__diag_ignore(GCC, 7, "-Wframe-address", "we're fully aware that __builtin_return_address can be problematic");
				pr_crit("NULL pointer violation "
					"in call from %p\n",
					__builtin_return_address(1));
__diag_pop();
				BUG();
			}


			pr_debug("Invalid Address/Not mapped: 0x%08x\n",
			       srmmu_get_mmu_fault_address() >> 24);


			ctx = mm_get_mmu_ctx();

			if (addr < mm_proc_mem[ctx].addr_lo) {

__diag_push();
__diag_ignore(GCC, 7, "-Wframe-address", "we're fully aware that __builtin_return_address can be problematic");
				pr_crit("Access violation: RESERVED (0x%08lx) "
					"in call from %p\n",
					addr,
					__caller(1));
__diag_pop();
				BUG();
			}

			if (addr > HIGHMEM_START) {
__diag_push();
__diag_ignore(GCC, 7, "-Wframe-address", "we're fully aware that __builtin_return_address can be problematic");
				pr_crit("Access violation: HIGHMEM (0x%08lx) "
					"in call from %p\n",
					addr,
					__caller(1));
				BUG();
__diag_pop();
			}


			if (addr < mm_proc_mem[ctx].sbrk) {
				alloc = (unsigned long) page_alloc();
				if (!alloc) {
					pr_crit("MM:\t Out of physical memory %lx\n", last);
					BUG();
				}

				last = alloc;

				pr_debug("MM: Allocating page %lx -> %lx\n",
					 addr, (unsigned int) alloc);




				/* XXX for now, set RWX with super use
				 * permissions until we have mprotect()  */
				if(srmmu_do_small_mapping(ctx, addr, alloc, (SRMMU_CACHEABLE | SRMMU_ACC_S_RWX_2)))
					pr_crit("MM: MMU error mapping pa %lx to va %lx\n", (int) alloc, addr);
			} else {
__diag_push();
__diag_ignore(GCC, 7, "-Wframe-address", "we're fully aware that __builtin_return_address can be problematic");
				pr_crit("Access violation: system break "
					"in call from %p for address 0x%lx\n",
					__builtin_return_address(1), addr);
__diag_pop();
				BUG();
			}
		}
		break;
	case SRMMU_FLT_FT_PROTECTION:
		pr_debug("Protection Error\n"); break;
	case SRMMU_FLT_FT_PRIV_VIOLATION:
		pr_debug("Priviledge Violation\n"); break;
	case SRMMU_FLT_FT_TRANSLATION:
		pr_debug("Translation Error\n"); break;
	case SRMMU_FLT_FT_ACCESS_BUS:
		pr_debug("Bus Access Error\n"); break;
	case SRMMU_FLT_FT_INTERNAL:
		pr_debug("Internal Error\n"); break;
	default:
		pr_debug("Unknown\n"); break;
	}

	pr_debug("MM:\tPage table level: %d\n", status.page_table_level);

	if (status.fault_address_vaild)
		pr_debug("MM:\tVirtual address 0x%x", srmmu_get_mmu_fault_address());

	if (status.overwrite)
		pr_debug("\tMultiple errors since last read of status recorded");


	pr_debug("\nMM:\n");

}



/**
 *
 * @brief initialise paging on the SRMMU
 */

void mm_mmu_paging_init(void)
{
	/* XXX: must be configured earlier by reading appropriate register */
	sparc_cpu_model = sparc_leon;
	mm_load_mmu();
}
