/********************************************************************************************
* FILE:	eth.c

* DESCRIPTION:
	以太网层接收，发送处理
MODIFY HISTORY:
	2010.7.13            lixingfu   create
	2014.06.26          yangshansong modify
********************************************************************************************/

#include "config.h"

#include "string.h"

#include "net.h"
#include "eth.h"
#include "cli.h"
#include "arp.h"
#include "ip.h"
#include "cli.h"
#include "hostEnd.h"

/*
	以太网收发包数量
*/
volatile UINT32 eth_rx_pkts = 0;
volatile UINT32 eth_tx_pkts = 0;
volatile UINT32 eth_reset_counts = 0;/*复位次数*/

/* Local variables */
static UINT8 TxBufIndex;
static UINT8 RxBufIndex;

/* ENET local DMA Descriptors. */
static __align(16) RX_Desc Rx_Desc[NUM_RX_BUF];
static __align(16) TX_Desc Tx_Desc[NUM_TX_BUF];

/* ENET local DMA buffers. */
static UINT32 rx_buf[NUM_RX_BUF][ETH_BUF_SIZE>>2];

#define IDX(x) (x/2)
#define SWE(n) ((((n) & 0x00FF) << 8) | (((n) & 0xFF00) >> 8))

/*
	以太网接口操作函数
*/
LOCAL NET_FUNCS ethNetFuncs=
{
	eth_rcve,
	eth_send,
};

/*
	以太网接口定义
*/
ETH_DEVICE ethDevice = 
{
	{
		"eth",
		0,
		IFF_RUNNING|IFF_UP,
		0,0,0,0,0,0,0,
		hostEndRcv,/*在HOST模式下直接送给*/
		&ethNetFuncs
	},
};

/*
GPIO_PF3:LINK/ACT LED0	需要程序控制
GPIO_PF2:SPEED LED1		需要程序控制
*/
#ifdef ETH_SUPPORT

/*--------------------------- write_PHY --------------------------------------*/
void write_PHY (UINT32 PhyReg, UINT16 Value)
{
  /* Write a data 'Value' to PHY register 'PhyReg'. */
  UINT32 tout;

  /* Clear MII Interrupt */
  ENET_EIR = ENET_EIR_MII_MASK;

  /* Send MDIO write command */
  ENET_MMFR =  ENET_MMFR_ST (1)             |
                ENET_MMFR_OP (1)            |
                ENET_MMFR_PA (PHY_DEF_ADDR) |
                ENET_MMFR_RA (PhyReg)       |
                ENET_MMFR_TA (2)            |
                ENET_MMFR_DATA (Value)      ;

  /* Wait until operation completed */
  tout = 0;
  for (tout = 0; tout < MII_WR_TOUT; tout++) 
  {
    if (ENET_EIR & ENET_EIR_MII_MASK) 
    {
      break;
    }
  }
}

/*--------------------------- read_PHY ---------------------------------------*/
UINT16 read_PHY (UINT32 PhyReg)
{
  /* Read a PHY register 'PhyReg'. */
  UINT32 tout;

  /* Clear MII Interrupt */
  ENET_EIR = ENET_EIR_MII_MASK;

  /* Send MDIO read command */
  ENET_MMFR =  ENET_MMFR_ST (1)            |
               ENET_MMFR_OP (2)            |
               ENET_MMFR_PA (PHY_DEF_ADDR) |
               ENET_MMFR_RA (PhyReg)       |
               ENET_MMFR_TA (2)            ;

  /* Wait until operation completed */
  tout = 0;
  for (tout = 0; tout < MII_RD_TOUT; tout++) 
  {
    if (ENET_EIR & ENET_EIR_MII_MASK) 
    {
      break;
    }
  }
  return (ENET_MMFR & ENET_MMFR_DATA_MASK);
}

