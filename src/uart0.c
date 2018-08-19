/********************************************************************************************
* FILE:	uart0.c

* DESCRIPTION:
	UART0收发
MODIFY HISTORY:
	2014.05.28            yangshansong   create
********************************************************************************************/
#include "config.h"
#include "uart0.h"

#ifdef UART0_SUPPORT

/*环形缓冲区*/
LOCAL UCHAR uart0_rx_buf[MAX_UART0_BUF_SIZE];//128
LOCAL UCHAR *rx_in_pos = uart0_rx_buf;
LOCAL UCHAR *rx_out_pos = uart0_rx_buf;

/********************************************************************/
/*
	fun:uart初始化;
	arg:
		-baud:波特率;
 */
void uart0_init(int baud)
{
    UART_MemMapPtr uartch = CLI_UART;
    register uint16 sbr, brfa;
    uint8 temp;
    int sysclk;

	/************************引脚初始化****************************/
	/* Enable the pins for the selected UART */
	if (uartch == UART0_BASE_PTR)
	{
		/* Enable the UART0_TXD function on PTD6 */
		PORTD_PCR6 = PORT_PCR_MUX(0x3); // UART is alt3 function for this pin

		/* Enable the UART0_RXD function on PTD7 */
		PORTD_PCR7 = PORT_PCR_MUX(0x3); // UART is alt3 function for this pin
	}

	if (uartch == UART1_BASE_PTR)
	{
		/* Enable the UART1_TXD function on PTC4 */
		PORTC_PCR4 = PORT_PCR_MUX(0x3); // UART is alt3 function for this pin

		/* Enable the UART1_RXD function on PTC3 */
		PORTC_PCR3 = PORT_PCR_MUX(0x3); // UART is alt3 function for this pin
	}

	if (uartch == UART2_BASE_PTR)
	{
		/* Enable the UART2_TXD function on PTD3 */
		PORTD_PCR3 = PORT_PCR_MUX(0x3); // UART is alt3 function for this pin

		/* Enable the UART2_RXD function on PTD2 */
		PORTD_PCR2 = PORT_PCR_MUX(0x3); // UART is alt3 function for this pin
	}

	if (uartch == UART3_BASE_PTR)
	{
		/* Enable the UART3_TXD function on PTC17 */
		PORTC_PCR17 = PORT_PCR_MUX(0x3); // UART is alt3 function for this pin

		/* Enable the UART3_RXD function on PTC16 */
		PORTC_PCR16 = PORT_PCR_MUX(0x3); // UART is alt3 function for this pin
	}
	if (uartch == UART4_BASE_PTR)
	{
		/* Enable the UART3_TXD function on PTC17 */
		PORTE_PCR24 = PORT_PCR_MUX(0x3); // UART is alt3 function for this pin

		/* Enable the UART3_RXD function on PTC16 */
		PORTE_PCR25 = PORT_PCR_MUX(0x3); // UART is alt3 function for this pin
	}
	if (uartch == UART5_BASE_PTR)
	{
		SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;
		
		/* Enable the UART3_TXD function on PTC17 */
		PORTE_PCR8 = PORT_PCR_MUX(0x3); // UART is alt3 function for this pin

		/* Enable the UART3_RXD function on PTC16 */
		PORTE_PCR9 = PORT_PCR_MUX(0x3); // UART is alt3 function for this pin
	}    


	/************************时钟使能****************************/
	/* UART0 and UART1 are clocked from the core clock, but all other UARTs are
	* clocked from the peripheral clock. So we have to determine which clock
	* to send to the uart_init function.
	*/
	if ((uartch == UART0_BASE_PTR) | (uartch == UART1_BASE_PTR))
	{
		sysclk = SYS_FREQUENCY / 1000;
	}
	else
	{
		sysclk = SYS_FREQUENCY / 1000;
		sysclk = sysclk / (((SIM_CLKDIV1 & SIM_CLKDIV1_OUTDIV2_MASK) >> 24)+ 1); 
	}
    
	/* Enable the clock to the selected UART */    
    if(uartch == UART0_BASE_PTR)
    {
		SIM_SCGC4 |= SIM_SCGC4_UART0_MASK;
	}
    else
    {
    	if (uartch == UART1_BASE_PTR)
    	{
			SIM_SCGC4 |= SIM_SCGC4_UART1_MASK;
		}
    	else
    	{
    		if (uartch == UART2_BASE_PTR)
    		{
    			SIM_SCGC4 |= SIM_SCGC4_UART2_MASK;
    		}
    		else
    		{	
    			if(uartch == UART3_BASE_PTR)
    			{
    				SIM_SCGC4 |= SIM_SCGC4_UART3_MASK;
    			}
    			else
    			{
    				if(uartch == UART4_BASE_PTR)
    					SIM_SCGC1 |= SIM_SCGC1_UART4_MASK;
    				else
    					SIM_SCGC1 |= SIM_SCGC1_UART5_MASK;
    			}
    		}
    	}
	}

    /************************波特率、帧格式配置****************************/                            
    /* Make sure that the transmitter and receiver are disabled while we 
     * change settings.
     */
    UART_C2_REG(uartch) &= ~(UART_C2_TE_MASK | UART_C2_RE_MASK);

    /* Configure the UART for 8-bit mode, no parity */
    UART_C1_REG(uartch) = 0;	/* We need all default settings, so entire register is cleared */
    
    /* Calculate baud settings */
    sbr = (uint16)((sysclk*1000) / (baud * 16));
        
    /* Save off the current value of the UARTx_BDH except for the SBR field */
    temp = UART_BDH_REG(uartch) & ~(UART_BDH_SBR(0x1F));
    
    UART_BDH_REG(uartch) = temp | UART_BDH_SBR(((sbr & 0x1F00) >> 8));
    UART_BDL_REG(uartch) = (uint8)(sbr & UART_BDL_SBR_MASK);
    
    /* Determine if a fractional divider is needed to get closer to the baud rate */
    brfa = (((sysclk*32000)/(baud * 16)) - (sbr * 32));
    
    /* Save off the current value of the UARTx_C4 register except for the BRFA field */
    temp = UART_C4_REG(uartch) & ~(UART_C4_BRFA(0x1F));
    
    UART_C4_REG(uartch) = temp |  UART_C4_BRFA(brfa);    
	
    /* Enable receiver and transmitter */
	UART_C2_REG(uartch) |= (UART_C2_TE_MASK | UART_C2_RE_MASK);

	/************************中断初始化配置****************************/
	/*Enable receive interrupt*/
	UART_C2_REG(uartch) |= UART_C2_RIE_MASK;

	UART_C3_REG(uartch) |= (UART_C3_ORIE_MASK | UART_C3_FEIE_MASK);

	NVIC_IRQ_Enable(UART5_STA_IRQn);
	NVIC_IRQ_Enable(UART5_ERR_IRQn);	
}

