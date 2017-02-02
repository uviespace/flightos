/**
 * @brief Address Space Identifiers (ASI) for the LEON
 * @author Armin Luntzer (armin.luntzer@univie.ac.at)
 */


#ifndef _SPARC_ASI_H_
#define _SPARC_ASI_H_

#define ASI_LEON_NOCACHE        0x01
#define ASI_LEON_DCACHE_MISS    ASI_LEON_NOCACHE

#define ASI_LEON_CACHEREGS      0x02
#define ASI_LEON_IFLUSH         0x10
#define ASI_LEON_DFLUSH         0x11

#define ASI_LEON2_IFLUSH	0x05
#define ASI_LEON2_DFLUSH	0x06

#define ASI_LEON_MMUFLUSH       0x18
#define ASI_LEON_MMUREGS        0x19
#define ASI_LEON_BYPASS         0x1c
#define ASI_LEON_FLUSH_PAGE     0x10



#define ASI_LEON_CACHEREGS_SNOOPING_BIT	(1<<27)		/* GR712RC-UM p. 44 */



#endif /* _SPARC_ASI_H_ */
