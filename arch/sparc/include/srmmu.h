/**
 * @brief SPARC Reference (SR) Memory Management Unit (MMU)
 * @author Armin Luntzer (armin.luntzer@univie.ac.at)
 *
 * @see SPARCv8 Architecture Manual for more info
 */

#ifndef _SPARC_SRMMU_H_
#define _SPARC_SRMMU_H_


#include <stddef.h>


/* XXX: not here */
#define __BIG_ENDIAN_BITFIELD __BIG_ENDIAN_BITFIELD


#define SRMMU_CTRL_IMPL_MASK	0xf0000000
#define SRMMU_CTRL_VER_MASK	0x0f000000

#define SRMMU_CTRL_IMPL_SHIFT		 28
#define SRMMU_CTRL_VER_SHIFT		 24


#define SRMMU_CONTEXTS			256
#define SRMMU_SIZE_TBL_LVL_1		256
#define SRMMU_SIZE_TBL_LVL_2		 64
#define SRMMU_SIZE_TBL_LVL_3		 64
#define SRMMU_ENTRY_SIZE_BYTES		  4

/* SRMMU page tables must be aligned to their size */
#define SRMMU_TBL_LVL_1_ALIGN (SRMMU_SIZE_TBL_LVL_1 * SRMMU_ENTRY_SIZE_BYTES)
#define SRMMU_TBL_LVL_2_ALIGN (SRMMU_SIZE_TBL_LVL_2 * SRMMU_ENTRY_SIZE_BYTES)
#define SRMMU_TBL_LVL_3_ALIGN (SRMMU_SIZE_TBL_LVL_3 * SRMMU_ENTRY_SIZE_BYTES)

#define SRMMU_SMALL_PAGE_SIZE	PAGE_SIZE
#define SRMMU_MEDIUM_PAGE_SIZE	(SRMMU_SMALL_PAGE_SIZE  * SRMMU_SIZE_TBL_LVL_3)
#define SRMMU_LARGE_PAGE_SIZE	(SRMMU_MEDIUM_PAGE_SIZE * SRMMU_SIZE_TBL_LVL_2)


/* full 32 bit range divided by 256 lvl1 contexts:
 * upper 8 bits == 16 MiB pages
 *
 * a 16 MiB page divided by 64 lvl2 contexts:
 * next 6 bits = 256 kiB pages
 *
 * a 256 kiB page divided by 64 lvl3 contexts:
 * next 6 bits = 4 kiB pages
 *
 * the last 12 bits are the offset within a 4096 kiB page
 */
#define SRMMU_TBL_LVL_1_SHIFT		24
#define SRMMU_TBL_LVL_1_MASK		0xff
#define SRMMU_TBL_LVL_2_SHIFT		18
#define SRMMU_TBL_LVL_2_MASK		0x3f
#define SRMMU_TBL_LVL_3_SHIFT		12
#define SRMMU_TBL_LVL_3_MASK		0x3f

/* offsets into the different level tables */
#define SRMMU_LVL1_GET_TBL_OFFSET(addr)	((addr >> SRMMU_TBL_LVL_1_SHIFT) \
					 & SRMMU_TBL_LVL_1_MASK)
#define SRMMU_LVL2_GET_TBL_OFFSET(addr)	((addr >> SRMMU_TBL_LVL_2_SHIFT) \
					 & SRMMU_TBL_LVL_2_MASK)
#define SRMMU_LVL3_GET_TBL_OFFSET(addr)	((addr >> SRMMU_TBL_LVL_3_SHIFT) \
					 & SRMMU_TBL_LVL_3_MASK)


#define SRMMU_ENTRY_TYPE_INVALID	0x0
#define SRMMU_ENTRY_TYPE_PT_DESC	0x1
#define SRMMU_ENTRY_TYPE_PT_ENTRY	0x2
#define SRMMU_ENTRY_TYPE_RESERVED	0x3
#define SRMMU_ENTRY_TYPE_MASK		0x3

