#include <kernel/init.h>
#include <kernel/kmem.h>
#include <kernel/kthread.h>
#include <kernel/module.h>
#include <kernel/application.h>
#include <kernel/user.h>

#include <grspw2.h>
#include <modules-image.h>

#define MSG "MYAPP: "

/* a spacewire core configuration dummy (TODO: cleanup syscall IF) */
struct spw_user_cfg spw_cfg[2];

static int myapp_init(void)
{
#ifdef CONFIG_EMBED_APPLICATION
	/* load SMILE ASW */
	void *addr = module_read_embedded("myapp");
	printk(MSG "test executable address is %p\n", addr);
	if (addr)
		application_load(addr, "ASW", KTHREAD_CPU_AFFINITY_NONE, 0, NULL);
#endif /* CONFIG_EMBED_APPLICATION */

	return 0;
}
lvl1_usercall(myapp_init)
