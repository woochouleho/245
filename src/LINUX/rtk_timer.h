#ifndef INCLUDE_BOATIMER_H
#define INCLUDE_BOATIMER_H

#include <time.h>
struct	callout_s {
    struct timespec	c_time;		/* time at which to call routine */
    void		*c_arg;		/* argument to routine */
    void		(*c_func) __P((void *)); /* routine */
    struct		callout_s *c_next;
};

void timeout_utility(void (*func) __P((void *)), void *arg, int time, struct callout_s *handle);
void untimeout_utility(struct callout_s *handle);
void calltimeout(void);
#define TIMEOUT_F(fun, arg1, arg2, handle) 	timeout_utility(fun,arg1,arg2, &handle)
#define UNTIMEOUT_F(fun, arg, handle)		untimeout_utility(&handle)

#endif