/*--------------------------- rx_descr_init ----------------------------------*/
static void rx_descr_init (void) 
{
  /* Initialize Receive DMA Descriptor array. */
  UINT32 i,next;

  RxBufIndex = 0;
  for (i = 0, next = 0; i < NUM_RX_BUF; i++) 
  {
    if (++next == NUM_RX_BUF) next = 0;
    Rx_Desc[i].RBD[IDX(0x00)] = SWE(DESC_RX_E);
    Rx_Desc[i].RBD[IDX(0x04)] = SWE((UINT32)&rx_buf[i] >> 16);
    Rx_Desc[i].RBD[IDX(0x06)] = SWE((UINT32)&rx_buf[i] & 0xFFFF);
    Rx_Desc[i].RBD[IDX(0x08)] = SWE(DESC_RX_INT);
    Rx_Desc[i].RBD[IDX(0x10)] = 0;
  }
  
  Rx_Desc[i-1].RBD[IDX(0)] |= SWE(DESC_RX_W);
  ENET_RDSR = (UINT32)&Rx_Desc[0];
}

/*--------------------------- tx_descr_init ----------------------------------*/
static void tx_descr_init (void) 
{
  /* Initialize Transmit DMA Descriptor array. */
  UINT32 i,next;

  TxBufIndex = 0;
  for (i = 0, next = 0; i < NUM_TX_BUF; i++) 
  {
    if (++next == NUM_TX_BUF) 
    	next = 0;
    	
    Tx_Desc[i].TBD[IDX(0x00)] = SWE(DESC_TX_L);
    Tx_Desc[i].TBD[IDX(0x04)] = SWE(((UINT32)net_tx_buf) >> 16);
    Tx_Desc[i].TBD[IDX(0x06)] = SWE(((UINT32)net_tx_buf) & 0xFFFF);
    Tx_Desc[i].TBD[IDX(0x10)] = 0;
  }
  
  Tx_Desc[i-1].TBD[IDX(0x00)] |= SWE(DESC_TX_W);
  ENET_TDSR = (UINT32)&Tx_Desc[0];
}

static uint8 eth_address_hash(const uint8* addr)
{
    uint32 crc;
    uint8 byte;
    int i, j;

    crc = 0xFFFFFFFF;
    for(i=0; i<6; ++i)
    {
        byte = addr[i];
        for(j=0; j<8; ++j)
        {
            if((byte & 0x01)^(crc & 0x01))
            {
                crc >>= 1;
                crc = crc ^ 0xEDB88320;
            }
            else
                crc >>= 1;
            byte >>= 1;
        }
    }
    return (uint8)(crc >> 26);
}


/*
	设置芯片mac地址；
 */
void eth_address_set (uint8 *pa)
{
    uint8 crc;

    /*
     * Set the Physical Address
     */
    ENET_PALR/*(ch)*/ = (uint32)((pa[0]<<24) | (pa[1]<<16) | (pa[2]<<8) | pa[3]);
    ENET_PAUR/*(ch)*/ = (uint32)((pa[4]<<24) | (pa[5]<<16));

    /*
     * Calculate and set the hash for given Physical Address
     * in the  Individual Address Hash registers
     */
    crc = eth_address_hash(pa);
    if(crc >= 32)
        ENET_IAUR/*(ch)*/ |= (uint32)(1 << (crc - 32));
    else
        ENET_IALR/*(ch)*/ |= (uint32)(1 << crc);
}

