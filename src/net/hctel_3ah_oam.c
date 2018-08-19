/***************************************************************
* FILE:	hctel_3ah_oam.c

* DESCRIPTION:
	802.3ah 协议OAM功能模块；
MODIFY HISTORY:
	2014.6.9           yangshansong   create
****************************************************************/
#include "config.h"
#include "hctel_3ah_oam.h"
#include "net.h"
#include "eth.h"

#ifdef HCTEL_3AH_OAM_SUPPORT
LOCAL BOOL dot3ah_Info_TLV_Dump = FALSE;

BOOL dot3ah_debug = FALSE;
#define DOT3_AH_LOG(fmt, arg1, arg2, arg3, arg4, arg5, arg6) \
{ \
	if(dot3ah_debug)\
	{\
		CLI_PRINT(fmt, arg1, arg2, arg3, arg4, arg5, arg6);\
	}\
}

/*慢速协议使用的目的Mac地址(组播地址)*/
UCHAR slowPrtlDstAddr[6] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x02};

/*802.3ah协议相关的全局变量定义*/
volatile Dot_3AH_Para_Type dot_3AH_Para;

/*打印相关InfomationTLV信息*/
LOCAL void hctel_InfoTLV_Dump(char* titleStr, Dot_3AH_Info_TLV_Type* infoTLV)
{
	if(dot3ah_Info_TLV_Dump == TRUE)
	{
		CLI_PRINT("\r\n======%s======",(int)titleStr,0,0,0,0,0);
		CLI_PRINT("\r\nTLVType: %x",infoTLV->TLVType,0,0,0,0,0);
		CLI_PRINT("\r\ninfo_TLV_len: %x",infoTLV->info_TLV_len,0,0,0,0,0);
		CLI_PRINT("\r\noamVersion: %x",infoTLV->oamVersion,0,0,0,0,0);
		CLI_PRINT("\r\noamRevision: %x",infoTLV->oamRevision,0,0,0,0,0);
		CLI_PRINT("\r\noamState: %x",infoTLV->oamState,0,0,0,0,0);
		CLI_PRINT("\r\noamCfg: %x",infoTLV->oamCfg,0,0,0,0,0);
		CLI_PRINT("\r\noamPDUCfg: %x",infoTLV->oamPDUCfg,0,0,0,0,0);
		CLI_PRINT("\r\noamOUI[0]: %x",infoTLV->oamOUI[0],0,0,0,0,0);
		CLI_PRINT("\r\noamOUI[1]: %x",infoTLV->oamOUI[1],0,0,0,0,0);
		CLI_PRINT("\r\noamOUI[2]: %x",infoTLV->oamOUI[2],0,0,0,0,0);
		CLI_PRINT("\r\noamVenderSpeci[0]: %x",infoTLV->oamVenderSpeci[0],0,0,0,0,0);
		CLI_PRINT("\r\noamVenderSpeci[1]: %x",infoTLV->oamVenderSpeci[1],0,0,0,0,0);
		CLI_PRINT("\r\noamVenderSpeci[2]: %x",infoTLV->oamVenderSpeci[2],0,0,0,0,0);
		CLI_PRINT("\r\noamVenderSpeci[3]: %x",infoTLV->oamVenderSpeci[3],0,0,0,0,0);
		CLI_PRINT("\r\n==============\r\n",0,0,0,0,0,0);
	}	
}


/*提取对端发送的flags域*/
LOCAL void hctel_RmtFlagFieldGet(UINT16 flags)
{
	dot_3AH_Para.remoteFlags.remote_stable = flags & OAM_FLAGS_REMOTE_STABLE;
	dot_3AH_Para.remoteFlags.remote_evaluating = flags & OAM_FLAGS_REMOTE_EVALUATING; 
	dot_3AH_Para.remoteFlags.local_stable = flags & OAM_FLAGS_LOCAL_STABLE; 
	dot_3AH_Para.remoteFlags.local_evaluating = flags & OAM_FLAGS_LOCAL_EVALUATING;
	dot_3AH_Para.remoteFlags.critical_event = flags & OAM_FLAGS_CRITICAL_EVENT;
	dot_3AH_Para.remoteFlags.dying_gasp = flags & OAM_FLAGS_DYING_GASP;	
	dot_3AH_Para.remoteFlags.link_fault = flags & OAM_FLAGS_LINK_FAULT;
}

/*提取对端的LOCAL_TLV*/
LOCAL UINT16 hctel_RmtLocalTLVGet(Dot_3AH_Info_TLV_Type *oamInfoTLV)
{
	UINT16 retLen = 0;

	/*打印信息，方便调试*/
	hctel_InfoTLV_Dump("R_DTE-LOCAL_TLV", oamInfoTLV);

	/*type(1) + length(1)*/
	retLen += 2;
	
	/*oam version*/
	retLen += 1;
	
	/*revision field*/
	dot_3AH_Para.remoteInfo.infoRevision = ntohs(oamInfoTLV->oamRevision);
	retLen += 2;

	/*state field*/
	if(oamInfoTLV->oamState & MUX_STA_MASK)
	{
		dot_3AH_Para.remoteInfo.muxSta = OAM_MUX_DISCARD;
		DOT3_AH_LOG("\r\nremote mux discard\r\n",0,0,0,0,0,0);
	}
	else
	{
		dot_3AH_Para.remoteInfo.muxSta = OAM_MUX_FWD;		
	}
	
	dot_3AH_Para.remoteInfo.parSta = (enum DOT_3AH_PAR_ACTION )(oamInfoTLV->oamState & PAR_STA_MASK);
	if(dot_3AH_Para.remoteInfo.parSta == OAM_PAR_DISCARD)
	{
		DOT3_AH_LOG("\r\nremote par discard\r\n",0,0,0,0,0,0);
	}
	retLen += 1;
	
	/*oamcfg field*/
	if(oamInfoTLV->oamCfg & VAR_CFG_MASK)
	{
		dot_3AH_Para.remoteInfo.varCfg = TRUE;
	}
	else
	{
		dot_3AH_Para.remoteInfo.varCfg = FALSE;
	}

	if(oamInfoTLV->oamCfg & LKVET_CFG_MASK)
	{
		dot_3AH_Para.remoteInfo.lkevtCfg = TRUE;
	}
	else
	{
		dot_3AH_Para.remoteInfo.lkevtCfg = FALSE;
	}

	if(oamInfoTLV->oamCfg & LB_CFG_MASK)
	{
		dot_3AH_Para.remoteInfo.lbCfg = TRUE;
	}
	else
	{
		dot_3AH_Para.remoteInfo.lbCfg = FALSE;
	}
	
	if(oamInfoTLV->oamCfg & UNDIR_CFG_MASK)
	{
		dot_3AH_Para.remoteInfo.unidirCfg = TRUE;
	}
	else
	{
		dot_3AH_Para.remoteInfo.unidirCfg = FALSE;
	}
	
	dot_3AH_Para.remoteInfo.modeCfg = oamInfoTLV->oamCfg & OAM_MODE_MASK;
	retLen += 1;

	/*oampduCfg field*/
	*((UINT16*)dot_3AH_Para.remoteInfo.pduConfig) = ntohs(oamInfoTLV->oamPDUCfg); 
	retLen += 2;

	/*OUI field*/
	dot_3AH_Para.remoteInfo.oui[0] = oamInfoTLV->oamOUI[0]; 
	dot_3AH_Para.remoteInfo.oui[1] = oamInfoTLV->oamOUI[1]; 
	dot_3AH_Para.remoteInfo.oui[2] = oamInfoTLV->oamOUI[2]; 
	retLen += 3;

	/*vendor spec field*/
	dot_3AH_Para.remoteInfo.vendSpeci[0] = oamInfoTLV->oamVenderSpeci[0]; 
	dot_3AH_Para.remoteInfo.vendSpeci[1] = oamInfoTLV->oamVenderSpeci[1]; 
	dot_3AH_Para.remoteInfo.vendSpeci[2] = oamInfoTLV->oamVenderSpeci[2]; 
	dot_3AH_Para.remoteInfo.vendSpeci[3] = oamInfoTLV->oamVenderSpeci[3]; 
	retLen += 4;

	return retLen;
}

