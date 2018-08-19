/********************************************************************************************
* FILE:	pit.c

* DESCRIPTION:
	��ʱ����ش���
MODIFY HISTORY:
	2014.07.01          yangshansong create
********************************************************************************************/
#include "config.h"

#ifdef HCTEL_3AH_OAM_SUPPORT
#include "hctel_3ah_oam.h"
#endif

#ifdef PIT_SUPPORT
/*
	��ʱ����ʼ����periodUs:ÿ������us�ж�һ�Σ�
*/
STATUS pitInit(UINT32 periodUs)
{
	uint32 ldval;
	
	//���㶨ʱ����ֵ
	ldval = periodUs * (SYS_FREQUENCY/2/1000000);

	CLI_PRINT("\r\nldval=%ld",ldval,0,0,0,0,0);

	//������ʱģ��ʱ��
	SIM_SCGC6 |= SIM_SCGC6_PIT_MASK;

	// ���� PIT
	PIT_MCR = 0x00;

	//period = (period_ns/bus_period_ns)-1
	PIT_LDVAL1 = ldval - 1;

	PIT_TCTRL1 |= PIT_TCTRL_TIE_MASK;	
	
	//��ʼ��ʱ
	PIT_TCTRL1 |= PIT_TCTRL_TEN_MASK;	

	NVIC_IRQ_Enable(PIT1_IRQn);
	return OK;
}

volatile UINT32 pitCount = 0;
volatile BOOL secTimeFlag = FALSE;
/*
	��ʱ���жϷ�����;
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

	//����жϱ�־λ
	PIT_TFLG1 |= PIT_TFLG_TIF_MASK;	
	
	// ���� PIT
	PIT_MCR = 0x00;	
}

/*
	��Ҫ��ʱִ�е�����
*/
void pitTask(void)
{
	/*��ʱ��1s*/
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

