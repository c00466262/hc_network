/********************************************************************************************
* FILE:	clicmddef.c

* DESCRIPTION:
	CLI定义
MODIFY HISTORY:
	2010.7.13            lixingfu   create
********************************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "config.h"
#include "cli.h"
#include "clicmd.h"
#include "clicmddef.h"
#include "eth.h"
#include "net.h"
#include "hctel_3ah_oam.h"

#ifdef CLI_SUPPORT

#define argc		usrCmdParaStruct->argc 
#define argv		usrCmdParaStruct->argv 
#define retInfo		usrCmdParaStruct->retInfo


BOOL isdigitalstr(char * str)
{
	int i;
	
	for(i = 0; i < strlen(str); i++)
	{
		if(!isdigit(str[i]))
	 		return FALSE;
	}
	return TRUE;
}

/*把一个长整型数(4个字节)转化到缓冲区中*/
char* ltoa(long i)
{
	static char buf[12];/*整型,结尾必须是-1,294,967,295'\0'*/
	BOOL negative;
	char *ptr;

	//|0|1|2|3|4|5|6|7|8|9|10|11('\0')|

	memset(buf, 0, 12);
	
	if(i == 0)/*i == 0*/
	{
		buf[10] = '0';
		return &buf[10];
	}
	
	if(i < 0)
	{
		negative = TRUE;
		i /= -1;/*除去负号*/
	}
	else
	{
		negative = FALSE;
	}
	/*i > 0*/
	ptr = buf+11;/*指向buf的末尾*/
	
	while(i != 0)
	{
		*(--ptr) = i%10 + 0x30;
		i /= 10;
	}
	
	if(negative)
		*(--ptr) = '-';
	
	return ptr;
}

/*缓冲区中的十六进制字符串转化成无符号长整型*/
/*"0x2345" -> 0x2345*/
unsigned int axtoi(char *str)
{
	unsigned int result = 0;

	char *p =  NULL;

	if(str == NULL)
	{
		return 0;
	}
	else
	if(str[0] == 'x' || str[0] == 'X')
	{
		p = str+1;
	}
	else
	if(str[0] == '0')
	{
		if(str[1] == 'x' || str[1] == 'X')
		{
			p = str+2;
		}
		else
		{
			p = str;
		}
	}
	else
	{
		p = str;
	}

	while(*p != '\0')
	{
		if('0' <= *p && *p <= '9')
			result = result * 16 + (*p - 0x30);
		else
		if('a' <= *p && *p <= 'f')
			result = result * 16 + (*p - 0x61 + 10);
		else
		if('A' <= *p && *p <= 'F')
			result = result * 16 + (*p - 0x41 + 10);
		else
			return result;			
		p++;
	}

	return result;
}

/*字符串转化成long型*/
unsigned long axtol(char *str)
{
	unsigned long result = 0;

	char *p =  NULL;

	if(str == NULL)
	{
		return 0;
	}
	else
	if(str[0] == 'x' || str[0] == 'X')
	{
		p = str+1;
	}
	else
	if(str[0] == '0')
	{
		if(str[1] == 'x' || str[1] == 'X')
		{
			p = str+2;
		}
		else
		{
			p = str;
		}
	}
	else
	{
		p = str;
	}

	while(*p != '\0')
	{
		if('0' <= *p && *p <= '9')
			result = result * 16 + (*p - 0x30);
		else
		if('a' <= *p && *p <= 'f')
			result = result * 16 + (*p - 0x61 + 10);
		else
		if('A' <= *p && *p <= 'F')
			result = result * 16 + (*p - 0x41 + 10);
		else
			return result;			
		p++;
	}

	return result;
}


char cli_show_run(USRCMD_PARA_STRUCT *usrCmdParaStruct)
{
	/*----3. 网络接口----------------------------*/
#ifdef NET_SUPPORT
	CLI_PRINT("interface nm:\r\n",0,0,0,0,0,0);
	CLI_PRINT("mac address: %02X:%02X:%02X",my_hwaddr[0],my_hwaddr[1],my_hwaddr[2],0,0,0); 
	CLI_PRINT(":%02X:%02X:%02X\r\n",my_hwaddr[3], my_hwaddr[4], my_hwaddr[5],0,0,0);
	CLI_PRINT("ip address : %s\r\n", (int)inet_ntoa(my_ipaddr),0,0,0,0,0); 
	CLI_PRINT("subnet mask: %s\r\n", (int)inet_ntoa(my_subnetmask),0,0,0,0,0);		
	CLI_PRINT("gateway	  : %s\r\n",(int)inet_ntoa(gateway_ipaddr),0,0,0,0,0);
	CLI_PRINT("exit\r\n\r\n",0,0,0,0,0,0);
#endif

	return CLIRET_OK;
}

