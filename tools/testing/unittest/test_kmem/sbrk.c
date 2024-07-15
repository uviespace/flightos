#include <stdint.h>
#include <stddef.h>

extern unsigned long ksbrk;

extern uint32_t addr_lo;
extern uint32_t addr_hi;

#define STACK_ALIGN	8
#define PAGE_SIZE	4096
#define ALIGN_MASK(x, mask)    (((x) + (mask)) & ~(mask))
#define ALIGN(x, a)            ALIGN_MASK(x, (typeof(x))(a) - 1)
#define ALIGN_PTR(x, a)        (typeof(x)) ALIGN((unsigned long) x, a)
#define IS_ALIGNED(x, a)       (((x) & ((typeof(x))(a) - 1)) == 0)


#define WORD_ALIGN(x)	ALIGN((x), sizeof(uint64_t))


#define PAGE_ALIGN(addr)	ALIGN(addr, PAGE_SIZE)

void *memset32(void *s, uint32_t c, size_t n)
{
        uint32_t *p = s;

        while (n--)
                *p++ = c;

        return s;
}

void *kernel_sbrk(intptr_t increment)
{
        long brk;
        long oldbrk;

        unsigned long ctx;


        if (!increment)
                 return (void *) ksbrk;

        oldbrk = ksbrk;

        /* make sure we are always double-word aligned, otherwise we can't
         * use ldd/std instructions without trapping */
        increment = ALIGN(increment, STACK_ALIGN);

        brk = oldbrk + increment;

        if (brk < addr_lo)
                return (void *) -1;

        if (brk > addr_hi)
                return (void *) -1;

        /* try to release pages if we decremented below a page boundary */
        if (increment < 0) {
                if (PAGE_ALIGN(brk) < PAGE_ALIGN(oldbrk - PAGE_SIZE)) {
#if 0
                        printf("SBRK: release %lx (va %lx, pa%lx) to va %lx pa %lx, %d pages\n",
                                 brk, PAGE_ALIGN(brk), PAGE_ALIGN(brk),
                                 oldbrk, oldbrk,
                                 (PAGE_ALIGN(oldbrk) - PAGE_ALIGN(brk)) / PAGE_SIZE);
#endif

			/* mark unused */
                        if (PAGE_ALIGN(oldbrk) >  PAGE_ALIGN(brk)) {
				/* we legally own more memory, so setting n+1 is
				 * fine
				 */
				int i;
				unsigned char *p = (unsigned char *) brk;
#if 1
				for (i = 0; i < oldbrk - brk; i++) {
#if 0
					printf("set %p\n", &p[i]);
#endif

					p[i] = 0x55;
				}
#endif
			}
                }
        }

        ksbrk = brk;
#if 0
        printf("SBRK: moved %08lx -> %08lx, alloc %g MB\n", oldbrk, brk, (brk-addr_lo) / (1024. * 1024.));
#else
#if 0
        printf("%g\n", (brk-addr_lo) / (1024. * 1024.));
#endif
#endif

        return (void *) oldbrk;
}



