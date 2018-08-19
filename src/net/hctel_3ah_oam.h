#ifndef __HCTEL_3AH_OAM_H__
#define __HCTEL_3AH_OAM_H__

#include "config.h"

#define OAM_TYPE_802_3AH	0x8809
#define DOT_3AH_SUB_TYPE	0x03
#define TLV_NUM_IN_ONE_OAMPDU	3  /*执行协议栈允许在单个Infomaion OAMPDU中包含的TLV数量*/

#define OAM_OUI_B0 0X00    /*hctel OUI 00-25-7C*/
#define OAM_OUI_B1 0X25
#define OAM_OUI_B2 0X7C


/*****************************************************************************
						802.3ah协议数据包结构体定义
*****************************************************************************/
typedef struct
{
	UINT8 TLVType;
        #define OAM_INFOTLV_END         0X00
        #define OAM_INFOTLV_LOCAL       0X01
        #define OAM_INFOTLV_REMOTE   	0X02
        #define OAM_INFOTLV_ORGSPECI 	0XFE 	
	UINT8 info_TLV_len; 
		#define	OAM_INFOTLV_LEN			0x10
	UINT8 oamVersion;
	UINT16 oamRevision;
	UINT8 oamState;
	UINT8 oamCfg;
	UINT16 oamPDUCfg;
	UINT8 oamOUI[3];
	UINT8 oamVenderSpeci[4];
}__attribute__ ((packed))Dot_3AH_Info_TLV_Type;

#define OAM_VER_IN_TLV		0
#define REVISION_IN_TLV		1
#define STATE_IN_TLV		3
	#define MUX_STA_MASK	0x04
	#define PAR_STA_MASK	0x03
#define OAM_CFG_IN_TLV		4
	#define VAR_CFG_MASK	0x10
	#define LKVET_CFG_MASK	0x08
	#define LB_CFG_MASK		0x04
	#define UNDIR_CFG_MASK	0x02
	#define OAM_MODE_MASK	0x01
#define PDU_CFG_IN_TLV  	5
	#define MAX_OAMPDU_SIZE_CFG_MASK	0x07FF
#define OUI_IN_TLV			7
#define VENDOR_INF_IN_TLV	10

typedef struct
{
	UINT8 	dst[6];   
	UINT8 	src[6];
	UINT16  proType;
	UINT8   subType;
	UINT16  flags;
        #define OAM_FLAGS_REMOTE_STABLE     	(0x1 << 6)
        #define OAM_FLAGS_REMOTE_EVALUATING     (0x1 << 5)
        #define OAM_FLAGS_LOCAL_STABLE 			(0x1 << 4)
        #define OAM_FLAGS_LOCAL_EVALUATING 		(0x1 << 3)
        #define OAM_FLAGS_CRITICAL_EVENT 		(0x1 << 2)
        #define OAM_FLAGS_DYING_GASP 			(0x1 << 1)
        #define OAM_FLAGS_LINK_FAULT 			(0x1 << 0)	
	UINT8 	code;
        #define OAM_CODE_INFORMATION 0x0
        #define OAM_CODE_EVENTNOTIFI 0x1
        #define OAM_CODE_VARREQ 0x2
        #define OAM_CODE_VARRES 0x3
        #define OAM_CODE_LOOPCTL 0x4
              #define OAM_LOOPCMD_ENABLE  0x1
              #define OAM_LOOPCMD_DISABLE 0x2
        #define OAM_CODE_ORGSPECI 0xfe
        #define OAM_CODE_RESERVED 0xff	
}__attribute__ ((packed))Dot_3AH_Pkt_Head_Type;

enum DOT_3AH_OAMPDU_TYPE
{
	OAM_INFOPDU_NOTLV = 0,
	OAM_INFOPDU_LOCALTLV,
	OAM_INFOPDU_LOCAL_RMT_TLV,
    OAM_LBPDU_ENABLE,
    OAM_LBPDU_DISABLE,
};

/*****************************************************************************
						802.3ah协议变量、参数定义
*****************************************************************************/
/*OAMPUD 类型定义*/
#define DOT_3AH_INFO_OAMPDU					0x00
#define DOT_3AH_EVT_NOTIFICATION_OAMPDU		0x01
#define DOT_3AH_VAR_REQ_OAMPDU				0x02
#define DOT_3AH_VAR_RSP_OAMPDU				0x03
#define DOT_3AH_LB_OAMPDU					0x04
#define DOT_3AH_ORG_OAMPDU					0xFE

#define LOCAL_INFOMATION_TLV_LEN			0x10 /*Local Infomation TLV Lengh*/

enum DOT_3AH_DISCOVERY_STATE 
{
	OAM_DISCV_FAULT = 0,
	OAM_DISCV_ACTIVE_SEND_LOCAL,
	OAM_DISCV_PASSIVE_WAIT,
	OAM_DISCV_SEND_LOCAL_REMOTE,
	OAM_DISCV_SEND_LOCAL_REMOTE_OK,
	OAM_DISCV_SEND_ANY,
};

enum  DOT_3AH_LOCAL_PDU 
{
    OAM_LF_INFO = 0,
    OAM_RX_INFO, 
    OAM_INFO,
    OAM_ANY,
};

enum DOT_3AH_MUX_ACTION 
{
    OAM_MUX_FWD = 0,
    OAM_MUX_DISCARD,
};

enum DOT_3AH_PAR_ACTION 
{
    OAM_PAR_FWD = 0,
    OAM_PAR_LB, 
    OAM_PAR_DISCARD,
    OAM_PAR_RESV,
};

enum DOT_3AH_MODE
{
    OAM_PASSIVE = 0,
    OAM_ACTIVE,
};


