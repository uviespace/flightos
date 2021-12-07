/**
 * @file include/kernel/syscall.h
 *
 * @ingroup syscall
 *
 * These macros were ripped of linux/syscalls.h, because the argument expansion
 * is very handy. We don't keep the asmlinkage and metadata stuff, but the
 * argument type size checks and sanitization we like, because it will catch
 * those muppets who tries to pass a struct to a syscall by value.
 */


#ifndef _KERNEL_SYSCALL_H_
#define _KERNEL_SYSCALL_H_

#include <kernel/kernel.h>
#include <kernel/types.h>


#define __MAP0(m,...)
#define __MAP1(m,t,a) m(t,a)
#define __MAP2(m,t,a,...) m(t,a), __MAP1(m,__VA_ARGS__)
#define __MAP3(m,t,a,...) m(t,a), __MAP2(m,__VA_ARGS__)
#define __MAP4(m,t,a,...) m(t,a), __MAP3(m,__VA_ARGS__)
#define __MAP5(m,t,a,...) m(t,a), __MAP4(m,__VA_ARGS__)
#define __MAP6(m,t,a,...) m(t,a), __MAP5(m,__VA_ARGS__)
#define __MAP(n,...) __MAP##n(__VA_ARGS__)


#define __SC_DECL(t, a)	t a
#define __TYPE_IS_LL(t) (__same_type((t)0, 0LL) || __same_type((t)0, 0ULL))
#define __SC_LONG(t, a) __typeof(__builtin_choose_expr(__TYPE_IS_LL(t), 0LL, 0L)) a
#define __SC_CAST(t, a)	(t) a
#define __SC_ARGS(t, a)	a
#define __SC_TEST(t, a) (void)BUILD_BUG_ON_ZERO(!__TYPE_IS_LL(t) && sizeof(t) > sizeof(long))


#define SYSCALL_DEFINE0(sname)					\
	long sys_##sname(void)

#define SYSCALL_DEFINE1(name, ...) SYSCALL_DEFINEx(1, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE2(name, ...) SYSCALL_DEFINEx(2, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE3(name, ...) SYSCALL_DEFINEx(3, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE4(name, ...) SYSCALL_DEFINEx(4, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE5(name, ...) SYSCALL_DEFINEx(5, _##name, __VA_ARGS__)
#define SYSCALL_DEFINE6(name, ...) SYSCALL_DEFINEx(6, _##name, __VA_ARGS__)



#define SYSCALL_DEFINEx(x, name, ...)						\
	long sys##name(__MAP(x,__SC_DECL,__VA_ARGS__))				\
		__attribute__((alias(__stringify(SyS##name))));			\
	static inline long __syscall##name(__MAP(x,__SC_DECL,__VA_ARGS__));	\
	long SyS##name(__MAP(x,__SC_LONG,__VA_ARGS__));				\
	long SyS##name(__MAP(x,__SC_LONG,__VA_ARGS__))				\
	{									\
		long ret = __syscall##name(__MAP(x,__SC_CAST,__VA_ARGS__));	\
		__MAP(x,__SC_TEST,__VA_ARGS__);					\
		return ret;							\
	}									\
	static inline long __syscall##name(__MAP(x,__SC_DECL,__VA_ARGS__))


#endif /* _KERNEL_SYSCALL_H_ */
