/*
 *	Timer Function -- support multiple timer
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include "rtk_timer.h"

static struct callout_s *callout = NULL;	/* Callout list */
static struct timespec timenow;
//static struct timeval timenow;		/* Current time */

/*
 * timeout - Schedule a timeout.
 *
 * Note that this timeout takes the number of seconds, NOT hz (as in
 * the kernel).
 */
void
timeout_utility(func, arg, time, handle)
    void (*func) __P((void *));
    void *arg;
    int time;
    struct callout_s *handle;
{
    struct callout_s *p, **pp;

    untimeout_utility(handle);

    handle->c_arg = arg;
    handle->c_func = func;
	clock_gettime(CLOCK_MONOTONIC, &timenow);
    //gettimeofday(&timenow, NULL);
    handle->c_time.tv_sec = timenow.tv_sec + time;
    handle->c_time.tv_nsec = timenow.tv_nsec;

    /*
     * Find correct place and link it in.
     */
    for (pp = &callout; (p = *pp); pp = &p->c_next)
	if (handle->c_time.tv_sec < p->c_time.tv_sec
	    || (handle->c_time.tv_sec == p->c_time.tv_sec
		&& handle->c_time.tv_nsec < p->c_time.tv_nsec))
	    break;
    handle->c_next = p;
    *pp = handle;
}


/*
 * untimeout - Unschedule a timeout.
 */
void
untimeout_utility(handle)
struct callout_s *handle;
{
    struct callout_s **copp, *freep;
	
    /*
     * Find first matching timeout and remove it from the list.
     */
    for (copp = &callout; (freep = *copp); copp = &freep->c_next)
	if (freep == handle) {
	    *copp = freep->c_next;
	    break;
	}
}


/*
 * calltimeout - Call any timeout routines which are now due.
 */
void
calltimeout()
{
    struct callout_s *p;

    while (callout != NULL) {
		p = callout;
		if (clock_gettime(CLOCK_MONOTONIC, &timenow) < 0)
		    //fatal("Failed to get time of day: %m");
	    	printf("Failed to get time of day: %m");
		if (!(p->c_time.tv_sec < timenow.tv_sec
		      || (p->c_time.tv_sec == timenow.tv_sec
			  && p->c_time.tv_nsec <= timenow.tv_nsec)))
	    	break;		/* no, it's not time yet */
		callout = p->c_next;
		(*p->c_func)(p->c_arg);
    }
}

/*
 * timeleft - return the length of time until the next timeout is due.
 */
static struct timespec *
timeleft(tvp)
    struct timespec *tvp;
{
    if (callout == NULL)
	return NULL;

    clock_gettime(CLOCK_MONOTONIC, &timenow);
    tvp->tv_sec = callout->c_time.tv_sec - timenow.tv_sec;
    tvp->tv_nsec = callout->c_time.tv_nsec - timenow.tv_nsec;
    if (tvp->tv_nsec < 0) {
	tvp->tv_nsec += 1000000000;
	tvp->tv_sec -= 1;
    }
    if (tvp->tv_sec < 0)
	tvp->tv_sec = tvp->tv_nsec = 0;

    return tvp;
}