/*填充本端的LOCAL_TLV*/
LOCAL UINT16 hctel_LocalTLVSet(Dot_3AH_Info_TLV_Type *infoTLV)
{
	UINT16 retLen = 0;

	/*type field*/
	infoTLV->TLVType = OAM_INFOTLV_LOCAL;
	retLen += 1;

	/*len field*/
	infoTLV->info_TLV_len = OAM_INFOTLV_LEN;
	retLen += 1;

	/*version field*/
	infoTLV->oamVersion = 0x01;
	retLen += 1;	
	
	/*revision field*/
	infoTLV->oamRevision = htons(dot_3AH_Para.localInfo.infoRevision);
	retLen += 2;

	if(dot_3AH_Para.localInfo.muxSta == OAM_MUX_DISCARD)
	{
		infoTLV->oamState |= MUX_STA_MASK;
	}
	else
	{
		infoTLV->oamState &= ~MUX_STA_MASK;		
	}
	infoTLV->oamState &= ~PAR_STA_MASK;
	infoTLV->oamState |= dot_3AH_Para.localInfo.parSta & PAR_STA_MASK;
	retLen += 1;

	/*oamcfg field*/
	if(dot_3AH_Para.localInfo.varCfg)
	{
		infoTLV->oamCfg |= VAR_CFG_MASK;
	}
	else
	{
		infoTLV->oamCfg &= ~VAR_CFG_MASK;
	}

	if(dot_3AH_Para.localInfo.lkevtCfg)
	{
		infoTLV->oamCfg |= LKVET_CFG_MASK;
	}
	else
	{
		infoTLV->oamCfg &= ~LKVET_CFG_MASK;
	}	

	if(dot_3AH_Para.localInfo.lbCfg)
	{
		infoTLV->oamCfg |= LB_CFG_MASK;
	}
	else
	{
		infoTLV->oamCfg &= ~LB_CFG_MASK;
	}	

	if(dot_3AH_Para.localInfo.unidirCfg)
	{
		infoTLV->oamCfg |= UNDIR_CFG_MASK;
	}
	else
	{
		infoTLV->oamCfg &= ~UNDIR_CFG_MASK;
	}	

	if(dot_3AH_Para.localInfo.modeCfg & OAM_MODE_MASK)
	{
		infoTLV->oamCfg |= OAM_MODE_MASK;
	}
	else
	{
		infoTLV->oamCfg &= ~OAM_MODE_MASK;
	}
	retLen += 1;

	/*oampduCfg field*/
	infoTLV->oamPDUCfg = htons(*((UINT16*)dot_3AH_Para.localInfo.pduConfig)); 
	retLen += 2;

	/*OUI field*/
	infoTLV->oamOUI[0] = dot_3AH_Para.localInfo.oui[0]; 
	infoTLV->oamOUI[1] = dot_3AH_Para.localInfo.oui[1]; 
	infoTLV->oamOUI[2] = dot_3AH_Para.localInfo.oui[2];
	retLen += 3;

	/*vendor spec field*/
	infoTLV->oamVenderSpeci[0] = dot_3AH_Para.localInfo.vendSpeci[0]; 
	infoTLV->oamVenderSpeci[1] = dot_3AH_Para.localInfo.vendSpeci[1]; 
	infoTLV->oamVenderSpeci[2] = dot_3AH_Para.localInfo.vendSpeci[2]; 
	infoTLV->oamVenderSpeci[3] = dot_3AH_Para.localInfo.vendSpeci[3]; 
	retLen += 4;

	DOT3_AH_LOG("\r\nlocal oamState(Send): %x\r\n",infoTLV->oamState,0,0,0,0,0);	

	return retLen;
}

/*填充Remote_TLV*/
LOCAL UINT16 hctel_RemoteTLVSet(Dot_3AH_Info_TLV_Type *infoTLV)
{
	UINT16 retLen = 0;

	/*type field*/
	infoTLV->TLVType = OAM_INFOTLV_REMOTE;
	retLen += 1;

	/*len field*/
	infoTLV->info_TLV_len = OAM_INFOTLV_LEN;
	retLen += 1;
	
	/*revision field*/
	infoTLV->oamRevision = htons(dot_3AH_Para.remoteInfo.infoRevision);
	retLen += 2;

	/*state field*/
	if(dot_3AH_Para.remoteInfo.muxSta == OAM_MUX_DISCARD)
	{
		infoTLV->oamState |= MUX_STA_MASK;
	}
	else
	{
		infoTLV->oamState &= ~MUX_STA_MASK;		
	}
	infoTLV->oamState &= ~PAR_STA_MASK;
	infoTLV->oamState |= dot_3AH_Para.remoteInfo.parSta & PAR_STA_MASK;
	retLen += 1;

	/*oamcfg field*/
	if(dot_3AH_Para.remoteInfo.varCfg)
	{
		infoTLV->oamCfg |= VAR_CFG_MASK;
	}
	else
	{
		infoTLV->oamCfg &= ~VAR_CFG_MASK;
	}

	if(dot_3AH_Para.remoteInfo.lkevtCfg)
	{
		infoTLV->oamCfg |= LKVET_CFG_MASK;
	}
	else
	{
		infoTLV->oamCfg &= ~LKVET_CFG_MASK;
	}	

	if(dot_3AH_Para.remoteInfo.lbCfg)
	{
		infoTLV->oamCfg |= LB_CFG_MASK;
	}
	else
	{
		infoTLV->oamCfg &= ~LB_CFG_MASK;
	}	

	if(dot_3AH_Para.remoteInfo.unidirCfg)
	{
		infoTLV->oamCfg |= UNDIR_CFG_MASK;
	}
	else
	{
		infoTLV->oamCfg &= ~UNDIR_CFG_MASK;
	}	
	
	if(dot_3AH_Para.remoteInfo.modeCfg & OAM_MODE_MASK)
	{
		infoTLV->oamCfg |= OAM_MODE_MASK;
	}
	else
	{
		infoTLV->oamCfg &= ~OAM_MODE_MASK;
	}	
	retLen += 1;

	/*oampduCfg field*/
	infoTLV->oamPDUCfg = htons(*((UINT16*)dot_3AH_Para.remoteInfo.pduConfig)); 
	retLen += 2;

	/*OUI field*/
	infoTLV->oamOUI[0] = dot_3AH_Para.remoteInfo.oui[0]; 
	infoTLV->oamOUI[1] = dot_3AH_Para.remoteInfo.oui[1]; 
	infoTLV->oamOUI[2] = dot_3AH_Para.remoteInfo.oui[2];
	retLen += 3;

	/*vendor spec field*/
	infoTLV->oamVenderSpeci[0] = dot_3AH_Para.remoteInfo.vendSpeci[0]; 
	infoTLV->oamVenderSpeci[1] = dot_3AH_Para.remoteInfo.vendSpeci[1]; 
	infoTLV->oamVenderSpeci[2] = dot_3AH_Para.remoteInfo.vendSpeci[2]; 
	infoTLV->oamVenderSpeci[3] = dot_3AH_Para.remoteInfo.vendSpeci[3]; 
	retLen += 4;	
	
	DOT3_AH_LOG("\r\nrmt oamState(Send): %x\r\n",infoTLV->oamState,0,0,0,0,0);
	
	/*本端发出的REMOTE_TLV打印*/
	hctel_InfoTLV_Dump("LOCAL_DTE-RMT_TLV", infoTLV);

	return retLen;
}