/*
	PHY为自协商模式，根据PHY的协商结果，
	调整RMII的速率；
*/
void eth_Spd_Syn(void)
{
	/* Initialize the ETH ethernet controller. */
	UINT32 regv,ctrl;

	/* Check the link status. */
	regv = read_PHY (PHY_REG_BSTAT);
	if(!(regv & 0x0004)) 
	{		  
	    /*link down*/
		return;
	}

	/* Check Operation Mode Indication in PHY Control register */ 
	regv = read_PHY (PHY_REG_PC2);
	/* Configure Full/Half Duplex mode. */
	switch ((regv & 0x001C) >> 2) 
	{
		case 1:  ctrl = PHY_CON_10M  | PHY_CON_HD; break;
		case 2:  ctrl = PHY_CON_100M | PHY_CON_HD; break;
		case 5:  ctrl = PHY_CON_10M  | PHY_CON_FD; break;
		case 6:  ctrl = PHY_CON_100M | PHY_CON_FD; break;
		default: ctrl = 0;				           break;
	}
	
	if ((ctrl & PHY_CON_FD) && ((ENET_TCR & ENET_TCR_FDEN_MASK) != ENET_TCR_FDEN_MASK)) 
	{
		ENET_TCR |= ENET_TCR_FDEN_MASK;	/* Enable Full duplex				  */
	}

	/* Configure 100MBit/10MBit mode. */
	if((ctrl & PHY_CON_100M) && (ENET_RCR & ENET_RCR_RMII_10T_MASK)) 
	{		
		ENET_RCR &= ~ENET_RCR_RMII_10T_MASK; /* 100MBit mode.				  */
	}	
	else
	if((ctrl & PHY_CON_10M) && ((ENET_RCR & ENET_RCR_RMII_10T_MASK) != ENET_RCR_RMII_10T_MASK)) 
	{
		ENET_RCR |= ENET_RCR_RMII_10T_MASK; /* 10MBit mode.				  */
	}
}

