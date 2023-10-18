#ifndef WATCHDOG_H
#define WATCHDOG_H

int watchdog_enable(void);
int watchdog_disable(void);
int watchdog_feed(unsigned long timeout_ns);


#endif /* WATCHDOG_H */
