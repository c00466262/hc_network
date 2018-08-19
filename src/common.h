#ifndef __COMMON_H__
#define __COMMON_H__

/*-------------------------------------------------------
	新类型定义
---------------------------------------------------------*/
typedef unsigned char	UCHAR;
typedef unsigned char	UINT8;
typedef short			INT16;
typedef unsigned short	UINT16;
typedef int				INT32;
typedef unsigned int	UINT;
typedef unsigned int	UINT32;
typedef unsigned long	ULONG;
typedef unsigned char	BYTE;
typedef unsigned int	WORD;


/*
 * The basic data types
 */
typedef unsigned char		uint8;  /*  8 bits */
typedef unsigned short int	uint16; /* 16 bits */
typedef unsigned long int	uint32; /* 32 bits */

typedef char			    int8;   /*  8 bits */
typedef short int	        int16;  /* 16 bits */
typedef int		            int32;  /* 32 bits */

typedef volatile int8		vint8;  /*  8 bits */
typedef volatile int16		vint16; /* 16 bits */
typedef volatile int32		vint32; /* 32 bits */

typedef volatile uint8		vuint8;  /*  8 bits */
typedef volatile uint16		vuint16; /* 16 bits */
typedef volatile uint32		vuint32; /* 32 bits */


typedef	int	STATUS;
typedef	char BOOL;


//BOOL Constants
#define TRUE 1
#define FALSE 0

//STATUS Constants
#define OK 0
#define ERROR (-1)

#define UP		1
#define DOWN	0

#define LOCAL static

#define NO_WAIT			0
#define WAIT_FOREVER	-1

/*-------------------------------------------------------------------------
					 trace info level 
--------------- ---------------------------------------------------------- */
#define TRACE_LEVEL_OFF			0
#define TRACE_LEVEL_ERROR		1
#define TRACE_LEVEL_WARN		2
#define TRACE_LEVEL_INFO		3

extern int trace_level;

#define trace(level, fmt, arg1, arg2, arg3, arg4, arg5, arg6) \
	if(level <= trace_level)\
	{\
		CLI_PRINT(fmt, arg1, arg2, arg3, arg4, arg5, arg6);\
	}



/*-------------------------------------------------------
	bit操作定义
-------------------------------------------------------*/
#define SET_BIT(var, bitmask)	( (var) |= (bitmask) )
#define CLR_BIT(var, bitmask)	( (var) &= ~(bitmask) )
#define IS_SET(var, bitmask)	( (var) & (bitmask))
#define SET_BITS(var, bitsmask, bits) (var)=(((var)&(~(bitsmask)))|((bits) & (bitsmask)))


#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

#define ARRAY_LEN(array) 	((int)(sizeof(array)/sizeof(array[0]))


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
    #define htons(a)                   \
         ((((a) >> 8) & 0x00ff) |    \
         (((a) << 8) & 0xff00))
#endif

#ifndef ntohs
    #define ntohs(a)    htons((a))
#endif

typedef enum IRQn
{
/* -------------------  Cortex-M4 Processor Exceptions Numbers  ------------------- */
	NonMaskableInt_IRQn           = -14,      /*!<  2 Non Maskable Interrupt          */
	HardFault_IRQn                = -13,      /*!<  3 HardFault Interrupt             */
	MemoryManagement_IRQn         = -12,      /*!<  4 Memory Management Interrupt     */
	BusFault_IRQn                 = -11,      /*!<  5 Bus Fault Interrupt             */
	UsageFault_IRQn               = -10,      /*!<  6 Usage Fault Interrupt           */
	SVCall_IRQn                   =  -5,      /*!< 11 SV Call Interrupt               */
	DebugMonitor_IRQn             =  -4,      /*!< 12 Debug Monitor Interrupt         */
	PendSV_IRQn                   =  -2,      /*!< 14 Pend SV Interrupt               */
	SysTick_IRQn                  =  -1,      /*!< 15 System Tick Interrupt           */

/* ----------------------  ARMCM4 Specific Interrupt Numbers  --------------------- */	
	UART5_STA_IRQn                =   55,      /*!< UART4 tx/rx Interrupt                    */
	UART5_ERR_IRQn                =   56,      /*!< UART4 error Interrupt                    */
	ENET_Transmit_IRQn			  =   76,
	ENET_Receive_IRQn		      =   77,
	ENET_Error_IRQn				  =   78,

	PIT0_IRQn                     =	  68,               /**< PIT timer channel 0 interrupt */
	PIT1_IRQn                     =   69,               /**< PIT timer channel 1 interrupt */
	PIT2_IRQn                     =   70,               /**< PIT timer channel 2 interrupt */
	PIT3_IRQn                     =   71,               /**< PIT timer channel 3 interrupt */	
}IRQn_Type;

/*使能对应中断通道*/
#define NVIC_IRQ_Enable(IRQn)	  	NVIC_ISER((uint32)((uint32)(IRQn) >> 5)) \
			 		= (uint32)(1 << ((uint32)((uint32)(IRQn) & (uint32)(0x1F))))

/*禁止对应中断通道*/
#define NVIC_IRQ_Disable(IRQn)	  	NVIC_ICER(((uint32)(IRQn) >> 5)) = (1 << ((uint32)(IRQn) & 0x1F)) /* disable interrupt */	 		

#endif

