#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <kernel/sysctl.h>


enum modes {OFF, ON, FLASH};
static enum modes mode = FLASH;

static ssize_t mode_show(__attribute__((unused)) struct sysobj *sobj,
			 __attribute__((unused)) struct sobj_attribute *sattr,
			 char *buf)
{
	switch(mode) {
	case OFF:   return sprintf(buf, "off\n");
	case ON:    return sprintf(buf, "on\n");
	case FLASH: return sprintf(buf, "flash\n");
	default:    return sprintf(buf, "Mode error\n");
	}
}

static ssize_t mode_store(__attribute__((unused)) struct sysobj *sobj,
			  __attribute__((unused)) struct sobj_attribute *sattr,
			  const char *buf, size_t len)
{
	if (!strncmp(buf, "on", len))
		mode = ON;
	else if (!strncmp(buf, "off", len))
		mode = OFF;
	else if (!strncmp(buf, "flash", len))
		mode = FLASH;

	return len;
}

static ssize_t hello_show(__attribute__((unused)) struct sysobj *sobj,
			  __attribute__((unused)) struct sobj_attribute *sattr,
			  char *buf)
{
	return sprintf(buf, "hey!\n");
}


__extension__
static struct sobj_attribute mode_attr = __ATTR(mode, mode_show, mode_store);
__extension__
static struct sobj_attribute hello_attr = __ATTR(hello, hello_show, NULL);
__extension__
static struct sobj_attribute shit_attr = __ATTR(shit, hello_show, NULL);
__extension__
static struct sobj_attribute *attributes[] = {&mode_attr, &hello_attr,
	&shit_attr, NULL};

__extension__
static struct sobj_attribute mode2_attr = __ATTR(mode2, mode_show, mode_store);
__extension__
static struct sobj_attribute hello2_attr = __ATTR(hello2, hello_show, NULL);
__extension__
static struct sobj_attribute shit2_attr = __ATTR(shit2, hello_show, NULL);
__extension__
static struct sobj_attribute *attributes2[] = {&mode2_attr, &hello2_attr,
	&shit2_attr, NULL};


int main(void)
{
	struct sysset *sset;
	struct sysobj *sobj;

	sysctl_init();


	sset = sysset_create_and_add("irq", NULL, sys_set);
	sobj = sysobj_create();
	sysobj_add(sobj, NULL, sset, "secondary");

	sset = sysset_create_and_add("more", NULL, sys_set);
	sset = sysset_create_and_add("crap", NULL, sset);

	sobj = sysobj_create();
	sobj->sattr = attributes;
	sysobj_add(sobj, NULL, driver_set, "memory");

	sobj = sysobj_create();
	sysobj_add(sobj, NULL, sys_set, "primary");

	sobj = sysobj_create();
	sysobj_add(sobj, NULL, sys_set, "secondary");

	sobj = sysobj_create();
	sobj->sattr = attributes2;
	sysobj_add(sobj, NULL, sset, "here");


	printf("\n\n");
	sysset_show_tree(sys_set);
	printf("\n\n");

#if 0
	sysobj_show_attr(sysset_find_obj(sys_set,  "/sys/memory"), "mode");
	sysobj_store_attr(sysset_find_obj(sys_set, "/sys/memory"), "mode", "on", 2);
	sysobj_show_attr(sysset_find_obj(sys_set,  "/sys/memory"), "mode");
	printf("\n");
#endif

	return 0;
}

