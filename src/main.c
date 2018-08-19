/*
	系统主函数
*/
#include "config.h"
#include "uart0.h"
#include "pit.h"
#include "eth.h"
#include "net.h"
#include "hctel_3ah_oam.h"

int	trace_level = TRACE_LEVEL_WARN; /* trace level for logging messages */

/*
	系统事件，一般是在中断中产生事件，需要再mian中处理
*/
volatile UINT32 event = 0;

BOOL uartInitFlag = FALSE;
uint8 intSta = 0;

void led_link(int linkCnt);

/*
	主函数
*/
int main(void)
{	
	static volatile UINT32 event_copy;

	//
	// 初始化动态内存分配的区域,从最高地址空间内取RAM_DYNAMIC_SIZE大小
	//
	init_mempool((void *)(0x1FFF0000 + RAM_SIZE - RAM_DYNAMIC_SIZE), RAM_DYNAMIC_SIZE);	
	
	/*系统时钟初始化*/
	sysInit();

	/*uart初始化*/
	uart0_init(UART0_BAUDRATE);
	
	/*使能PORTA时钟*/
	SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK;
	
	/*PORTA_11设置为GPIO引脚*/
	PORTA_PCR11 &= ~0x700;
	PORTA_PCR11 |= 0x100;

	/*PTA初始化为输出*/	
	GPIOA_PDDR |= 0x800;

	/*初始化灭*/
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