/*
	以太网初始化
*/
void eth_init(void)
{
	/* Initialize the ETH ethernet controller. */
	UINT32 regv,tout,id1,id2,ctrl;

	OSC_CR	 |= OSC_CR_ERCLKEN_MASK;	/* Enable external reference clock	  */
	SIM_SCGC2 |= SIM_SCGC2_ENET_MASK;	/* Enable ENET module gate clocking   */
	SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK |	/* Enable Port A module gate clocking */
				 SIM_SCGC5_PORTB_MASK ;	/* Enable Port B module gate clocking */

	//允许并发访问MPU控制器  	
	MPU_CESR = 0; 			 
  	
	/* Reset Ethernet MAC */
	ENET_ECR =  ENET_ECR_RESET_MASK;
	while(ENET_ECR & ENET_ECR_RESET_MASK);

	/* Set MDC clock frequency @ 50MHz MAC clock frequency */
	ENET_MSCR = ENET_MSCR_MII_SPEED(0x13);	

	/* Configure Ethernet Pins  */
	PORTA_PCR5  &= ~(PORT_PCR_MUX_MASK | PORT_PCR_PS_MASK);
	PORTA_PCR5  |=  PORT_PCR_MUX(4);   /* Pull-down on RX_ER is enabled      */	

	PORTA_PCR12  = (PORTA_PCR12 & ~PORT_PCR_MUX_MASK) | PORT_PCR_MUX(4);
	PORTA_PCR13  = (PORTA_PCR13 & ~PORT_PCR_MUX_MASK) | PORT_PCR_MUX(4);
	PORTA_PCR14  = (PORTA_PCR14 & ~PORT_PCR_MUX_MASK) | PORT_PCR_MUX(4);
	PORTA_PCR15  = (PORTA_PCR15 & ~PORT_PCR_MUX_MASK) | PORT_PCR_MUX(4);
	PORTA_PCR16  = (PORTA_PCR16 & ~PORT_PCR_MUX_MASK) | PORT_PCR_MUX(4);
	PORTA_PCR17  = (PORTA_PCR17 & ~PORT_PCR_MUX_MASK) | PORT_PCR_MUX(4);

	PORTB_PCR0   = (PORTB_PCR0  & ~PORT_PCR_MUX_MASK) | PORT_PCR_MUX(4)| PORT_PCR_ODE_MASK;
	PORTB_PCR1   = (PORTB_PCR1  & ~PORT_PCR_MUX_MASK) | PORT_PCR_MUX(4);		

	/* Read PHY ID */
	id1 = read_PHY (PHY_REG_ID1);
	id2 = read_PHY (PHY_REG_ID2);

	/* Check if this is a KSZ8041NL PHY. */
	if (((id1 << 16) | (id2 & 0xFFF0)) == PHY_ID_KSZ8041)
	{
		/* Put the PHY in reset mode */
		write_PHY (PHY_REG_BCTRL, 0x8000);

		/* Wait for hardware reset to end. */
		for (tout = 0; tout < 0x10000; tout++) 
		{
		  regv = read_PHY (PHY_REG_BCTRL);
		  if (!(regv & 0x8800)) 
		  {
			/* Reset complete, device not Power Down. */
			break;
		  }
	    }
	    
		/* Configure the PHY device */
	#if defined (_10MBIT_)
		/* Connect at 10MBit */
		write_PHY (PHY_REG_BCTRL, PHY_FULLD_10M);
	#elif defined (_100MBIT_)
		/* Connect at 100MBit */
		write_PHY (PHY_REG_BCTRL, PHY_FULLD_100M);
	#else
		/* Use autonegotiation about the link speed. */
		write_PHY (PHY_REG_BCTRL, PHY_AUTO_NEG);
		/* Wait to complete Auto_Negotiation. */
		for (tout = 0; tout < 0x10000; tout++) 
		{
		  regv = read_PHY (PHY_REG_BSTAT);
		  if (regv & 0x0020) 
		  {		  
		  	break;							/* Autonegotiation Complete 		  */
		  }
		}
	#endif
	
		/* Check the link status. */
		for (tout = 0; tout < 0x10000; tout++) 
		{
		  regv = read_PHY (PHY_REG_BSTAT);
		  if (regv & 0x0004) 
		  {		  
			break;							/* Link is on						  */
		  }
		}

		/* Check Operation Mode Indication in PHY Control register */ 
		regv = read_PHY (PHY_REG_PC2);
		/* Configure Full/Half Duplex mode. */
		switch ((regv & 0x001C) >> 2) 
		{
		  case 1:  ctrl = PHY_CON_10M  | PHY_CON_HD; break;
		  case 2:  ctrl = PHY_CON_100M | PHY_CON_HD; break;
		  case 5:  ctrl = PHY_CON_10M  | PHY_CON_FD; break;
		  case 6:  ctrl = PHY_CON_100M | PHY_CON_FD; break;
		  default: 
		  		   ctrl = 0;						 
		           break;
		}
		
		if (ctrl & PHY_CON_FD) 
		{
		  ENET_TCR |= ENET_TCR_FDEN_MASK;	/* Enable Full duplex				  */
		}

		/* Configure 100MBit/10MBit mode. */
		if (ctrl & PHY_CON_100M) 
		{		
		  ENET_RCR &= ~ENET_RCR_RMII_10T_MASK; /* 100MBit mode.				  */
		}
	}	

	/* Set receive control */
	ENET_RCR = ENET_RCR_MAX_FL (0x5EE) |
			   ENET_RCR_RMII_MODE_MASK | /* MAC Configured for RMII operation  */
			   ENET_RCR_MII_MODE_MASK  | /* This bit must always be set		  */
			   ENET_RCR_PROM_MASK;
			   //ENET_RCR_CRCFWD_MASK;

	/* Set transmit control */
	ENET_TCR = ENET_TCR_ADDINS_MASK |	/* MAC overwrites the source MAC */
			   ENET_TCR_ADDSEL(0)   ;	/* MAC address is in PADDR 1 and 2 */

	/* Set thresholds */
	ENET_RSFL = 0;					/*Store and forward on the RX FIFO   */
	ENET_RAEM = 4;
	ENET_RAFL = 4;
	
	ENET_TAEM = 4;
	ENET_TAFL = 8;
	ENET_FTRL = 0x7FF;					/* Frame Truncation Length			  */


	/* MAC address filtering, accept multicast packets. */

	/* Set the Ethernet MAC Address registers */
	ENET_PAUR = ENET_PAUR_TYPE (0x8808) |
			    ENET_PAUR_PADDR2(((UINT32)my_hwaddr[4] << 8) | (UINT32)my_hwaddr[5]);
	ENET_PALR = ((UINT32)my_hwaddr[0] << 24) | (UINT32)my_hwaddr[1] << 16 |
			    ((UINT32)my_hwaddr[2] <<  8) | (UINT32)my_hwaddr[3];

	ENET_MIBC |= ENET_MIBC_MIB_CLEAR_MASK;		    
	
	/* Initialize Tx and Rx DMA Descriptors */
	rx_descr_init ();
	tx_descr_init ();
	
	ENET_MRBR = ETH_BUF_SIZE;	

	/* Enable Ethernet, reception and transmission are possible */
	ENET_ECR  |= ENET_ECR_EN1588_MASK | ENET_ECR_ETHEREN_MASK;

	/* Reset all interrupts */
	ENET_EIR = 0x7FFF8000;

	/* Enable receive interrupt */
	ENET_EIMR |= ENET_EIMR_RXF_MASK; 
	
	/* Start MAC receive descriptor ring pooling */
	ENET_RDAR = ENET_RDAR_RDAR_MASK;

	NVIC_IRQ_Enable(ENET_Receive_IRQn); 	   //ENET接收中断	
}

