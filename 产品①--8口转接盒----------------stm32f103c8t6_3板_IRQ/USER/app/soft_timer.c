/**
  ******************************************************************************
  * @file    :soft_timer.c used for Potevio interface board
  * @author  :linux.lucian@gmail.com
  * @version :Version-1.0
  * @date    :2014-08-23
  * @brief   :source file for software timer.
  * 
  ******************************************************************************
  */

#include "soft_timer.h"

static struct soft_timer* soft_timer_list;

/**
  * @brief  reset the soft_timer_list.
  * @retval None.
  */

void soft_timer_list_reset(void)
{
	soft_timer_list = TIMER_NULL;
}

/**
  * @brief  add a timer to the software timer list.
  * @param  timer---------pointer to the timer you want to add.
  * @param  call_back-----the call back function when the timer is over.
  * @param  time_count-----the timer count.
  * @retval None.
  */

void add_timer(struct soft_timer* timer,void(*call_back)(void),unsigned int time_count)
{
	struct soft_timer* p;

	p = soft_timer_list;

	if(p==TIMER_NULL)				//if the soft_timer_list have no timer
	{
		p = timer;
		p->flag = TIMER_FLAG_SUSPEND;
		p->tick_count = time_count;
		p->time_over_proc = call_back;
		p->next = TIMER_NULL;
		soft_timer_list = p;
	}
	else
	{
		p = soft_timer_list;
		while(p->next!=TIMER_NULL)
		{
			p = p->next;
		}
		p->next = timer;
		p->next->flag = TIMER_FLAG_SUSPEND;
		p->next->tick_count = time_count;
		p->next->time_over_proc = call_back;
		p->next->next = TIMER_NULL; 
	}
}


/**
  * @brief  remove a timer from the software timer list.
  * @retval None.
  */
void remove_timer(struct soft_timer* timer)
{
	struct soft_timer* t;
	for(t = soft_timer_list; t != TIMER_NULL; t = t->next)
	{
		if(t->next==timer)
		{
			t->next = timer->next;
			break;
		}
	}
}

/**
  * @brief  start or continue a timer   
  * @retval None.
  */

void start_timer(struct soft_timer* timer)
{
	timer->flag = TIMER_FLAG_RUN ;
}

/**
  * @brief  stop a timer without calling back
  * @retval None.
  */
void stop_timer(struct soft_timer* timer)
{
	timer->flag = TIMER_FLAG_STOP;
	timer->tick_count = 0;
}

/**
  * @brief  stop a timer with calling back
  * @retval None.
  */

void stop_timer_with_call(struct soft_timer* timer)
{
	if(timer->flag == TIMER_FLAG_RUN)
		timer->tick_count = 0;
	else if(timer->flag == TIMER_FLAG_SUSPEND)
	{
		timer->flag = TIMER_FLAG_RUN;
		timer->tick_count = 0;
	}
}

/**
  * @brief  suspend a timer
  * @retval None.
  */
void suspend_timer(struct soft_timer* timer)
{
	if(timer->flag == TIMER_FLAG_RUN)
		timer->flag = TIMER_FLAG_SUSPEND;
}

/**
  * @brief  reload the tick_count for a timer
  * @retval None.
  */
void reload_timer(struct soft_timer* timer,unsigned int time_count)
{
	timer->flag = TIMER_FLAG_SUSPEND;
	timer->tick_count = time_count;
}

/**
  * @brief  shoulde be called periodicly by the hardware timer ISR 
  * @retval None.
  */
void timer_periodic_refresh(void)
{
	struct soft_timer* t = TIMER_NULL;

	for(t = soft_timer_list; t != TIMER_NULL; t = t->next)
	{
		if(t->flag==TIMER_FLAG_RUN)
		{
			if(t->tick_count > 0){
				--t->tick_count;
			}else{
				stop_timer(t);
				t->time_over_proc();
			}
		}
	}
}

//-----------------------------------------------eof--------------------------------------------