/*比较远端发送的REMOTE_TLV与本端的localInfo是否匹配*/
BOOL hctel_RemoteTLVSatisfiedOk(Dot_3AH_Info_TLV_Type* infoTLV)
{		
	/*打印信息，方便调试*/
	hctel_InfoTLV_Dump("R_DTE-RMT_TLV", infoTLV);
	
	/*在进出环回模式时，有环回处理函数检测对应状态*/
	if(dot_3AH_Para.start_loopback || dot_3AH_Para.end_loopback)
	{
		/*state field:mux state*/
		if(((infoTLV->oamState & MUX_STA_MASK) >> 2) != dot_3AH_Para.localInfo.muxSta)
		{
			DOT3_AH_LOG("\r\n muxSta not satisfy!(%x) \r\n", infoTLV->oamState, 0, 0, 0, 0, 0);
			return FALSE;
		}
		
		/*state field:par state*/
		if((infoTLV->oamState & PAR_STA_MASK) != dot_3AH_Para.localInfo.parSta)
		{
			DOT3_AH_LOG("\r\n parSta not satisfy!(%x) \r\n", infoTLV->oamState, 0, 0, 0, 0, 0);
			return FALSE;
		}
	}

	/*oamcfg field:var cfg*/
	if(((infoTLV->oamCfg & VAR_CFG_MASK) >> 4) != dot_3AH_Para.localInfo.varCfg)
	{
		DOT3_AH_LOG("\r\n varCfg not satisfy!(%x) \r\n", infoTLV->oamCfg, 0, 0, 0, 0, 0);
		return FALSE;
	}

	/*oamcfg field:lkevt cfg*/
	if(((infoTLV->oamCfg & LKVET_CFG_MASK) >> 3) != dot_3AH_Para.localInfo.lkevtCfg)
	{
		DOT3_AH_LOG("\r\n lkevtCfg not satisfy!(%x) \r\n", infoTLV->oamCfg, 0, 0, 0, 0, 0);
		return FALSE;
	}

	/*oamcfg field:lb cfg*/
	if(((infoTLV->oamCfg & LB_CFG_MASK) >> 2) != dot_3AH_Para.localInfo.lbCfg)
	{
		DOT3_AH_LOG("\r\n lbCfg not satisfy!(%x) \r\n", infoTLV->oamCfg, 0, 0, 0, 0, 0);
		return FALSE;
	}

	/*oamcfg field:undir cfg*/
	if(((infoTLV->oamCfg & UNDIR_CFG_MASK) >> 1) != dot_3AH_Para.localInfo.unidirCfg)
	{
		DOT3_AH_LOG("\r\n unidirCfg not satisfy!(%x) \r\n", infoTLV->oamCfg, 0, 0, 0, 0, 0);
		return FALSE;
	}

	/*oamcfg field:oam mode*/
	if((infoTLV->oamCfg & OAM_MODE_MASK) != (dot_3AH_Para.localInfo.modeCfg & OAM_MODE_MASK))
	{
		DOT3_AH_LOG("\r\n modeCfg not satisfy!(%x) \r\n", infoTLV->oamCfg, 0, 0, 0, 0, 0);
		return FALSE;
	}

	/*oampduCfg field*/
	if(*((UINT16*)dot_3AH_Para.localInfo.pduConfig) != ntohs(infoTLV->oamPDUCfg))
	{
		DOT3_AH_LOG("\r\n oamPDUCfg not satisfy!(%x) \r\n", *((UINT16*)dot_3AH_Para.localInfo.pduConfig), 0, 0, 0, 0, 0);
		return FALSE;
	} 

	/*OUI field*/
	if((dot_3AH_Para.localInfo.oui[0] != infoTLV->oamOUI[0]) ||
	   (dot_3AH_Para.localInfo.oui[1] != infoTLV->oamOUI[1]) || 
	   (dot_3AH_Para.localInfo.oui[2] != infoTLV->oamOUI[2]))
	{
		DOT3_AH_LOG("\r\n oui not satisfy! \r\n", 0, 0, 0, 0, 0, 0);
		return FALSE;
	}	

	return TRUE;	
}

LOCAL UINT16 hctel_oamFillPktHead(Dot_3AH_Pkt_Head_Type *oamPktHdrInfo, UINT8 pduType)
{
	UINT16 flagsTmp;
	UINT16 bufptr = 0;
	
	/*填充组播地址*/
	memcpy(oamPktHdrInfo->dst, slowPrtlDstAddr, 6);
	bufptr = bufptr + 6;
	
	/*填充源地址*/
	memcpy(oamPktHdrInfo->src, my_hwaddr, 6);
	bufptr = bufptr + 6;
	
	/*填充慢速协议类型和子类型*/
	oamPktHdrInfo->proType = htons(OAM_TYPE_802_3AH);
	oamPktHdrInfo->subType = DOT_3AH_SUB_TYPE;
	bufptr = bufptr + 3;

	/*填充Flags域*/
	flagsTmp = 0;
	(dot_3AH_Para.localFlags.remote_stable)?(flagsTmp |= 0x40):(0);
	(dot_3AH_Para.localFlags.remote_evaluating)?(flagsTmp |= 0x20):(0);
	(dot_3AH_Para.localFlags.local_stable)?(flagsTmp |= 0x10):(0);
	(dot_3AH_Para.localFlags.local_evaluating)?(flagsTmp |= 0x08):(0);
	(dot_3AH_Para.localFlags.critical_event)?(flagsTmp |= 0x04):(0);
	(dot_3AH_Para.localFlags.dying_gasp)?(flagsTmp |= 0x02):(0);
	(dot_3AH_Para.localFlags.link_fault)?(flagsTmp |= 0x01):(0);
	oamPktHdrInfo->flags = htons(flagsTmp);
	bufptr = bufptr + 2;

	/*填充code域*/
	oamPktHdrInfo->code = pduType;
	bufptr = bufptr + 1;
	
	return bufptr;
}

