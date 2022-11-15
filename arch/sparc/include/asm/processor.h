/**
 * @file arch/sparc/include/asm/processor.h
 */

#ifndef _SPARC_PROCESSOR_H_
#define _SPARC_PROCESSOR_H_

#include <asm/leon.h>
#include <asm/leon_reg.h>
#include <asm/io.h>

#ifdef CONFIG_MPPB
/* write/read the leon2 power down register */
#define cpu_relax() do {						\
		iowrite32be(0xFEE1DEAD, (void *) LEON2_POWERDOWN);	\
		ioread32be((void *) 0x80000018);			\
		} while(0);

#define cpu_icache_disable() \
		iowrite32be(ioread32be((void *) LEON2_CACHECTRL) & 0xFFFFFFFC, \
			    (void *) LEON2_CACHECTRL);
#define cpu_dcache_disable() \
		iowrite32be(ioread32be((void *) LEON2_CACHECTRL) & 0xFFFFFFF3, \
			    (void *) LEON2_CACHECTRL);




#else
/* XXX should not assume that this is always a valid physical address
 * (see also LEON3FT Cache Controller Power Down erratum
 */
#define cpu_relax() leon3_powerdown_safe(0xFFFFFFF0)
#endif

#endif /* _SPARC_PROCESSOR_H_ */