//------------------------------------------------------------------------
// This adds the ethernet header and sends completed Ethernet
// frame to CS8900A.  See "TCP/IP Illustrated, Volume 1" Sect 2.2
//------------------------------------------------------------------------
STATUS eth_send(void *pCookie, UCHAR * outbuf, int len)
{
	int sentlen;
	/* Send frame to ETH ethernet controller */
	//UINT32 *sp,*dp;
	//UINT32 i,j, adr;	
	UINT32 j;

	NET_LOG("eth_send: len %d\r\n", len, 0, 0,0,0,0);
	
	/*以太网包的长度最小为64个字节，包括4个字节FCS，所以说，len最小为60字节*/
	if(len < ETHERSMALL)
	{
		memset(outbuf+len, 0, ETHERSMALL-len);
		len = ETHERSMALL;
	}

	#if 0
	{
		int i;
		for(i=0; i<len; i++)
		{
			CLI_PRINT("%d ",outbuf[i],0,0,0,0,0);
		}
	}
	#endif

	j = TxBufIndex;
	/* Wait until previous packet transmitted. */
	while (Tx_Desc[j].TBD[IDX(0)] & SWE(DESC_TX_R));

	#if 0
	adr  = SWE(Tx_Desc[j].TBD[IDX(4)]) << 16;
	adr |= SWE(Tx_Desc[j].TBD[IDX(6)]);
	dp = (UINT32 *)(adr & ~3);
	//sp = (UINT32 *)&frame->data[0];
	sp = (UINT32*)outbuf;

	/* Copy frame data to ETH IO buffer. */
	//for (i = (frame->length + 3) >> 2; i; i--) 
	for (i=(len + 3)>>2; i; i--) 
	{
		*dp++ = *sp++;
	}
	#else
    Tx_Desc[j].TBD[IDX(0x04)] = SWE((UINT32)outbuf >> 16);
    Tx_Desc[j].TBD[IDX(0x06)] = SWE((UINT32)outbuf & 0xFFFF);		
	#endif
	
	Tx_Desc[j].TBD[IDX(2)]  = SWE(len);//SWE(frame->length);
	Tx_Desc[j].TBD[IDX(0)] |= SWE(DESC_TX_R | DESC_TX_L | DESC_TX_TC);
	Tx_Desc[j].TBD[IDX(0x10)] = 0;

	if (++j == NUM_TX_BUF) 
		j = 0;

	TxBufIndex = j;
	
	/* Start frame transmission. */
	ENET_TDAR = ENET_TDAR_TDAR_MASK;	

	sentlen = len;
	NET_LOG("eth_send: sent len %d\r\n", sentlen, 0, 0,0,0,0);

	eth_tx_pkts++;
	ethDevice.end.outPkts++;	

	return OK;
}

