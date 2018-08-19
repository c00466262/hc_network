#ifndef __PIT_H__
#define __PIT_H__

/*定时器每隔多少us中断一次*/
#define PIT_TIMER_VAL	1000L /*1ms*/


STATUS pitInit(UINT32 periodUs);
void pitTask(void);

#endif