/*
	负责处理本端发送loopback请求命令后，
	对端的响应；
*/
LOCAL void hctel_oamLoopbackRspProcess(void)
{
	/*检测远端环回响应是否超时*/
	if(dot_3AH_Para.lbrep_timeout == TRUE)
	{
		/*接收到对端正确的状态后修改
		本端par和mux状态为forwording,并
		发送infomation OAMPDU*/
		dot_3AH_Para.localInfo.parSta = OAM_PAR_FWD;
		dot_3AH_Para.localInfo.muxSta = OAM_MUX_FWD;
		dot_3AH_Para.localInfo.infoRevision++;

		/*调用底层驱动函数设置芯片状态*/
		;	

		/*发送infomationOAMPDU*/
		hctel_oamSendPKt(OAM_INFOPDU_LOCAL_RMT_TLV);
		
		dot_3AH_Para.start_loopback = FALSE;
		dot_3AH_Para.end_loopback = FALSE;
		dot_3AH_Para.act_in_lbmode = FALSE;

		DOT3_AH_LOG("\r\n OAM_RCV:lb timeout! \r\n", 0, 0, 0, 0, 0, 0);
	}

	/*使能环回请求后处理远端响应*/
	if(dot_3AH_Para.start_loopback == TRUE)
	{
		if((dot_3AH_Para.remoteInfo.muxSta == OAM_MUX_DISCARD) &&
		   (dot_3AH_Para.remoteInfo.parSta == OAM_PAR_LB))
		{
			/*接收到对端正确的状态后修改
			本端mux状态为forwording,并发送infomation OAMPDU*/
			dot_3AH_Para.localInfo.muxSta = OAM_MUX_FWD;
			dot_3AH_Para.localInfo.infoRevision++;

			/*调用底层驱动函数设置芯片状态*/
			;	

			hctel_oamSendPKt(OAM_INFOPDU_LOCAL_RMT_TLV);
			
			dot_3AH_Para.start_loopback = FALSE;
			dot_3AH_Para.act_in_lbmode = TRUE;	

			DOT3_AH_LOG("\r\n OAM_RCV:start remote lb ok! \r\n", 0, 0, 0, 0, 0, 0);
		}
	}

	/*禁止环回请求后处理远端响应*/
	if(dot_3AH_Para.end_loopback == TRUE)
	{
		if((dot_3AH_Para.remoteInfo.muxSta == OAM_MUX_FWD) &&
		   (dot_3AH_Para.remoteInfo.parSta == OAM_PAR_FWD))
		{
			/*接收到对端正确的状态后修改
			本端par和mux状态为forwording,并
			发送infomation OAMPDU*/
			dot_3AH_Para.localInfo.parSta = OAM_PAR_FWD;
			dot_3AH_Para.localInfo.muxSta = OAM_MUX_FWD;
			dot_3AH_Para.localInfo.infoRevision++;

			/*调用底层驱动函数设置芯片状态*/
			;	

			hctel_oamSendPKt(OAM_INFOPDU_LOCAL_RMT_TLV);
			
			dot_3AH_Para.end_loopback = FALSE;
			dot_3AH_Para.act_in_lbmode = FALSE;	

			DOT3_AH_LOG("\r\n OAM_RCV:end remote lb ok! \r\n", 0, 0, 0, 0, 0, 0);
		}
	}			
}

/*
	负责处理对端发送的loopback请求数据包；
*/
LOCAL void hctel_oamLoopbackReqProcess(UINT8 lbCmd)
{
	if(dot_3AH_Para.localInfo.lbCfg == FALSE)
	{
		/*如果本端配置不支持loopBack，直接退出*/
		DOT3_AH_LOG("\r\n OAM_RCV:Local loopback rsp is disable! \r\n", 0, 0, 0, 0, 0, 0);
		return;
	}

	if(lbCmd == OAM_LOOPCMD_ENABLE)
	{
		/*进入loopback模式*/
		DOT3_AH_LOG("\r\n OAM_RCV: Enter loopback mode! \r\n", 0, 0, 0, 0, 0, 0);

		/*调用底层驱动函数设置芯片状态，实现环回*/
		;
		
		/*修改mux和par状态*/
		dot_3AH_Para.localInfo.muxSta = OAM_MUX_DISCARD;
		dot_3AH_Para.localInfo.parSta = OAM_PAR_LB;

		/*!!!!!!此处必须使revision域与之前不同，用于指示
		localInfo发生了改变，否则会造成loopback失败!!!!!!!*/	
		dot_3AH_Para.localInfo.infoRevision++;

		/*发送LOCAL_TLV Infomaiton OAMPDU指示状态发生改变*/
		hctel_oamSendPKt(OAM_INFOPDU_LOCALTLV);
	}
	else
	if(lbCmd == OAM_LOOPCMD_DISABLE)
	{
		/*退出loopback模式*/
		DOT3_AH_LOG("\r\n OAM_RCV: Exit loopback mode! \r\n", 0, 0, 0, 0, 0, 0);

		/*调用底层驱动函数设置芯片状态，实现环回*/
		;
		
		/*修改mux和par状态*/
		dot_3AH_Para.localInfo.muxSta = OAM_MUX_FWD;
		dot_3AH_Para.localInfo.parSta = OAM_PAR_FWD;

		/*!!!!!!此处必须使revision域与之前不同，用于指示
		localInfo发生了改变，否则会造成loopback失败!!!!!!!*/	
		dot_3AH_Para.localInfo.infoRevision++;

		/*发送LOCAL_TLV Infomaiton OAMPDU指示状态发生改变*/
		hctel_oamSendPKt(OAM_INFOPDU_LOCALTLV);				
	}	
}

