/* 
 * deonet omcid timer.h 
 */

#ifndef TIME_H
#define TIME_H
#include <stdlib.h>

#define MAX_TIMER_COUNT 1000

typedef enum
{
	TIMER_SINGLE_SHOT = 0, /*Periodic Timer*/
	TIMER_PERIODIC         /*Single Shot Timer*/
} t_timer;

typedef void (*time_handler)(size_t timer_id, void * user_data);

struct timer_node
{
	int                 fd;
	time_handler        callback;
	void *              user_data;
	unsigned int        interval;
	t_timer             type;
	struct timer_node * next;
};

int timer_initialize();
size_t start_timer(unsigned int interval, time_handler handler, t_timer type, void * user_data);
void stop_timer(size_t timer_id);
void timer_finalize();

#endif