#define SRMMU_PTE_FLAGS_MASK		0xff

#define SRMMU_CACHEABLE			0x80

#define SRMMMU_ACC_SHIFT	   2

/* user access permissions, ASI 0x8 or 0xa */
#define SRMMU_ACC_U_R_1		(0x0 << SRMMMU_ACC_SHIFT)
#define SRMMU_ACC_U_RW		(0x1 << SRMMMU_ACC_SHIFT)
#define SRMMU_ACC_U_RX		(0x2 << SRMMMU_ACC_SHIFT)
#define SRMMU_ACC_U_RWX		(0x3 << SRMMMU_ACC_SHIFT)
#define SRMMU_ACC_U_X		(0x4 << SRMMMU_ACC_SHIFT)
#define SRMMU_ACC_U_R_2		(0x5 << SRMMMU_ACC_SHIFT)
#define SRMMU_ACC_U_NO_ACCESS_1	(0x6 << SRMMMU_ACC_SHIFT)
#define SRMMU_ACC_U_NO_ACCESS_2	(0x7 << SRMMMU_ACC_SHIFT)

/* supervisor access permissions, ASI 0x9 or 0xb */
#define SRMMU_ACC_S_R		(0x0 << SRMMMU_ACC_SHIFT)
#define SRMMU_ACC_S_RW_1	(0x1 << SRMMMU_ACC_SHIFT)
#define SRMMU_ACC_S_RX_1	(0x2 << SRMMMU_ACC_SHIFT)
#define SRMMU_ACC_S_RWX_1	(0x3 << SRMMMU_ACC_SHIFT)
#define SRMMU_ACC_S_X		(0x4 << SRMMMU_ACC_SHIFT)
#define SRMMU_ACC_S_RW_2	(0x5 << SRMMMU_ACC_SHIFT)
#define SRMMU_ACC_S_RX_2	(0x6 << SRMMMU_ACC_SHIFT)
#define SRMMU_ACC_S_RWX_2	(0x7 << SRMMMU_ACC_SHIFT)


/**
 * The physical address is 36 bits long and composed of a page offset that is 
 * 12 bits wide and the physical page number, that is 24 bits wide
 * The virtual address is composed of the virtual page number, which is 20
 * bits wide and the page offset that is the same as in the physical address.
 * The 4 extra bits are used to map 4 GB sections. Any address that is used
 * to set up the page table entry/descriptor address is hence shifted right
 * by 4 bits for the correct representation of the physical page number
 */
#define SRMMU_PTD(addr)	(((addr >> 4) & ~SRMMU_ENTRY_TYPE_MASK) | \
			SRMMU_ENTRY_TYPE_PT_DESC)

#define SRMMU_PTE(addr, flags)	(((addr >> 4) & ~SRMMU_PTE_FLAGS_MASK) | \
			SRMMU_ENTRY_TYPE_PT_ENTRY | flags)

#define SRMMU_PTD_TO_ADDR(ptd) ((ptd & ~SRMMU_ENTRY_TYPE_MASK) << 4)

#define SRMMU_PTE_TO_ADDR(pte) ((pte & ~(SRMMU_PTE_FLAGS_MASK)) << 4)

/**
 * a SRMMU page table descriptor/entry
 * the page table pointer must be aligned to a boundary equal to the size of
 * the next level page table, i.e.
 * SRMMU_SIZE_TBL_LVL_1 * SRMMU_ENTRY_SIZE_BYTES for level 1,
 * SRMMU_SIZE_TBL_LVL_2 * SRMMU_ENTRY_SIZE_BYTES for level 2,
 * SRMMU_SIZE_TBL_LVL_3 * SRMMU_ENTRY_SIZE_BYTES for level 3.
 *
 * The entry type field may be set to INVALID, PT_DESC, PT_ENTRY or RESERVED
 * and determines whether the MMU treats the remaining bits as page table
 * descriptor or entry
 *
 * @note the cacheable flag should be 0 for all mapped i/o locations
 */
