/*
 * File:        sysinit.c
 * Purpose:     Kinetis Configuration
 *              Initializes processor to a default state
 *
 * Notes:
 *
 */

#include "config.h"
#include "sysinit.h"
#include "uart0.h"
#include "wdog.h"

/********************************************************************/

/* Actual system clock frequency */
int core_clk_khz;
int core_clk_mhz;
int periph_clk_khz;

/********************************************************************/
void sysInit (void)
{
	/*
	* Enable all of the port clocks. These have to be enabled to configure
	* pin muxing options, so most code will need all of these on anyway.
	*/
	SIM_SCGC5 |= (SIM_SCGC5_PORTA_MASK
				  | SIM_SCGC5_PORTB_MASK
				  | SIM_SCGC5_PORTC_MASK
				  | SIM_SCGC5_PORTD_MASK
				  | SIM_SCGC5_PORTE_MASK );

#if 1
	//禁用看门狗模块
	/* WDOG_UNLOCK: WDOGUNLOCK=0xC520 */
	WDOG_BASE_PTR->UNLOCK = (UINT16)0xC520u;     /* Key 1 */
	/* WDOG_UNLOCK : WDOGUNLOCK=0xD928 */
	WDOG_BASE_PTR->UNLOCK  = (UINT16)0xD928u;    /* Key 2 */
	/* WDOG_STCTRLH: ??=0,DISTESTWDOG=0,BYTESEL=0,TESTSEL=0,TESTWDOG=0,??=0,STNDBYEN=1,WAITEN=1,STOPEN=1,DBGEN=0,ALLOWUPDATE=1,WINEN=0,IRQRSTEN=0,CLKSRC=1,WDOGEN=0 */
	WDOG_BASE_PTR->STCTRLH = (UINT16)0x01D2u;  
#endif

#if defined(NO_PLL_INIT)
	core_clk_mhz = 21; //FEI mode
#else 
	/* Ramp up the system clock */
	core_clk_mhz = pll_init(CORE_CLK_MHZ, REF_CLK);
#endif

	/*
	* Use the value obtained from the pll_init function to define variables
	* for the core clock in kHz and also the peripheral clock. These
	* variables can be used by other functions that need awareness of the
	* system frequency.
	*/
	core_clk_khz = core_clk_mhz * 1000;
	periph_clk_khz = core_clk_khz / (((SIM_CLKDIV1 & SIM_CLKDIV1_OUTDIV2_MASK) >> 24)+ 1);

	/* For debugging purposes, enable the trace clock and/or FB_CLK so that
	* we'll be able to monitor clocks and know the PLL is at the frequency
	* that we expect.
	*/
	traceClkInit();
	fbClkInit();
}

/*
	将系统时钟输出到
	芯片的一个外部引脚上便于测量；这里选用PA6
*/
void traceClkInit(void)
{
	/* Set the trace clock to the core clock frequency */
	SIM_SOPT2 |= SIM_SOPT2_TRACECLKSEL_MASK;

	/* Enable the TRACE_CLKOUT pin function on PTA6 (alt7 function) */
	PORTA_PCR6 = (PORT_PCR_MUX(0x7));
}

/*
	将FlexBus的时钟输出到外部引脚，便于查看,这里用PC3
*/
void fbClkInit(void)
{
	/* Enable the clock to the FlexBus module */
    SIM_SCGC7 |= SIM_SCGC7_FLEXBUS_MASK;

 	/* Enable the FB_CLKOUT function on PTC3 (alt5 function) */
	PORTC_PCR3 = (PORT_PCR_MUX(0x5));
}
/********************************************************************/