/*
	发送一个字符；
 */ 
void uart0_put_char (char ch)
{
	UART_MemMapPtr uartch = CLI_UART;
	
    /* Wait until space is available in the FIFO */
    while(!(UART_S1_REG(uartch) & UART_S1_TDRE_MASK));
    
    /* Send the character */
    UART_D_REG(uartch) = (uint8)ch;
}

/*
	发送一个字符串
*/
void uart0_put_str(char *str)
{
	int i = 0;
	
	while(str[i] != '\0')
	{
	
		uart0_put_char(str[i++]);
	}
}

/*
	从串口缓冲区读出1字节数据
	先取出再加1
*/
BOOL uart0_get_char (char* ch)
{
	if(rx_in_pos==rx_out_pos)           //RxBuf Empty
	{
		return FALSE;
	}
	
	*ch = *rx_out_pos;  
	rx_out_pos++;
	
	if(rx_out_pos == uart0_rx_buf + MAX_UART0_BUF_SIZE) 
		rx_out_pos = uart0_rx_buf;

	return TRUE;
}


/*
	向串口缓冲区写入1字节数据
	先写入再加1
*/
void uart0_interrupt(void)
{
	UART_MemMapPtr uartch = CLI_UART;
	UCHAR *p;
	UINT8 state;
	
	/*
	--bit7:TDRE(Transmit data register emptyFlag)
	--bit6:TC(Transmit complete flag)
	--bit5:RDRF(Receive data register Full Flag)
	--bit4:IDLE;
	--bit3:OR(rcv overrun);
	--bit2:NF;
	--bit1:FE(Frame error);
	--bit0:PF(Parity error);
	*/
	state = UART_S1_REG(uartch);
	if((state & UART_S1_RDRF_MASK) && (UART_C2_REG(uartch) & UART_C2_RIE_MASK))
	{				
		p = rx_in_pos + 1;

		if(p == uart0_rx_buf + MAX_UART0_BUF_SIZE)/*超过返回到开头*/
			p = uart0_rx_buf;

		if(p == rx_out_pos) /*收发必须至少间隔一个字节*/
			return;                 //RxBuf Full

		*rx_in_pos = UART_D_REG(uartch);
		rx_in_pos = p;
	}
}

/*
	串口错误中断处理函数
*/
void uart0_interrupt_err(void)
{
	UART_MemMapPtr uartch = CLI_UART;
	uint8 state;
	
	state = UART_S1_REG(uartch);
	if((state & UART_S1_OR_MASK) || (state & UART_S1_FE_MASK))
	{
		UART_D_REG(uartch); /*读一下D寄存器，清除错误标志*/
	}

	if(UART_S2_REG(uartch) & UART_S2_LBKDIF_MASK)
	{
		UART_S2_REG(uartch) |= UART_S2_LBKDIF_MASK;
	}
}

/************************U********************************************/
#if 0
static char cliPrintBuf[256];
void cli_print(char *fmt, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6)
{
	sprintf(cliPrintBuf, fmt, arg1, arg2, arg3, arg4, arg5, arg6);

	uart0_put_str(cliPrintBuf);
}
#endif

#endif