#ifdef HCTEL_3AH_OAM_SUPPORT
LOCAL void dot3ah_print_info(BOOL boolFlag, char* str1, char* str2)
{
	if(boolFlag)
	{
		CLI_PRINT(" %s", (int)str1, 0, 0, 0, 0, 0);
	}
	else
	{
		CLI_PRINT(" %s", (int)str2, 0, 0, 0, 0, 0);
	}
}

char cli_show_dot3ah(USRCMD_PARA_STRUCT *usrCmdParaStruct)
{
	char *stringPtr = NULL;
	
	/*------dot3ah Info----------------------*/
	CLI_PRINT("\r\ndot3ah info:", 0, 0, 0, 0, 0, 0);	

	/*dot3ah 工作使能*/
	CLI_PRINT("\r\ndot3ah state:", 0, 0, 0, 0, 0, 0);	
	dot3ah_print_info(dot_3AH_Para.dot_3ah_Enable, "enable", "disable");

	/*discovery 成功次数*/
	CLI_PRINT("\r\ndiscovery count: %d", dot_3AH_Para.discoveryIsAnyCout, 0, 0, 0, 0, 0);	
	
	/*discovery 状态*/
	CLI_PRINT("\r\ndiscovery state:", 0, 0, 0, 0, 0, 0);	
	switch(dot_3AH_Para.discoverySta)
	{
		case OAM_DISCV_FAULT:
		{
			stringPtr = "fault";
			break;
		}
		
		case OAM_DISCV_ACTIVE_SEND_LOCAL:
		{
			stringPtr = "active_send_local";
			break;
		}

		case OAM_DISCV_PASSIVE_WAIT:
		{
			stringPtr = "passive_wait";
			break;
		}

		case OAM_DISCV_SEND_LOCAL_REMOTE:
		{
			stringPtr = "send_local_remote";
			break;
		}

		case OAM_DISCV_SEND_LOCAL_REMOTE_OK:
		{
			stringPtr = "send_local_remote_ok";
			break;
		}

		case OAM_DISCV_SEND_ANY:
		{
			stringPtr = "any";
			break;
		}
	}
	CLI_PRINT(" %s", (int)stringPtr, 0, 0, 0, 0, 0);

	/*dot3ah工作模式*/
	CLI_PRINT("\r\ndot3ah mode:", 0, 0, 0, 0, 0, 0);	
	dot3ah_print_info(dot_3AH_Para.localInfo.modeCfg, "active", "passive");

	/*端口发送状态*/
	CLI_PRINT("\r\nmultiplexer state:", 0, 0, 0, 0, 0, 0);	
	dot3ah_print_info((dot_3AH_Para.localInfo.muxSta == OAM_MUX_FWD), "fowording", "discard");

	/*端口接收状态*/
	CLI_PRINT("\r\nparser state:", 0, 0, 0, 0, 0, 0);
	if(dot_3AH_Para.localInfo.parSta == OAM_PAR_FWD)
	{
		stringPtr = "fowording";
	}
	else
	if(dot_3AH_Para.localInfo.parSta == OAM_PAR_LB)
	{
		stringPtr = "loopback";
	}
	else
	if(dot_3AH_Para.localInfo.parSta == OAM_PAR_DISCARD)
	{
		stringPtr = "discard";
	}
	CLI_PRINT(" %s", (int)stringPtr, 0, 0, 0, 0, 0);

	/*参数配置*/
	CLI_PRINT("\r\nloopback rsp:", 0, 0, 0, 0, 0, 0);
	dot3ah_print_info(dot_3AH_Para.localInfo.lbCfg, "enable", "disable");

	CLI_PRINT("\r\nlkevt state:", 0, 0, 0, 0, 0, 0);
	dot3ah_print_info(dot_3AH_Para.localInfo.lkevtCfg, "enable", "disable");

	CLI_PRINT("\r\n---------------------", 0, 0, 0, 0, 0, 0);

	/*dot3ah工作模式*/
	CLI_PRINT("\r\nremote dot3ah mode:", 0, 0, 0, 0, 0, 0);	
	dot3ah_print_info(dot_3AH_Para.remoteInfo.modeCfg, "active", "passive");

	/*端口发送状态*/
	CLI_PRINT("\r\nremote multiplexer state:", 0, 0, 0, 0, 0, 0);	
	dot3ah_print_info((dot_3AH_Para.remoteInfo.muxSta == OAM_MUX_FWD), "fowording", "discard");

	/*端口接收状态*/
	CLI_PRINT("\r\nremote parser state:", 0, 0, 0, 0, 0, 0);
	if(dot_3AH_Para.remoteInfo.parSta == OAM_PAR_FWD)
	{
		stringPtr = "fowording";
	}
	else
	if(dot_3AH_Para.remoteInfo.parSta == OAM_PAR_LB)
	{
		stringPtr = "loopback";
	}
	else
	if(dot_3AH_Para.remoteInfo.parSta == OAM_PAR_DISCARD)
	{
		stringPtr = "discard";
	}
	CLI_PRINT(" %s", (int)stringPtr, 0, 0, 0, 0, 0);

	/*参数配置*/
	CLI_PRINT("\r\nremote loopback rsp:", 0, 0, 0, 0, 0, 0);
	dot3ah_print_info(dot_3AH_Para.remoteInfo.lbCfg, "enable", "disable");

	CLI_PRINT("\r\nremote lkevt state:", 0, 0, 0, 0, 0, 0);
	dot3ah_print_info(dot_3AH_Para.remoteInfo.lkevtCfg, "enable", "disable");	

	CLI_PRINT("\r\nexit", 0, 0, 0, 0, 0, 0);	
	return CLIRET_OK;
}

