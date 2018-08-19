//-----------------------------------------------------------------------------
// NET.H
//
//-----------------------------------------------------------------------------
#ifndef	__NET_H__
#define __NET_H__


//*****************************************************************************
//
// Helper Macros for Ethernet Processing
//
//*****************************************************************************
//
// htonl/ntohl - big endian/little endian byte swapping macros for
// 32-bit (long) values
//
//*****************************************************************************
#ifndef htonl
    #define htonl(a)                    \
        ((((a) >> 24) & 0x000000ff) |   \
         (((a) >>  8) & 0x0000ff00) |   \
         (((a) <<  8) & 0x00ff0000) |   \
         (((a) << 24) & 0xff000000))
#endif

#ifndef ntohl
    #define ntohl(a)    htonl((a))
#endif

//*****************************************************************************
//
// htons/ntohs - big endian/little endian byte swapping macros for
// 16-bit (short) values
//
//*****************************************************************************
#ifndef htons
    #define htons(a)                \
        ((((a) >> 8) & 0x00ff) |    \
         (((a) << 8) & 0xff00))
#endif

#ifndef ntohs
    #define ntohs(a)    htons((a))
#endif

/*
 * Socket address, internet style.
*/
typedef struct
{
	UINT32 sin_addr;
	UINT16 sin_port;
}__attribute__ ((packed))sockaddr_in;


// Port numbers 
#define ECHO_PORT					   	7
#define DAYTIME_PORT				    13
#define CHARGEN_PORT				    19
#define TIME_PORT						37
#define HTTP_PORT  				    	80
#define SNMP_PORT                       161
#define SNMP_TRAP_PORT                  162   
#define TFTP_PORT						69



// Type number field in Ethernet frame
#define ETHERTYPE_IP		0x0800
#define ETHERTYPE_ARP		0x0806
#define ETHERTYPE_RARP		0x8035
#define	ETHERTYPE_CTP		0x9000	/* loopback protocol */
#define ETHERTYPE_VLAN		0x8100

// Protocol identifier field in IP datagram
#define ICMP_TYPE             	1
#define IGMP_TYPE				2
#define TCP_TYPE              	6
#define UDP_TYPE              	17

// Message type field in ARP messages 
#define ARP_REQUEST           	1
#define ARP_RESPONSE          	2
#define RARP_REQUEST		    3
#define RARP_RESPONSE         	4


// Hardware type field in ARP message
#define DIX_ETHERNET          	1
#define IEEE_ETHERNET         	6

typedef struct
{
   UINT32	ipaddr;
   UINT8	hwaddr[6];
   //UCHAR timer;//不再使用timer,采用轮转替换
} __attribute__ ((packed)) ARP_CACHE;//ARP-IP地址转换表

typedef struct
{
  UINT8 dest_hwaddr[6];    //6*8=48位
  UINT8 source_hwaddr[6];  //6*8=48位
  UINT16  frame_type;
} __attribute__ ((packed)) ETH_HEADER;//link layer header


typedef struct
{
   UINT16	hardware_type; 
   UINT16	protocol_type;           
   UINT8	hwaddr_len;
   UINT8	ipaddr_len;               
   UINT16	message_type;
   UINT8	source_hwaddr[6];              
   UINT32	src_ipaddr;
   UINT8	dest_hwaddr[6];    
   UINT32	dest_ipaddr;
} __attribute__ ((packed)) ARP_HEADER;


typedef struct
{
   UINT8 ver_len;
   UINT8 type_of_service;
   UINT16  total_length;
   UINT16  identifier;
   UINT16  fragment_info;
   UINT8 time_to_live;
   UINT8 protocol_id;
   UINT16  header_cksum;
   UINT32 src_ipaddr;
   UINT32 dest_ipaddr;
} __attribute__ ((packed)) IP_HEADER;


typedef struct
{
   UINT8 msg_type;
   UINT8 msg_code;
   UINT16  checksum;
   UINT16  identifier;
   UINT16  sequence;
   UINT8 echo_data;
} __attribute__ ((packed)) ICMP_HEADER;

#define PING_HEADER_LEN		8