/*
	发送OAMPDU数据包；
*/
STATUS hctel_oamSendPKt(enum DOT_3AH_OAMPDU_TYPE oamPDUType)
{
	UINT16 pktLen = 0;
	Dot_3AH_Pkt_Head_Type *oamPktHdrInfo = (Dot_3AH_Pkt_Head_Type *)net_tx_buf;
	Dot_3AH_Info_TLV_Type *oamInfoTLV = NULL;

	switch(oamPDUType)
	{
		case OAM_INFOPDU_NOTLV:
		{
			/*填充OAMPDU头部*/
			pktLen += hctel_oamFillPktHead(oamPktHdrInfo, OAM_CODE_INFORMATION);	

			/*通过Infomation TLV指针处理数据*/
			oamInfoTLV = (Dot_3AH_Info_TLV_Type*)(net_tx_buf + pktLen);		

			/*------NO TLV-------*/
			oamInfoTLV->TLVType = OAM_INFOTLV_END;
			oamInfoTLV->info_TLV_len = 0;
			pktLen += 2;
			
			break;
		}
		
		case OAM_INFOPDU_LOCALTLV:
		{
			/*填充OAMPDU头部*/
			pktLen += hctel_oamFillPktHead(oamPktHdrInfo, OAM_CODE_INFORMATION);	

			/*通过Infomation TLV指针处理数据*/
			oamInfoTLV = (Dot_3AH_Info_TLV_Type*)(net_tx_buf + pktLen);	

			/*------LOCAL TLV-------*/		
			pktLen += hctel_LocalTLVSet(oamInfoTLV);
			oamInfoTLV = (Dot_3AH_Info_TLV_Type*)(net_tx_buf + pktLen);
			
			/*------NO TLV-------*/
			oamInfoTLV->TLVType = OAM_INFOTLV_END;
			oamInfoTLV->info_TLV_len = 0;
			pktLen += 2;				
			break;
		}
		
		case OAM_INFOPDU_LOCAL_RMT_TLV:
		{
			/*填充OAMPDU头部*/
			pktLen += hctel_oamFillPktHead(oamPktHdrInfo, OAM_CODE_INFORMATION);	

			/*通过Infomation TLV指针处理数据*/
			oamInfoTLV = (Dot_3AH_Info_TLV_Type*)(net_tx_buf + pktLen);	

			/*------LOCAL TLV-------*/		
			pktLen += hctel_LocalTLVSet(oamInfoTLV);
			oamInfoTLV = (Dot_3AH_Info_TLV_Type*)(net_tx_buf + pktLen);		

			/*------Remote TLV-------*/
			pktLen += hctel_RemoteTLVSet(oamInfoTLV);
			oamInfoTLV = (Dot_3AH_Info_TLV_Type*)(net_tx_buf + pktLen);

			/*------NO TLV-------*/
			oamInfoTLV->TLVType = OAM_INFOTLV_END;
			oamInfoTLV->info_TLV_len = 0;
			pktLen += 2;				
			break;
		}

		case OAM_LBPDU_ENABLE:
		{
			/*填充OAMPDU头部*/
			pktLen += hctel_oamFillPktHead(oamPktHdrInfo, OAM_CODE_LOOPCTL);	
			net_tx_buf[pktLen++] = OAM_LOOPCMD_ENABLE;
			break;
		}

		case OAM_LBPDU_DISABLE:
		{
			/*填充OAMPDU头部*/
			pktLen += hctel_oamFillPktHead(oamPktHdrInfo, OAM_CODE_LOOPCTL);	
			net_tx_buf[pktLen++] = OAM_LOOPCMD_DISABLE;		
			break;
		}

		default:
		{
			return ERROR;
		}
	}

	/*调用驱动函数进行发送*/
	eth_send(NULL, net_tx_buf, pktLen);
	return OK;
}

