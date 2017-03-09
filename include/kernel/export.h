/**
 * @file include/kernel/export.h
 *
 * @note derived from include/linux/export.h */
#ifndef _KERNEL_EXPORT_H_
#define _KERNEL_EXPORT_H_


struct kernel_symbol {
        unsigned long value;
        const char *name;
};


#define __KERNEL_SYMBOL(x) x
#define __KERNEL_SYMBOL_STR(x) #x


/* Indirect, so macros are expanded before pasting. */
#define KERNEL_SYMBOL(x) __KERNEL_SYMBOL(x)
#define KERNEL_SYMBOL_STR(x) __KERNEL_SYMBOL_STR(x)

#if 0
/* For every exported symbol, place a struct in the __ksymtab section */
#define ___EXPORT_SYMBOL(sym, sec)                                      \
        extern typeof(sym) sym;                                         \
        static const char __kstrtab_##sym[]                             \
        __attribute__((section("__ksymtab_strings"), aligned(1)))       \
        = KERNEL_SYMBOL_STR(sym);                                       \
        static const struct kernel_symbol __ksymtab_##sym               \
        __attribute__((used))                                           \
        __attribute__((section("___ksymtab" sec "+" #sym), used))       \
        = { (unsigned long)&sym, __kstrtab_##sym }
#else
#define ___EXPORT_SYMBOL(sym, sec)                                      \
	extern typeof(sym) sym;                                         \
	static const char __kstrtab_##sym[]                             \
        = KERNEL_SYMBOL_STR(sym);                                       \
        static const struct kernel_symbol __ksymtab_##sym               \
        __attribute__((used))                                           \
        = { (unsigned long)&sym, __kstrtab_##sym }

#endif

#define __EXPORT_SYMBOL ___EXPORT_SYMBOL

#define EXPORT_SYMBOL(sym)                                      \
        __EXPORT_SYMBOL(sym, "")



#endif /* _KERNEL_EXPORT_H_ */
