/********************************************************************************************
* FILE:	net.c

* DESCRIPTION:
	网络配置
MODIFY HISTORY:
	2010.7.13            lixingfu   create
********************************************************************************************/

#include "config.h"
#include "net.h"
#include "eth.h"
#include "cli.h"

//
//设备网络配置
//


UINT8 my_hwaddr[6] = {'h', 'c', 0xC0, 0xA8, 0x0B, 0x04};
UINT8 broadcast_hwaddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/*
	IP配置
*/
UINT32 my_ipaddr = 0xC0A80A0A;
UINT32 my_subnetmask = 0xFFFFFF00;
UINT32 gateway_ipaddr = 0xC0A80A01;
UINT32 trap_ipaddr = 0xC0A80A01;
BOOL snmp_trap_addr_setted = FALSE;

BOOL net_debug = FALSE;

#ifdef NET_SUPPORT

//
//以太网接收发送缓冲区
//
//UCHAR net_rx_buf[MAX_OF_NET_BUF];
UCHAR net_tx_buf[MAX_OF_NET_BUF];

/*******************************************************************************
*
* inet_ntoa_b - convert an network address to dot notation, store it in a buffer
*
* This routine converts an Internet address in network format to dotted
* decimal notation.
*
* This routine is identical to the UNIX inet_ntoa() routine
* except that you must provide a buffer of size INET_ADDR_LEN.
*
* EXAMPLE
* The following example copies the string "90.0.0.2" to <pString>:
* .CS
*     struct in_addr iaddr;
*      ...
*     iaddr.s_addr = 0x5a000002;
*      ...
*     inet_ntoa_b (iaddr, pString);
* .CE
*
* RETURNS: N/A
*/

char* inet_ntoa(UINT32 ipaddr)
{
	char *p = (char *)&ipaddr;
  	static char str[16];

#define	UC(b)	(((int)b)&0xff)
	(void) sprintf (str, "%d.%d.%d.%d", UC(p[3]), UC(p[2]), UC(p[1]), UC(p[0]));

	return str;
}

/*******************************************************************************
*
* inet_network - convert an Internet network number from string to address
*
* This routine forms a network address from an ASCII string containing
* an Internet network number.
*
* EXAMPLE
* The following example returns 0x5a:
* .CS
*     inet_network ("90");
* .CE
*
* RETURNS: The Internet address for an ASCII string, or ERROR if invalid.
*/

UINT32 inet_network
    (
    char *inetString           /* string version of inet addr */
    )
{
	UINT32 val, base, n;
	char c;
	UINT32 parts[4], *pp = parts;
	int i;

again:
	val = 0;
	base = 10;

	if (*inetString == '0')
	base = 8, inetString++;
	if (*inetString == 'x' || *inetString == 'X')
	base = 16, inetString++;

	while ((c = *inetString) != '\0')
	{
		if (isdigit ((int) c))
		{
			val = (val * base) + (c - '0');
			inetString++;
			continue;
		}
		if (base == 16 && isxdigit ((int) c))
		{
			val = (val << 4) + (c + 10 - (islower ((int) c) ? 'a' : 'A'));
			inetString++;
			continue;
		}
		break;
	}

	if (*inetString == '.')
	{
		if (pp >= parts + 4)
		{
			return (UINT32)ERROR;
		}
		
		*pp++ = val, inetString++;
		goto again;
	}

	if (*inetString && !isspace ((int) *inetString))
	{
		return (UINT32)ERROR;
	}

	*pp++ = val;
	n = pp - parts;

	if (n > 4)
	{
		return (UINT32)ERROR;
	}

	for (val = 0, i = 0; i < n; i++)
	{
		val <<= 8;
		val |= parts[i] & 0xff;
	}

	return (val);
}

/*
	内存拷贝函数，一次拷贝1个字，而不是一个字节，省时间
*/
void* wordcpy(void *dest, void *src, int bytecount)
{
	int i = 0;

    //
    // Read all but the last WORD into the receive buffer.
    //
	while(i <= (bytecount - 4))
	{
		*(UINT32 *)&((UCHAR *)dest)[i] = *(UINT32 *)&((UCHAR *)src)[i];
		i += 4;
	}

	//
	// Read the last 1, 2, or 3 BYTES into the buffer
	//
	while(i < bytecount)
	{
		((UCHAR *)dest)[i] = ((UCHAR *)src)[i];
		i++;
    }

	return dest;
}

#endif
