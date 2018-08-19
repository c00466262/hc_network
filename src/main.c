/*
	ϵͳ������
*/
#include "config.h"
#include "uart0.h"
#include "pit.h"
#include "eth.h"
#include "net.h"
#include "hctel_3ah_oam.h"

int	trace_level = TRACE_LEVEL_WARN; /* trace level for logging messages */

/*
	ϵͳ�¼���һ�������ж��в����¼�����Ҫ��mian�д���
*/
volatile UINT32 event = 0;

BOOL uartInitFlag = FALSE;
uint8 intSta = 0;

void led_link(int linkCnt);

/*
	������
*/
int main(void)
{	
	static volatile UINT32 event_copy;

	//
	// ��ʼ����̬�ڴ���������,����ߵ�ַ�ռ���ȡRAM_DYNAMIC_SIZE��С
	//
	init_mempool((void *)(0x1FFF0000 + RAM_SIZE - RAM_DYNAMIC_SIZE), RAM_DYNAMIC_SIZE);	
	
	/*ϵͳʱ�ӳ�ʼ��*/
	sysInit();

	/*uart��ʼ��*/
	uart0_init(UART0_BAUDRATE);
	
	/*ʹ��PORTAʱ��*/
	SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK;
	
	/*PORTA_11����ΪGPIO����*/
	PORTA_PCR11 &= ~0x700;
	PORTA_PCR11 |= 0x100;

	/*PTA��ʼ��Ϊ���*/	
	GPIOA_PDDR |= 0x800;

	/*��ʼ����*/
	GPIOA_PDOR |= 0x800;

#ifdef PIT_SUPPORT
	pitInit(PIT_TIMER_VAL);	
#endif

#ifdef HCTEL_3AH_OAM_SUPPORT
	hctel_oamInit();
#endif

	eth_init();
	cliCmdShellStart();
	
	while(1)
	{			
		cliCmdShell();

	#ifdef ETH_SUPPORT
		eth_process();
	#endif

	#ifdef PIT_SUPPORT
		pitTask();
	#endif	
	}

	return 1;
}

void led_link(int linkCnt)
{
	int i;

	for(i=0; i<linkCnt; i++)
	{
		GPIOA_PDOR &= ~0x800;
		time_delay_ms(50);
		GPIOA_PDOR |= 0x800;
		time_delay_ms(50);
	}
}


