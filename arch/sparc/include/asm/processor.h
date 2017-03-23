/**
 * @file arch/sparc/include/asm/processor.h
 */

#ifndef _SPARC_PROCESSOR_H_
#define _SPARC_PROCESSOR_H_

#include <asm/leon.h>
#include <asm/io.h>

#ifdef CONFIG_MPPB
/* write/read the leon2 power down register */
#define cpu_relax() do {					\
		iowrite32be(0xFEE1DEAD, (void *) 0x80000018);	\
		ioread32be((void *) 0x80000018);		\
		} while(0);

#else
/* XXX should not assume that this is always a valid physical address
 * (see also LEON3FT Cache Controller Power Down erratum
 */
#define cpu_relax() leon3_powerdown_safe(0x40000000)
#endif

#endif /* _SPARC_PROCESSOR_H_ */