typedef struct
{
   UINT8 msg_type;
   UINT8 msg_code;
   UINT16  checksum;
   UINT32 msg_data;
   UINT8 echo_data;
} __attribute__ ((packed)) ICMP_ERR_HEADER;


typedef struct 
{
   UINT16  src_port;
   UINT16  dest_port;
   UINT16  length;
   UINT16  checksum;
   UINT8 msg_data;
} __attribute__ ((packed)) UDP_HEADER;


typedef struct
{
   UINT16  src_port;
   UINT16  dest_port;
   UINT32 sequence;//到最大值后，从0开始
   UINT32 ack_number;
   UINT16  flags;
   UINT16  window;
   UINT16  checksum;
   UINT16  urgent_ptr;
   UINT8 options;
} __attribute__ ((packed)) TCP_HEADER;


typedef struct
{
	BOOL valid;
	sockaddr_in his_addr;	/* Where this packet came from. */
	sockaddr_in my_addr;	/* Where this packet came to.   */

	UINT32 his_sequence;    //
	UINT32 my_sequence;     //正常发送 
	UINT32 old_sequence;    //用于重发,如果对方需要重发，从这个序号开始
	UINT32 his_ack;         //对方送过来的ack
	
	UINT32 timer;           //该连接重传计时器
	UINT32 inactivity;	    //不活动状态时间计数器,30秒没有数据通过，则自动关闭连接
	UINT8 state;
	UINT8 query[20];       //应用层保存TCP头的前20个字节，保持足够的信息用来重发数据
} __attribute__ ((packed)) TCP_CONNECTION;            //TCP连接


//============================================================

extern UINT8  my_hwaddr[];
extern UINT8  broadcast_hwaddr[]; 
extern UINT32  my_ipaddr;
extern UINT32  my_subnetmask;
extern UINT32  gateway_ipaddr;
extern UINT32  trap_ipaddr;
extern BOOL snmp_trap_addr_setted;

//=============================================================
//             协议支持情况选项
//

#define ETH_SUPPORT
#define ARP_SUPPORT
#define IP_SUPPORT
#define ICMP_SUPPORT
#define UDP_SUPPORT
//#define TFTP_SUPPORT
//#define SNMP_SUPPORT   
//#define SNMP_AGENT_SUPPORT


#define TCP_SUPPORT	
//#define CHECK_COMMUNITY
#define HTTP_SUPPORT
#define WEB_SERVER_SUPPORT

#ifndef NET_SUPPORT
#undef ETH_SUPPORT
#undef ARP_SUPPORT
#undef IP_SUPPORT
#undef ICMP_SUPPORT
#undef UDP_SUPPORT
#undef TCP_SUPPORT
#undef SNMP_SUPPORT
#undef SNMP_AGENT_SUPPORT
#undef HTTP_SUPPORT
#undef WEB_SERVER_SUPPORT
#undef TFTP_SUPPORT
#endif

//--------     TCP 最大连接数目   
#define MAX_OF_TCP_CONNS  10


#define LINK_HEAD_LEN 14    //link layer length
#define ETHERSMALL	  60
#define IP_HEAD_LEN   20 
#define TCP_HEAD_LEN  20
#define UDP_HEAD_LEN  8

#define MAX_ETH_LEN 1514 //单个以太网包的最大长度
#define MIN_ETH_LEN 60 //单个以太网包的最小长度

#define MAX_COMMUNITY_LEN	32

/*每一个以太网包大小为MAX_ETH_LEN,在TCP中,可以发送一个大于MAX_ETH_LEN的包,TCP会自动分包*/
#define MAX_OF_NET_BUF		3000//1536

//extern UCHAR net_rx_buf[MAX_OF_NET_BUF];
extern UCHAR net_tx_buf[MAX_OF_NET_BUF];

extern BOOL net_debug;

#define NET_LOG(fmt, arg1, arg2, arg3, arg4, arg5, arg6) \
{ \
	if(net_debug)\
	{\
		CLI_PRINT(fmt, arg1, arg2, arg3, arg4, arg5, arg6);\
	}\
}
char* inet_ntoa(UINT32 ipaddr);
UINT32 inet_network(char *inetString);
void *wordcpy(void *dest, void *src, int bytecount);

//*******************************************8
#endif /* __NET_H__ */