//------------------------------------------------------------------------
// This is the handler for incoming Ethernet frames
//	This is designed to handle standard Ethernet (RFC 893) frames
// See "TCP/IP Illustrated, Volume 1" Sect 2.2
//------------------------------------------------------------------------
STATUS eth_rcve(void)
{
	UINT32 i, adr, RxLen;
	UINT32 *sp;

	NET_LOG("\r\neth_rcve:\r\n",0,0,0,0,0,0);

	eth_rx_pkts++;
	ethDevice.end.inPkts++;	

	i = RxBufIndex;
	while(!(Rx_Desc[i].RBD[IDX(0)] & SWE(DESC_RX_E)))
	{
		if (Rx_Desc[i].RBD[IDX(0)] & SWE(DESC_RX_TR)) 
		{
			/* Frame is truncated */
			NET_LOG("\r\nETH_RCV: frame TR\r\n",0,0,0,0,0,0);
			goto rel;
		}
		RxLen = SWE(Rx_Desc[i].RBD[IDX(2)]);

		if (Rx_Desc[i].RBD[IDX(0)] & SWE(DESC_RX_L)) 
		{
			/* Last in a frame, length includes CRC bytes */
			RxLen -= 4;
			if (Rx_Desc[i].RBD[IDX(0)] & SWE(DESC_RX_OV)) 
			{
				/* Receive FIFO overrun */
				NET_LOG("\r\nETH_RCV: FIFO OR\r\n",0,0,0,0,0,0);
				goto rel;
			}
			if (Rx_Desc[i].RBD[IDX(0)] & SWE(DESC_RX_NO)) 
			{
				/* Received non-octet aligned frame */
				NET_LOG("\r\nETH_RCV: no-octer\r\n",0,0,0,0,0,0);
				goto rel;
			}

			if (Rx_Desc[i].RBD[IDX(0)] & SWE(DESC_RX_CR)) 
			{
				/* CRC or frame error */
				NET_LOG("\r\nETH_RCV: frame crc err\r\n",0,0,0,0,0,0);
				goto rel;
			}  
		}
		else
		{
			NET_LOG("\r\nnot end-of-frame(RxBufIndex:%d)\r\n",RxBufIndex,0,0,0,0,0);
			goto rel;
		}

		if (RxLen > MAX_ETH_LEN) 
		{
			/* Packet too big, ignore it and free buffer. */
			NET_LOG("eth_rcve: unrecognized eth frame(too big)\r\n",0,0,0,0,0,0);
			goto rel;
		}

		adr  = SWE(Rx_Desc[i].RBD[IDX(4)]) << 16;
		adr |= SWE(Rx_Desc[i].RBD[IDX(6)]);
		sp = (UINT32 *)(adr & ~3);

		NET_LOG("eth_rcve: eth frame len %d\r\n", RxLen, 0, 0,0,0,0);

		if((RxLen < MIN_ETH_LEN) || (RxLen > MAX_ETH_LEN))
		{
			NET_LOG("eth_rcve: unrecognized eth frame\r\n",0,0,0,0,0,0);
			goto rel;
		}	

		/*
		送给上层处理
		*/
		END_RCV_RTN_CALL(&ethDevice.end, (UCHAR*)sp, RxLen);	 

		/* Release this frame from ETH IO buffer. */
		rel:Rx_Desc[i].RBD[IDX(0)] |= SWE(DESC_RX_E);
		Rx_Desc[i].RBD[IDX(0x10)] = 0;	

		if (++i == NUM_RX_BUF) 
		i = 0;

		RxBufIndex = i;
	}

	/* Start MAC receive descriptor ring pooling */
	ENET_RDAR = ENET_RDAR_RDAR_MASK;  

	return OK;
}

/*
	以太网数据包处理；
*/
void eth_process(void)
{
	UINT32 event_copy = event;

	/*同步以太网速率和双工模式*/	
	eth_Spd_Syn();

	/*接收到一个以太网包,处理*/
	if((event_copy & EVENT_ETH_INT_RX) == EVENT_ETH_INT_RX)
	{	
		eth_rcve();
		CLR_BIT(event, EVENT_ETH_INT_RX);
		
		//使能中断
		NVIC_IRQ_Enable(ENET_Receive_IRQn); 	   //ENET接收中断
	}	
}

void eth_interrupt(void)
{	
	SET_BIT(event, EVENT_ETH_INT_RX);
	NVIC_IRQ_Disable(ENET_Receive_IRQn);	   //ENET接收中断
	
	/* Clear pending interrupt bits */
	ENET_EIR = ENET_EIR_RXB_MASK | ENET_EIR_RXF_MASK;
}

#else
void eth_interrupt()
{
	return;
}
#endif