char cli_set_rmt_lb(USRCMD_PARA_STRUCT *usrCmdParaStruct)
{
	if(argc != 3)
	{
		sprintf(retInfo,"\r\nusage: rmt lb <\"enable\"|\"disable\">\r\n"); 
		return CLIRET_OK;
	}

	if(strcmp(argv[2], "enable") == 0)
	{
		hctel_oamLoopbackCtrl(TRUE); 
	}
	else
	if(strcmp(argv[2], "disable") == 0)
	{
		hctel_oamLoopbackCtrl(FALSE); 
	}	
	else
	{
		sprintf(retInfo,"\r\nusage: rmt lb <\"enable\"|\"disable\">\r\n"); 
		return CLIRET_OK;		
	}
	
	return CLIRET_OK;
}

#endif

char cli_show_version(USRCMD_PARA_STRUCT *usrCmdParaStruct)
{
	int j = 0;
	
	j = sprintf(retInfo, "\r\nSoftware Version %d.%d, CPU: LM3S9B90 %s\r\n", (int)softVersion/10,(int)softVersion%10,creationDate);

#ifdef FPGA_SUPPORT
	j = sprintf(retInfo+j,"fpga version %d.%d", fpgaVersion/10, fpgaVersion%10);
#endif

	return CLIRET_OK;
}

/*
	set mac 68 63 11 22 33 44
或者set mac 68:63:11:22:33:44
*/
char cli_set_mac(USRCMD_PARA_STRUCT *usrCmdParaStruct)
{
#ifdef ETH_SUPPORT
	int i, j;

	if(argc != 8)
	{
		sprintf(retInfo,"\r\nusage: set mac <BYTE6 - BYTE1>\r\n"); 
		return CLIRET_OK;
	}

	/*读取MAC*/
	for(i = min(argc-1, 7), j = 5; (i >= 2) && (j >=0) ; i-- , j--)
	{
		int temp;
		sscanf(argv[i], "%x",  &temp);
		my_hwaddr[j] = (char)temp;
	}
	
	eth_address_set(my_hwaddr);
#endif

	return CLIRET_OK;
}