/*
	处理解析接收到的802.3ah协议数据包；
*/
STATUS hctel_oamRcvPkt(UCHAR* pktbuf)
{
	UCHAR i;
	UINT16 pktLen = 0;
	BOOL remoteTLVRcved = FALSE;
	BOOL localTLVRcved = FALSE;
	Dot_3AH_Pkt_Head_Type *oamPktHdrInfo = NULL;
	Dot_3AH_Info_TLV_Type *oamInfoTLV = NULL;
	Dot_3AH_Info_TLV_Type *oamInfoLocalTLV = NULL;
	Dot_3AH_Info_TLV_Type *oamInfoRmtTLV = NULL;

	if(dot_3AH_Para.dot_3ah_Enable == FALSE)
	{
		DOT3_AH_LOG("\r\n dot3ah is disable \r\n", 0, 0, 0, 0, 0, 0);
		return ERROR;
	}

	oamPktHdrInfo = (Dot_3AH_Pkt_Head_Type*)(pktbuf);

	/*1.检测目的地址是否为oam功能指定的组播地址*/
	for(i=0; i<6; i++)
	{
		if(oamPktHdrInfo->dst[i] != slowPrtlDstAddr[i])
		{
			DOT3_AH_LOG("\r\n OAM_RCV:Dest Addr Error! \r\n", 0, 0, 0, 0, 0, 0);
			return ERROR;
		}
	}
	pktLen += 6; /*dest addr*/
	pktLen += 6; /*src addr*/

	/*2.检测协议类型及子类型*/
	if((ntohs(oamPktHdrInfo->proType) != OAM_TYPE_802_3AH) ||
	   (oamPktHdrInfo->subType != DOT_3AH_SUB_TYPE))
	{
		DOT3_AH_LOG("\r\n OAM_RCV:Protocol Type Error!(type:%x,sub:%x) \r\n", 
					(int)ntohs(oamPktHdrInfo->proType), (int)oamPktHdrInfo->subType, 0, 0, 0, 0);
		return ERROR;		
	}
	pktLen += 3;/*protocolType + subtype*/

	DOT3_AH_LOG("\r\n OAM_RCV:PDU_Type:(code = %d)\r\n", oamPktHdrInfo->code, 0, 0, 0, 0, 0);
	
	/*3.解析数据包*/
	if(dot_3AH_Para.localPdu != OAM_ANY)/*local_pdu:LF_INFO RX_INFO or INFO*/
	{		
		/*此时只允许接收Infomation OAMPDU*/
		if(oamPktHdrInfo->code == OAM_CODE_INFORMATION)
		{	
			/*清零lost_link_timer*/
			dot_3AH_Para.timer_lostlink = 0;
			
			/*提取Flag域相关信息*/
			hctel_RmtFlagFieldGet(ntohs(oamPktHdrInfo->flags));
			pktLen += 3;/*flags(2) + code(1)*/
			
			/*等待接收到对端发来的包含LOCAL_TLV的Infomation OAMPDU*/
			for(i=0; i<TLV_NUM_IN_ONE_OAMPDU; i++)
			{
				/*通过TLV指针处理TLV信息*/
				oamInfoTLV = (Dot_3AH_Info_TLV_Type*)(pktbuf + pktLen);			

				DOT3_AH_LOG("\r\n OAM_RCV:TLV_Type:(%d)\r\n", oamInfoTLV->TLVType, 0, 0, 0, 0, 0);
				
				/*接收到对端发来的包含LOCAL_TLV的Infomation OAMPDU*/
				if(oamInfoTLV->TLVType == OAM_INFOTLV_LOCAL)
				{
					if(oamInfoTLV->info_TLV_len == OAM_INFOTLV_LEN)
					{
						DOT3_AH_LOG("\r\n OAM_RCV:remote_state_valid is TRUE \r\n", 0, 0, 0, 0, 0, 0);
						
						/*指示接收到了包含LOCAL_TLV的Infomation OAMPDU*/
						dot_3AH_Para.remote_state_valid = TRUE;	
						
						/*提取对端的LOCAL_TLV信息*/
						pktLen += hctel_RmtLocalTLVGet(oamInfoTLV);

						/*收到后跳出循环*/	
						break;
					}
					else
					{
						DOT3_AH_LOG("\r\n OAM_RCV:LOCAL_TLV len error!\r\n", 0, 0, 0, 0, 0, 0);
					}
				}
			}

			/*已经接收到包含LOCAL_TLV的正确Infomation OAMPDU*/
			if(dot_3AH_Para.remote_state_valid == TRUE)
			{
				for(i=0; i<TLV_NUM_IN_ONE_OAMPDU; i++)
				{
					/*通过TLV指针处理TLV信息*/
					oamInfoTLV = (Dot_3AH_Info_TLV_Type*)(pktbuf + pktLen);				
					
					/*如果收到了包含LOCAL_TLV和REMOTE_TLV的Infomation OAMPDU*/
					if(oamInfoTLV->TLVType == OAM_INFOTLV_REMOTE)
					{
						/*检查对端发送的过来的REMOTE_TLV信息与本端LocalInfo是否匹配*/
						if(hctel_RemoteTLVSatisfiedOk(oamInfoTLV))
						{
							DOT3_AH_LOG("\r\n OAM_RCV:local_satisfied is TRUE \r\n", 0, 0, 0, 0, 0, 0);
							
							/*如果匹配,则local_satisfied == TRUE*/
							dot_3AH_Para.local_satisfied = TRUE;
							
							/*匹配后直接跳出循环*/
							break;
						}
						else
						{
							DOT3_AH_LOG("\r\n OAM_RCV:local_satisfied is FALSE \r\n", 0, 0, 0, 0, 0, 0);
							
							/*如果匹配,则local_satisfied == FALSE*/
							dot_3AH_Para.local_satisfied = FALSE;
						}

						/*remote_TLV 长度固定为16*/	
						pktLen += 16;
					}
				}

				/*如果远端发送的REMOTE_TLV信息与本端LocalInfo匹配*/	
				if(dot_3AH_Para.local_satisfied == TRUE)
				{
					DOT3_AH_LOG("\r\n OAM_RCV:local_stable is TRUE \r\n", 0, 0, 0, 0, 0, 0);
					dot_3AH_Para.localFlags.local_stable = TRUE; /*指示本端单方向的Discovery过程已经完成*/
				}
			}
		}
		else
		{
			DOT3_AH_LOG("\r\n OAM_RCV:ERROR:no rcv infomation OAMPDU! \r\n", 0, 0, 0, 0, 0, 0);
			return ERROR;
		}
	}
	else/*local_pdu=ANY*/
	{
		/*接收到Infomation OAMPDU*/
		if(oamPktHdrInfo->code == OAM_CODE_INFORMATION)
		{
			/*清零lost_link_timer*/
			dot_3AH_Para.timer_lostlink = 0;
			
			/*提取Flag域相关信息*/
			hctel_RmtFlagFieldGet(ntohs(oamPktHdrInfo->flags));
			pktLen += 3;/*flags(2) + code(1)*/					
			
			for(i=0; i<TLV_NUM_IN_ONE_OAMPDU; i++)
			{				
				/*通过TLV指针处理TLV信息*/
				oamInfoTLV = (Dot_3AH_Info_TLV_Type*)(pktbuf + pktLen);			
				
				if(oamInfoTLV->TLVType == OAM_INFOTLV_LOCAL)
				{	
					oamInfoLocalTLV = (Dot_3AH_Info_TLV_Type*)(pktbuf + pktLen);	
					
					pktLen += 16;
					localTLVRcved = TRUE;
				}
				else
				if(oamInfoTLV->TLVType == OAM_INFOTLV_REMOTE)
				{		
					oamInfoRmtTLV = (Dot_3AH_Info_TLV_Type*)(pktbuf + pktLen);	
					
					pktLen += 16;
					remoteTLVRcved = TRUE;
				}
				else
				{
					DOT3_AH_LOG("\r\n OAM_RCV:Infomation TLV is neither LOCAL_TLV or RMT_TLV\r\n", 0, 0, 0, 0, 0, 0);
					break;
				}

				/*成功接收到包含LOCAL_TLV和REMOTE_TLV的信息*/
				if((localTLVRcved == TRUE) && (remoteTLVRcved == TRUE))
				{	
					/*清零超时统计计数器*/
					dot_3AH_Para.timer_lostlink = 0;
					
					/*检查对端发送的过来的REMOTE_TLV信息与本端LocalInfo是否匹配*/
					if(hctel_RemoteTLVSatisfiedOk(oamInfoRmtTLV) == FALSE)
					{
						DOT3_AH_LOG("\r\n OAM_RCV:REMOTE_TLV is not satisfied! \r\n", 0, 0, 0, 0, 0, 0);
						
						/*发现远端发送的REMOTE_TLV与本端localInfo不匹配*/
						dot_3AH_Para.localFlags.local_stable = FALSE;
						dot_3AH_Para.local_satisfied = FALSE;
						return ERROR;
					}
					
					/*提取远端的LOCAL_TLV*/
					hctel_RmtLocalTLVGet(oamInfoLocalTLV);

					DOT3_AH_LOG("\r\n OAM_RCV:Infomation OAMPDU rcv ok! \r\n", 0, 0, 0, 0, 0, 0);

					/*处理对端恢复的loopback响应数据包*/
					hctel_oamLoopbackRspProcess();
					break;
				}
			}
		}
		else
		/*接收到LinkEvent OAMPDU*/
		if(oamPktHdrInfo->code == OAM_CODE_EVENTNOTIFI)
		{
			;
		}	
		else
		/*接收到Mib Req OAMPDU*/
		if(oamPktHdrInfo->code == OAM_CODE_VARREQ)
		{
			;
		}	
		else
		/*接收到Mib Rsp OAMPDU*/
		if(oamPktHdrInfo->code == OAM_CODE_VARRES)
		{
			;
		}
		/*接收到LoopBack OAMPDU*/
		else
		if(oamPktHdrInfo->code == OAM_CODE_LOOPCTL)
		{
			UINT8 loopBackCmd;
			
			pktLen += 3;/*flags(2) + code(1)*/	
			
			loopBackCmd = *(pktbuf + pktLen);

			hctel_oamLoopbackReqProcess(loopBackCmd);

		}	
		else
		{
			DOT3_AH_LOG("\r\n OAM_RCV: unknown pkt! \r\n", 0, 0, 0, 0, 0, 0);
		}
	}

	return OK;
}