typedef struct 
{
    UINT16 infoRevision;
    enum DOT_3AH_MUX_ACTION muxSta;
    enum DOT_3AH_PAR_ACTION parSta;
    BOOL varCfg;		/*TRUE:enable,FALSE:disable*/
    BOOL lkevtCfg; 	/*TRUE:enable,FALSE:disable*/
    BOOL lbCfg;     	/*TRUE:enable,FALSE:disable*/
    BOOL unidirCfg; 	/*TRUE:enable,FALSE:disable*/
    UINT8 modeCfg;		/*ACTIVE or PASSIVE*/
    UINT8 pduConfig[2];
    UINT8 oui[3];
    UINT8 vendSpeci[4];    
} DOT_3AH_Info_Type;

typedef struct
{
    UINT8 remote_stable;
    UINT8 remote_evaluating;
    UINT8 local_stable;
    UINT8 local_evaluating;
    UINT8 critical_event;/*When a critical event has occurred, it will be set true*/
    UINT8 dying_gasp;   /*When an unrecoverable local failure condition has occurred, it will be set true*/
    UINT8 link_fault;      /*When the PHY has detected a fault has occurred in the receive directioin of the local DTE, it will be set true*/
} DOT_3AH_FlagPara_Type;

typedef struct
{
	UINT32 discoveryIsAnyCout; /*802.3ah连接成功次数统计*/
	BOOL dot_3ah_Enable;
	UINT8 portNumber;
	UINT8 localMac[6];
	UINT8 remoteMac[6];
	enum DOT_3AH_DISCOVERY_STATE discoverySta;
	enum DOT_3AH_LOCAL_PDU localPdu; 
	DOT_3AH_Info_Type	localInfo;
	DOT_3AH_Info_Type	localPreInfo;
	DOT_3AH_Info_Type	remoteInfo;
	
    UINT8 remote_state_valid;	/*指示本端是否收到远端的infomation OAMPDU,命名沿用协议标准*/
    UINT8 local_satisfied;		/*指示本端是否同意完成discovery,命名沿用协议标准*/
    
	BOOL localLinkSta;

    UINT16 timer_lostlink;
		#define  OAM_TIMER_LOSTLINK  5 /*OAM discovery timeout duration, 5s*/

	
    volatile BOOL local_lost_link_timer_done;
    
    DOT_3AH_FlagPara_Type localFlags;
    DOT_3AH_FlagPara_Type remoteFlags;

    BOOL start_loopback;  /*准备进入loopback模式*/
    BOOL end_loopback;    /*准备退出loopback模式*/ 
    BOOL act_in_lbmode;   /*成功进入loopback模式*/

    volatile UINT16 timer_lbrep;
    	#define OAM_TIMER_LBREP  3 /*lb响应超时,3s*/
    volatile BOOL lbrep_timeout;
}Dot_3AH_Para_Type;


typedef struct
{
		UINT8 seqnum[2];
		UINT8 resvd1[2];
		UINT8 timestamp[2];
	
#define OAM_EVE_ERRSYMPRD_LEN 40
#define OAM_EVE_ERRFRM_LEN 26
#define OAM_EVE_ERRFRMPRD_LEN 28
#define OAM_EVE_ERRFRMSEC_LEN 18

		/*errored symbol period, len 40*/
		UINT8 errsymprd_win[8]; 	/*errored symbol Window*/
		UINT8 errsymprd_thr[8];  /*errored symbol threshold*/
		UINT8 errsymprd_errs[8]; 
		UINT8 errsymprd_errRuntal[8];
		UINT8 errsymprd_evenRuntal[4];

	
		/*errored frame event, len 26*/
		UINT8 errfrm_win[2];
		UINT8 errfrm_thr[4];
		UINT8 errfrm_errs[4];
		UINT8 errfrm_errRuntal[8];
		UINT8 errfrm_evenRuntal[4];

			
	
		/*errored frame period event TLV, len 28*/
		UINT8 errfrmprd_win[4];
		UINT8 errfrmprd_thr[4];
		UINT8 errfrmprd_errs[4];
		UINT8 errfrmprd_errRuntal[8];
		UINT8 errfrmprd_evenRuntal[4];
	
		/*errored frame seconds summary Event TLV, len 18*/
		UINT8 errfrmsec_errRuntal[4];
		UINT8 errfrmsec_evenRuntal[4];
		UINT8 errfrmsec_win[2];
		UINT8 errfrmsec_thr[2];
		UINT8 errfrmsec_errs[2];	
	
		/*other varable*/
		UINT8 sendtype; 			 /*send which type event notification frame*/
		UINT8 resvd2;
		UINT16 timer_errfrm;	   /*unit is second*/ 
#define OAM_TIMER_EVE_ERRFRM  1    
		UINT16 timer_errfrmsec;  /*unit is second*/
#define OAM_TIMER_EVE_ERRFRMSEC 10
		UINT8 rx_errpkts_pre[8];
		UINT8 rx_errfrmprdpkts_pre[8];
		UINT8 rx_errfrmsecpkts_pre[4];
		UINT8 rx_frmprdpkts_pre[8];
} hctel_oamEvenPara_Type;


extern UCHAR slowPrtlDstAddr[6];
extern volatile Dot_3AH_Para_Type dot_3AH_Para;

STATUS hctel_oamSendPKt(enum DOT_3AH_OAMPDU_TYPE oamPDUType);
STATUS hctel_oamRcvPkt(UCHAR* pktbuf);
void hctel_oamDiscoveryChangeSta(void);
STATUS hctel_oamSendPduPeriod(void);
void hctel_oamLinkStaCheck(void);
void hctel_oamLinkTimerDoneCheck(void);
void hctel_oamLBRspTOutCheck(void);
void hctel_oamInit(void);
STATUS hctel_oamLoopbackCtrl(BOOL enLB);


#endif