char cli_set_ip(USRCMD_PARA_STRUCT *usrCmdParaStruct)
{
#ifdef ETH_SUPPORT
	UINT32 ip, mask;
	int i, j;

	if(argc != 4)
	{
		sprintf(retInfo,"\r\nusage: set ip <ip-ddress> <subnet-mask>\r\n"); 
		return CLIRET_OK;
	}

	ip = inet_network(argv[2]);
	mask = inet_network(argv[3]);
	
	if((ip == (UINT32)ERROR) || (mask == (UINT32)ERROR))
	{
		sprintf(retInfo,"Illegal network number");
		return CLIRET_OK;
	}

	my_ipaddr = ip;
	my_subnetmask = mask;

	/*让MAC地址自动跟随IP，这样有助于工程*/
	for(i=0, j = 2; i<4 && j<6; i++, j++)
	{
		my_hwaddr[j] = ((UINT8 *)&my_ipaddr)[3-i];
	}
	
	eth_address_set(my_hwaddr);
#endif

	return CLIRET_OK;
}

char cli_set_gateway(USRCMD_PARA_STRUCT *usrCmdParaStruct)
{
#ifdef IP_SUPPORT
	UINT32 gateway;

	if(argc != 3)
	{
		sprintf(retInfo,"\r\nusage: set gateway <gateway>\r\n"); 
		return CLIRET_OK;
	}
	
	gateway = inet_network(argv[2]);
	
	if(gateway == (UINT32)ERROR)
	{
		sprintf(retInfo,"Illegal network number");
		return CLIRET_OK;
	}

	gateway_ipaddr = gateway;
#endif

	return CLIRET_OK;
}


/*调试接口*/

extern BOOL net_debug;

char cli_debug_net(USRCMD_PARA_STRUCT *usrCmdParaStruct)
{	
	net_debug = TRUE;
	return CLIRET_OK;
}

char cli_no_debug_net(USRCMD_PARA_STRUCT *usrCmdParaStruct)
{
	net_debug = FALSE;
	return CLIRET_OK;	
}

#ifdef HCTEL_3AH_OAM_SUPPORT
extern BOOL dot3ah_debug;

char cli_debug_dot3ah(USRCMD_PARA_STRUCT *usrCmdParaStruct)
{	
	dot3ah_debug = TRUE;
	return CLIRET_OK;
}

char cli_no_debug_dot3ah(USRCMD_PARA_STRUCT *usrCmdParaStruct)
{
	dot3ah_debug = FALSE;
	return CLIRET_OK;	
}
#endif


/*查看mii寄存器*/
char cli_get_mii(USRCMD_PARA_STRUCT *usrCmdParaStruct)
{
	int regAddr;
	UINT16 regVal;
	
	if(argc > 3)
	{
		sprintf(retInfo,"\r\nusage: get mii <reg>\r\n"); 
		return CLIRET_OK;
	}	

	regAddr = axtoi(argv[2]);
	
	regVal = read_PHY(regAddr);

	sprintf(retInfo,"\r\nget mii reg(%x) value = %x\r\n",regAddr, regVal); 
	return CLIRET_OK;
}

/*设置mii寄存器*/
char cli_set_mii(USRCMD_PARA_STRUCT *usrCmdParaStruct)
{
	int regAddr;
	UINT16 regVal;
	
	if(argc > 4)
	{
		sprintf(retInfo,"\r\nusage: set mii <reg> <value>\r\n"); 
		return CLIRET_OK;
	}	

	regAddr = axtoi(argv[2]);
	regVal = axtoi(argv[3]);

	write_PHY(regAddr,regVal);
	sprintf(retInfo,"\r\nset mii reg(%x) value = %x ok!\r\n",regAddr, regVal); 
	
	return CLIRET_OK;
}

char cli_show_reg_arm(USRCMD_PARA_STRUCT *usrCmdParaStruct)
{	
	UINT32 bits32;
	UINT32 reg;
		
	if(argc < 3)
	{
		sprintf(retInfo,"\r\nusage: show armreg <reg>\r\n");
		return CLIRET_OK;
	}

	reg = axtol(argv[2]);
	//bits8 = g_pusEPIMem[reg];
	//bits8 = *FPGA_REG8(reg);
	//bits8 = *((UINT8*)(0x00011c04));
	bits32 = *((UINT32*)(reg));
	sprintf(retInfo,"show armreg 0x%x value = %ld(0x%x)\r\n", reg, bits32, bits32);
	return CLIRET_OK;	
}

#endif