__extension__
struct srmmu_ptde {
	union {
#if defined(__BIG_ENDIAN_BITFIELD)
		struct {
			unsigned long page_table_pointer:30;
			unsigned long entry_type : 2;	
		};

		struct {
			unsigned long physical_page_number :24;
			unsigned long cacheable		   : 1;	
			unsigned long modified		   : 1;	
			unsigned long referenced	   : 1;	
			unsigned long access_permissions   : 3;	
			unsigned long entry_type	   : 2;	
		};
#endif
		unsigned long ptd;
		unsigned long pte;
	};
};


/* page table level the fault occured in */
#define SRMMU_FLT_L_CTX		0
#define SRMMU_FLT_L_LVL1	1
#define SRMMU_FLT_L_LVL2	2
#define SRMMU_FLT_L_LVL3	3


/* srmmu access type faults (User, Superuser, Read, Write eXecute */
#define SRMMU_FLT_AT_R__U_DATA	0
#define SRMMU_FLT_AT_R__S_DATA	1
#define SRMMU_FLT_AT_RX_U_INST	2
#define SRMMU_FLT_AT_RX_S_INST	3
#define SRMMU_FLT_AT_W__U_DATA	4
#define SRMMU_FLT_AT_W__S_DATA	5
#define SRMMU_FLT_AT_W__U_INST	6
#define SRMMU_FLT_AT_W__S_INST	7


/* fault type */
#define SRMMU_FLT_FT_NONE		0
#define SRMMU_FLT_FT_INVALID_ADDR	1
#define SRMMU_FLT_FT_PROTECTION		2
#define SRMMU_FLT_FT_PRIV_VIOLATION	3
#define SRMMU_FLT_FT_TRANSLATION	4
#define SRMMU_FLT_FT_ACCESS_BUS		5
#define SRMMU_FLT_FT_INTERNAL		6


#define SRMMU_FLT_EBE_SHIFT	10	/* external bus error (impl. dep.) */
#define SRMMU_FLT_L_SHIFT	8	/* page table level */
#define SRMMU_FLT_AT_SHIFT	5	/* access type */
#define SRMMU_FLT_FT_SHIFT	2	/* fault type */
#define SRMMU_FLT_FAV_SHIFT	1	/* fault address valid */
#define SRMMU_FLT_OV_SHIFT	0	/* multiple faults occured since read */



/* srmmu fault status register, see SPARCv8 manual, p256 */
__extension__
struct srmmu_fault_status {
	union {
#if defined(__BIG_ENDIAN_BITFIELD)
		struct {
			unsigned long reserved:14;
			unsigned long external_bus_error:8;
			unsigned long page_table_level:2;
			unsigned long access_type:3;
			unsigned long fault_type:3;	
			unsigned long fault_address_vaild:1;
			unsigned long overwrite:1;
		};
#endif
		unsigned long status;
	};
};




int srmmu_select_ctx(unsigned long ctx_num);
void srmmu_enable_mmu(void);

int srmmu_init(void *(*alloc)(size_t size), void (*free)(void *addr));

int srmmu_do_small_mapping_range(unsigned long ctx_num,
				 unsigned long va, unsigned long pa,
				 unsigned long num_pages, unsigned long perm);

int srmmu_do_large_mapping_range(unsigned long ctx_num,
				 unsigned long va, unsigned long pa,
				 unsigned long num_pages, unsigned long perm);

int srmmu_do_small_mapping(unsigned long ctx_num,
			   unsigned long va, unsigned long pa,
			   unsigned long perm);


int srmmu_do_large_mapping(unsigned long ctx_num,
			   unsigned long va, unsigned long pa,
			   unsigned long perm);


void srmmu_release_pages(unsigned long ctx_num,
			 unsigned long va, unsigned long va_end,
			 void  (*free_page)(void *addr));

#endif /*_SPARC_SRMMU_H_ */
