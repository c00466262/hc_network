#ifndef __PIT_H__
#define __PIT_H__

/*��ʱ��ÿ������us�ж�һ��*/
#define PIT_TIMER_VAL	1000L /*1ms*/


STATUS pitInit(UINT32 periodUs);
void pitTask(void);

#endif

