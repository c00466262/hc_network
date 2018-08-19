#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdio.h>
#include <stdint.h>
#include "MK60DZ10.h"

/*-----------------ģ��֧�ֿ���------------------*/
#define UART0_SUPPORT
#define CLI_SUPPORT
#define PIT_SUPPORT
#define NET_SUPPORT
#define HCTEL_3AH_OAM_SUPPORT


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>


#include "common.h"
#include "sysinit.h"
#include "mcg.h"
#include "lptmr.h"
#include "cli.h"


/*---------------------------------------------------------
	ϵͳ��Ƶ
----------------------------------------------------------*/
#define SYS_FREQUENCY	(96*1000*1000)	// 96Mhz_

/*
 * System Bus Clock Info
 */
#define K60_CLK             1
#define REF_CLK             XTAL8   /* value isn't used, but we still need something defined */
#define CORE_CLK_MHZ        PLL96      /* 96MHz is only freq tested for a clock input*/

/*---------------------------------------------------------
	��̬�ڴ����,����ߵ�ַ��ʼ
----------------------------------------------------------*/
#define RAM_SIZE   (128*1024)	/*128K RAM*/
#define RAM_DYNAMIC_SIZE  (10*1024)	/*�ɶ�̬����RAM�Ĵ�С10K*/


/*----------------------------------------------------------
	���ڲ�����
	UART0��ΪCLIʹ��
	UART1��Ϊ������ѯʹ��
-----------------------------------------------------------*/
#define CLI_UART				UART5_BASE_PTR
#define UART0_BAUDRATE			9600
#define MAX_UART0_BUF_SIZE		128

#define ENMP_UART
#define UART1_BAUDRATE	38400
#define MAX_UART1_RXTXBUF_SIZE	256


#define cpuLed		GPIOA_PDOR
#define cpuLedMask	0x0800
#define CPU_LED_ON	GPIOA_PDOR &= 0xF7FF;
#define CPU_LED_OFF	GPIOA_PDOR |= 0x0800;


/*------------------------------------------------------------
	event�¼�˵��	
*/
#define EVENT_ETH_INT_RX		0x00000001	//��̫���յ���
#define EVENT_ETH_INT_RXER		0x00000002	//��̫�����մ���
#define EVENT_ETH_INT_RXOF		0x00000004	//��̫������FIFO���
#define EVENT_ETH_INT_TXER		0x00000008	//��̫�����ʹ���

#define EVENT_TCP_RETRANSMIT	0x00000100	//TCP�ش�
#define EVENT_TCP_INACTIVITY	0x00000200	//TCP����

#define EVENT_RESET				0x00000400	//ϵͳ����
#define EVENT_IP_NEW			0x00000800	//�������µ�IP

extern volatile UINT32 event;



#define systemDescr  "		HCTEL K60DN512 Demo"

#define softVersion	10

#define creationDate   __DATE__ "," __TIME__

#endif