/*
	Discovery状态机，系统需要每隔1s执行一次该函数，
	用于实时更新Discovery状态；
*/
void hctel_oamDiscoveryChangeSta(void)
{
	static BOOL discoveryOk = FALSE;
	
	/*FAULT:local_lost_link_done + local_link_state = TRUE*/
	if((dot_3AH_Para.localLinkSta == FALSE) || (dot_3AH_Para.local_lost_link_timer_done == TRUE))
	{
		discoveryOk = FALSE;
		
		dot_3AH_Para.discoverySta = OAM_DISCV_FAULT;
		dot_3AH_Para.localPdu = OAM_LF_INFO;
		DOT3_AH_LOG("\r\n OAM_Discovery: dis sta FAULT! \r\n", 0, 0, 0, 0, 0, 0);
	}
	else
	{		
		/*没有收到远端Infomation OAMPDU*/
		if(dot_3AH_Para.remote_state_valid == FALSE)
		{
			discoveryOk = FALSE;
			
			/*ACTIVE mode*/
			if(dot_3AH_Para.localInfo.modeCfg == OAM_ACTIVE)
			{
				dot_3AH_Para.discoverySta = OAM_DISCV_ACTIVE_SEND_LOCAL;
				dot_3AH_Para.localPdu = OAM_INFO;	
				
				DOT3_AH_LOG("\r\n OAM_Discovery: dis sta ACTIVE_SEND_LOCAL! \r\n", 0, 0, 0, 0, 0, 0);
			}
			else
			/*PASSIVE mode*/
			if(dot_3AH_Para.localInfo.modeCfg == OAM_PASSIVE)
			{
				dot_3AH_Para.discoverySta = OAM_DISCV_PASSIVE_WAIT;
				dot_3AH_Para.localPdu = OAM_RX_INFO;
				DOT3_AH_LOG("\r\n OAM_Discovery: dis sta PASSIV_WAIT_LOCAL! \r\n", 0, 0, 0, 0, 0, 0);
			}	
		}
		else
		/*接收到了远端Infomation OAMPDU*/
		{
			/*本端与远端的OAM配置不匹配*/
			if(dot_3AH_Para.local_satisfied == FALSE)
			{
				discoveryOk = FALSE;
				
				dot_3AH_Para.discoverySta = OAM_DISCV_SEND_LOCAL_REMOTE;
				dot_3AH_Para.localPdu = OAM_INFO;		
				DOT3_AH_LOG("\r\n OAM_Discovery: dis sta SEND_LOCAL_REMOET! \r\n", 0, 0, 0, 0, 0, 0);
			}
			/*本端与远端的OAM配置匹配*/
			else
			{				
				/*指示:"Local DTE Discovery process has completed"*/
				dot_3AH_Para.localFlags.local_evaluating = FALSE;
				dot_3AH_Para.localFlags.local_stable = TRUE;

				/*表示远端OAM Discovery没有完成*/	
				if(dot_3AH_Para.remoteFlags.local_stable == FALSE)
				{
					discoveryOk = FALSE;
					
					dot_3AH_Para.discoverySta = OAM_DISCV_SEND_LOCAL_REMOTE_OK;
					dot_3AH_Para.localPdu = OAM_INFO;	
					DOT3_AH_LOG("\r\n OAM_Discovery: dis sta SEND_LOCAL_REMOET_OK! \r\n", 0, 0, 0, 0, 0, 0);
				}
				/*表示远端OAM Disvovery完成*/
				else
				{
					if(discoveryOk == FALSE)
					{
						dot_3AH_Para.discoveryIsAnyCout++;
						discoveryOk = TRUE;
					}
					
					dot_3AH_Para.discoverySta = OAM_DISCV_SEND_ANY;
					dot_3AH_Para.localPdu = OAM_ANY;
					DOT3_AH_LOG("\r\n OAM_Discovery: dis sta ANY! \r\n", 0, 0, 0, 0, 0, 0);
				}
			}
		}
	}
}

/*
	定时发送Infomation OAMPDU,需要每隔1s执行一次；
*/
STATUS hctel_oamSendPduPeriod(void)
{	
	if(dot_3AH_Para.dot_3ah_Enable == FALSE)
	{
		return ERROR;
	}
	
	/*此时只发送LinkFault域置位的noTLV InfomationOAMPDU*/
	if(dot_3AH_Para.discoverySta == OAM_DISCV_FAULT)
	{
		hctel_oamSendPKt(OAM_INFOPDU_NOTLV);
	}
	else
	/*此时只发送包含localTLV的InformationOAMPDU*/
	if(dot_3AH_Para.discoverySta == OAM_DISCV_ACTIVE_SEND_LOCAL)
	{
		hctel_oamSendPKt(OAM_INFOPDU_LOCALTLV);	
	}
	else
	/*此时只发送包含LocalTLV和RemoteTLV的InfomationOAMPDU*/
	if((dot_3AH_Para.discoverySta == OAM_DISCV_SEND_LOCAL_REMOTE) ||
	   (dot_3AH_Para.discoverySta == OAM_DISCV_SEND_LOCAL_REMOTE_OK))

	{
		hctel_oamSendPKt(OAM_INFOPDU_LOCAL_RMT_TLV);	
	}
	else
	/*允许发送任意OAMPDU*/
	if(dot_3AH_Para.discoverySta == OAM_DISCV_SEND_ANY)
	{			
		hctel_oamSendPKt(OAM_INFOPDU_LOCAL_RMT_TLV);	
	}
	
	return OK;
}

/*定时检测端口link状态;*/
void hctel_oamLinkStaCheck(void)
{
	if(dot_3AH_Para.dot_3ah_Enable == FALSE)
	{
		return ;
	}

	/*调用驱动函数查看Link状态*/
	if(read_PHY (PHY_REG_BSTAT) & 0x0004)
	{
		dot_3AH_Para.localLinkSta = TRUE;
	}
	else
	{
		dot_3AH_Para.localLinkSta = FALSE;
		DOT3_AH_LOG("\r\n OAM_LINK_CHECK:Link Down! \r\n", 0, 0, 0, 0, 0, 0);
	}
}

/*定时检测link_lost_done_timer是否溢出,每隔1s执行一次*/
void hctel_oamLinkTimerDoneCheck(void)
{
	if(dot_3AH_Para.dot_3ah_Enable == FALSE)
	{
		return ;
	}

	dot_3AH_Para.timer_lostlink++;
	if(dot_3AH_Para.timer_lostlink > OAM_TIMER_LOSTLINK)
	{
		dot_3AH_Para.local_lost_link_timer_done = TRUE;
		DOT3_AH_LOG("\r\n OAM_LINK_TIMER_Done_CHECK:Lost Link Timer Timeout! \r\n", 0, 0, 0, 0, 0, 0);
	}
	else
	{
		dot_3AH_Para.local_lost_link_timer_done = FALSE;
	}
}

/*检测loopback响应是否超时*/
void hctel_oamLBRspTOutCheck(void)
{
	if(dot_3AH_Para.start_loopback || dot_3AH_Para.end_loopback)
	{	
	    dot_3AH_Para.timer_lbrep++;
		if(dot_3AH_Para.timer_lbrep > OAM_TIMER_LBREP)
		{
			/*指示环回响应超时*/
			dot_3AH_Para.lbrep_timeout = TRUE;
		}
	}
}

/*越限告警门限*/
hctel_oamEvenPara_Type localExceed;

