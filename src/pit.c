/********************************************************************************************
* FILE:	pit.c

* DESCRIPTION:
	定时器相关处理；
MODIFY HISTORY:
	2014.07.01          yangshansong create
********************************************************************************************/
#include "config.h"

#ifdef HCTEL_3AH_OAM_SUPPORT
#include "hctel_3ah_oam.h"
#endif

#ifdef PIT_SUPPORT
/*
	定时器初始化，periodUs:每隔多少us中断一次；
*/
STATUS pitInit(UINT32 periodUs)
{
	uint32 ldval;
	
	//计算定时加载值
	ldval = periodUs * (SYS_FREQUENCY/2/1000000);

	CLI_PRINT("\r\nldval=%ld",ldval,0,0,0,0,0);

	//开启定时模块时钟
	SIM_SCGC6 |= SIM_SCGC6_PIT_MASK;

	// 开启 PIT
	PIT_MCR = 0x00;

	//period = (period_ns/bus_period_ns)-1
	PIT_LDVAL1 = ldval - 1;

	PIT_TCTRL1 |= PIT_TCTRL_TIE_MASK;	
	
	//开始定时
	PIT_TCTRL1 |= PIT_TCTRL_TEN_MASK;	

	NVIC_IRQ_Enable(PIT1_IRQn);
	return OK;
}

volatile UINT32 pitCount = 0;
volatile BOOL secTimeFlag = FALSE;
/*
	定时器中断服务函数;
*/
void pit1Interrupt(void)
{
	pitCount++;

	/*1s*/
	if((pitCount % 1000) == 0)
	{
		cpuLed ^= cpuLedMask;
		secTimeFlag = TRUE;
	}

	//清除中断标志位
	PIT_TFLG1 |= PIT_TFLG_TIF_MASK;	
	
	// 开启 PIT
	PIT_MCR = 0x00;	
}

/*
	需要定时执行的任务；
*/
void pitTask(void)
{
	/*定时满1s*/
	if(secTimeFlag == TRUE)
	{
		secTimeFlag = FALSE;

	#ifdef HCTEL_3AH_OAM_SUPPORT
		hctel_oamDiscoveryChangeSta();
		hctel_oamSendPduPeriod();
		hctel_oamLinkStaCheck(); 
		hctel_oamLinkTimerDoneCheck();
		hctel_oamLBRspTOutCheck();
	#endif
	}
}
#endif

