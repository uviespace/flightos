#ifndef _KERNEL_LEVELS_H_
#define _KERNEL_LEVELS_H_

/* stole that from linux/kern_levels.h */

#define KERN_SOH        "\001"          /* ASCII Start Of Header */
#define KERN_SOH_ASCII  '\001'

#define KERN_EMERG      KERN_SOH "0"    /* system is unusable */
#define KERN_ALERT      KERN_SOH "1"    /* action must be taken immediately */
#define KERN_CRIT       KERN_SOH "2"    /* critical conditions */
#define KERN_ERR        KERN_SOH "3"    /* error conditions */
#define KERN_WARNING    KERN_SOH "4"    /* warning conditions */
#define KERN_NOTICE     KERN_SOH "5"    /* normal but significant condition */
#define KERN_INFO       KERN_SOH "6"    /* informational */
#define KERN_DEBUG      KERN_SOH "7"    /* debug-level messages */

#define KERN_DEFAULT    KERN_SOH "d"    /* the default kernel loglevel */



/* we're boned */
#define pr_emerg(fmt, ...) \
        printk(KERN_EMERG fmt, ##__VA_ARGS__)

/* immediate action required, we are likely boned*/
#define pr_alert(fmt, ...) \
        printk(KERN_ALERT fmt, ##__VA_ARGS__)

/* critical condition occured, we are probably boned */
#define pr_crit(fmt, ...) \
        printk(KERN_CRIT fmt, ##__VA_ARGS__)

/* some error occured, we are probably fine */
#define pr_err(fmt, ...) \
        printk(KERN_ERR fmt, ##__VA_ARGS__)

/* outlook not so good */
#define pr_warning(fmt, ...) \
        printk(KERN_WARNING fmt, ##__VA_ARGS__)

#define pr_warn pr_warning

/* something worth knowing */
#define pr_notice(fmt, ...) \
        printk(KERN_NOTICE fmt, ##__VA_ARGS__)

/* still interesting */
#define pr_info(fmt, ...) \
        printk(KERN_INFO fmt, ##__VA_ARGS__)

/* Quite correct, sir, blabber on. */
#define pr_debug(fmt, ...) \
        printk(KERN_DEBUG fmt, ##__VA_ARGS__)


void printk_set_level(int lvl);


#endif /* _KERNEL_LEVELS_H_ */