/*
*门限及窗口初始化函数
*参数:无
*返回值:无
*    --------- 武宏祥创建 20131127
*/
void lkEvtMonitorInit(void)
{
	/*时间戳初始化*/
	*((UINT16*)&localExceed.timestamp[0]) = 0;
	/*错帧总数低四位清0*/
	*((UINT32*)&localExceed.errfrm_errRuntal[0]) = 1;
	/*错帧总数高四位清0*/
	*((UINT32*)&localExceed.errfrm_errRuntal[4]) = 0;
	
	/*错帧监控窗口范围1000ms-60000ms，默认值1000ms*/
	*((UINT16*)localExceed.errfrm_win) = 10;
	/*错帧监控门限范围 1-4294967295帧，默认值1帧*/
	*((UINT32*)localExceed.errfrm_thr) = 1 ;
	/*错帧周期监控窗范围1488~892800000帧，默认值892800000*/
	*((UINT32*)localExceed.errfrmprd_win) = 892800000;
	/*错帧周期监控窗门限1~892800000帧，默认值1*/
	*((UINT32*)localExceed.errfrmprd_thr) = 1;
	/*错帧秒监控时间窗范围10-900s，默认值60s*/
	*((UINT16*)localExceed.errfrmsec_win) = 600;
	/*错帧秒监控门限1-60s，默认值1s*/
	*((UINT16*)localExceed.errfrmsec_thr) = 1;
	/*错帧信号周期监控窗范围1-60个，默认值1个*/
	*((UINT32*)&localExceed.errsymprd_win[4]) = 1;
	*((UINT32*)&localExceed.errsymprd_win[0]) = 0;
	/*错帧信号周期监控门限1~892800000，默认值1*/
	*((UINT32*)&localExceed.errsymprd_thr[4]) = 1;
	*((UINT32*)&localExceed.errsymprd_thr[0]) = 0;
}


/*初始化802.3ah OAM相关参数*/
void hctel_oamInit(void)
{
	CLI_PRINT("Init 802.3ah function ...",0,0,0,0,0,0);
	
	dot_3AH_Para.dot_3ah_Enable = TRUE;
	dot_3AH_Para.localInfo.lbCfg = TRUE;
	dot_3AH_Para.localInfo.varCfg = FALSE;
	dot_3AH_Para.localInfo.unidirCfg = FALSE;
	dot_3AH_Para.localInfo.lkevtCfg = FALSE;
	dot_3AH_Para.localInfo.modeCfg = OAM_ACTIVE;

	dot_3AH_Para.localInfo.pduConfig[0] = 0x05;
	dot_3AH_Para.localInfo.pduConfig[1] = 0xEE;
	
	dot_3AH_Para.localInfo.oui[0] = OAM_OUI_B0;
	dot_3AH_Para.localInfo.oui[1] = OAM_OUI_B1;
	dot_3AH_Para.localInfo.oui[2] = OAM_OUI_B2;

	dot_3AH_Para.localInfo.vendSpeci[0] = 0x11;
	dot_3AH_Para.localInfo.vendSpeci[1] = 0x22;
	dot_3AH_Para.localInfo.vendSpeci[2] = 0x33;	
	dot_3AH_Para.localInfo.vendSpeci[3] = 0x44;

	dot_3AH_Para.start_loopback = FALSE;
	dot_3AH_Para.end_loopback = FALSE;
	dot_3AH_Para.act_in_lbmode = FALSE;
	dot_3AH_Para.lbrep_timeout = FALSE;

	lkEvtMonitorInit();

	CLI_PRINT("Done\r\n",0,0,0,0,0,0);
}

/*
	主动发送loopback请求使能远端环回和
	禁止远端环回；
*/
STATUS hctel_oamLoopbackCtrl(BOOL enLB)
{
	if(dot_3AH_Para.dot_3ah_Enable == FALSE)
	{
		DOT3_AH_LOG("\r\nLB_CTRL: dot3ah is disable! \r\n",0,0,0,0,0,0);
		return ERROR;
	}

	/*PASSIVE模式不能主动发起远端环回*/
	if(dot_3AH_Para.localInfo.modeCfg != OAM_ACTIVE)
	{
		DOT3_AH_LOG("\r\nLB_CTRL: The PASSIVE DTE cant not loopback remote! \r\n",0,0,0,0,0,0);
		return ERROR;		
	}
	
	if(dot_3AH_Para.discoverySta != OAM_DISCV_SEND_ANY)
	{
		DOT3_AH_LOG("\r\nLB_CTRL: discovery is not complete!\r\n",0,0,0,0,0,0);
		return ERROR;
	}	

	/*操作相关状态，方便其他函数配合处理loopback*/
	if(enLB)
	{
		/*对端设备的par状态不是FWD状态，可能已经处于环回状态*/
		if(dot_3AH_Para.remoteInfo.parSta != OAM_PAR_FWD)
		{
			DOT3_AH_LOG("\r\nLB_CTRL: remote par state is not forwording!\r\n",0,0,0,0,0,0);
			return ERROR;
		}	
		
		/*改变本地par和mux状态为DISCARD*/
		dot_3AH_Para.localInfo.muxSta = OAM_MUX_DISCARD;
		dot_3AH_Para.localInfo.parSta = OAM_PAR_DISCARD;

		/*调用底层驱动函数设置芯片状态，实现环回*/
		;


		/*!!务必进行该操作!!*/
		dot_3AH_Para.localInfo.infoRevision++;

		/*发送Infomaion OAMPDU指示状态改变*/
		hctel_oamSendPKt(OAM_INFOPDU_LOCAL_RMT_TLV);	

		/*发送loopback Enable请求*/
		hctel_oamSendPKt(OAM_LBPDU_ENABLE);
	
		dot_3AH_Para.start_loopback = TRUE;
		DOT3_AH_LOG("\r\nLB_CTRL: start remote loopback!\r\n",0,0,0,0,0,0);
	}
	else
	{
		/*改变本地par和mux状态为DISCARD*/
		dot_3AH_Para.localInfo.muxSta = OAM_MUX_DISCARD;
		dot_3AH_Para.localInfo.parSta = OAM_PAR_DISCARD;

		/*调用底层驱动函数设置芯片状态*/
		;


		/*!!务必进行该操作!!*/
		dot_3AH_Para.localInfo.infoRevision++;

		/*发送Infomaion OAMPDU指示状态改变*/
		hctel_oamSendPKt(OAM_INFOPDU_LOCAL_RMT_TLV);	

		/*发送loopback Enable请求*/
		hctel_oamSendPKt(OAM_LBPDU_DISABLE);
		
		dot_3AH_Para.end_loopback = TRUE;
		DOT3_AH_LOG("\r\nLB_CTRL: end remote loopback!\r\n",0,0,0,0,0,0);
	}
	
	dot_3AH_Para.lbrep_timeout = FALSE;
	
	/*计时器开始计时*/
	dot_3AH_Para.timer_lbrep = 0;
	
	return OK;
}
#endif

