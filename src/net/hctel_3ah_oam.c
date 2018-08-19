/***************************************************************
* FILE:	hctel_3ah_oam.c

* DESCRIPTION:
	802.3ah Э��OAM����ģ�飻
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

/*����Э��ʹ�õ�Ŀ��Mac��ַ(�鲥��ַ)*/
UCHAR slowPrtlDstAddr[6] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x02};

/*802.3ahЭ����ص�ȫ�ֱ�������*/
volatile Dot_3AH_Para_Type dot_3AH_Para;

/*��ӡ���InfomationTLV��Ϣ*/
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


/*��ȡ�Զ˷��͵�flags��*/
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

/*��ȡ�Զ˵�LOCAL_TLV*/
LOCAL UINT16 hctel_RmtLocalTLVGet(Dot_3AH_Info_TLV_Type *oamInfoTLV)
{
	UINT16 retLen = 0;

	/*��ӡ��Ϣ���������*/
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

/*��䱾�˵�LOCAL_TLV*/
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

/*���Remote_TLV*/
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
	
	/*���˷�����REMOTE_TLV��ӡ*/
	hctel_InfoTLV_Dump("LOCAL_DTE-RMT_TLV", infoTLV);

	return retLen;
}

/*�Ƚ�Զ�˷��͵�REMOTE_TLV�뱾�˵�localInfo�Ƿ�ƥ��*/
BOOL hctel_RemoteTLVSatisfiedOk(Dot_3AH_Info_TLV_Type* infoTLV)
{		
	/*��ӡ��Ϣ���������*/
	hctel_InfoTLV_Dump("R_DTE-RMT_TLV", infoTLV);
	
	/*�ڽ�������ģʽʱ���л��ش���������Ӧ״̬*/
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
	
	/*����鲥��ַ*/
	memcpy(oamPktHdrInfo->dst, slowPrtlDstAddr, 6);
	bufptr = bufptr + 6;
	
	/*���Դ��ַ*/
	memcpy(oamPktHdrInfo->src, my_hwaddr, 6);
	bufptr = bufptr + 6;
	
	/*�������Э�����ͺ�������*/
	oamPktHdrInfo->proType = htons(OAM_TYPE_802_3AH);
	oamPktHdrInfo->subType = DOT_3AH_SUB_TYPE;
	bufptr = bufptr + 3;

	/*���Flags��*/
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

	/*���code��*/
	oamPktHdrInfo->code = pduType;
	bufptr = bufptr + 1;
	
	return bufptr;
}

/*
	�������˷���loopback���������
	�Զ˵���Ӧ��
*/
LOCAL void hctel_oamLoopbackRspProcess(void)
{
	/*���Զ�˻�����Ӧ�Ƿ�ʱ*/
	if(dot_3AH_Para.lbrep_timeout == TRUE)
	{
		/*���յ��Զ���ȷ��״̬���޸�
		����par��mux״̬Ϊforwording,��
		����infomation OAMPDU*/
		dot_3AH_Para.localInfo.parSta = OAM_PAR_FWD;
		dot_3AH_Para.localInfo.muxSta = OAM_MUX_FWD;
		dot_3AH_Para.localInfo.infoRevision++;

		/*���õײ�������������оƬ״̬*/
		;	

		/*����infomationOAMPDU*/
		hctel_oamSendPKt(OAM_INFOPDU_LOCAL_RMT_TLV);
		
		dot_3AH_Para.start_loopback = FALSE;
		dot_3AH_Para.end_loopback = FALSE;
		dot_3AH_Para.act_in_lbmode = FALSE;

		DOT3_AH_LOG("\r\n OAM_RCV:lb timeout! \r\n", 0, 0, 0, 0, 0, 0);
	}

	/*ʹ�ܻ����������Զ����Ӧ*/
	if(dot_3AH_Para.start_loopback == TRUE)
	{
		if((dot_3AH_Para.remoteInfo.muxSta == OAM_MUX_DISCARD) &&
		   (dot_3AH_Para.remoteInfo.parSta == OAM_PAR_LB))
		{
			/*���յ��Զ���ȷ��״̬���޸�
			����mux״̬Ϊforwording,������infomation OAMPDU*/
			dot_3AH_Para.localInfo.muxSta = OAM_MUX_FWD;
			dot_3AH_Para.localInfo.infoRevision++;

			/*���õײ�������������оƬ״̬*/
			;	

			hctel_oamSendPKt(OAM_INFOPDU_LOCAL_RMT_TLV);
			
			dot_3AH_Para.start_loopback = FALSE;
			dot_3AH_Para.act_in_lbmode = TRUE;	

			DOT3_AH_LOG("\r\n OAM_RCV:start remote lb ok! \r\n", 0, 0, 0, 0, 0, 0);
		}
	}

	/*��ֹ�����������Զ����Ӧ*/
	if(dot_3AH_Para.end_loopback == TRUE)
	{
		if((dot_3AH_Para.remoteInfo.muxSta == OAM_MUX_FWD) &&
		   (dot_3AH_Para.remoteInfo.parSta == OAM_PAR_FWD))
		{
			/*���յ��Զ���ȷ��״̬���޸�
			����par��mux״̬Ϊforwording,��
			����infomation OAMPDU*/
			dot_3AH_Para.localInfo.parSta = OAM_PAR_FWD;
			dot_3AH_Para.localInfo.muxSta = OAM_MUX_FWD;
			dot_3AH_Para.localInfo.infoRevision++;

			/*���õײ�������������оƬ״̬*/
			;	

			hctel_oamSendPKt(OAM_INFOPDU_LOCAL_RMT_TLV);
			
			dot_3AH_Para.end_loopback = FALSE;
			dot_3AH_Para.act_in_lbmode = FALSE;	

			DOT3_AH_LOG("\r\n OAM_RCV:end remote lb ok! \r\n", 0, 0, 0, 0, 0, 0);
		}
	}			
}

/*
	������Զ˷��͵�loopback�������ݰ���
*/
LOCAL void hctel_oamLoopbackReqProcess(UINT8 lbCmd)
{
	if(dot_3AH_Para.localInfo.lbCfg == FALSE)
	{
		/*����������ò�֧��loopBack��ֱ���˳�*/
		DOT3_AH_LOG("\r\n OAM_RCV:Local loopback rsp is disable! \r\n", 0, 0, 0, 0, 0, 0);
		return;
	}

	if(lbCmd == OAM_LOOPCMD_ENABLE)
	{
		/*����loopbackģʽ*/
		DOT3_AH_LOG("\r\n OAM_RCV: Enter loopback mode! \r\n", 0, 0, 0, 0, 0, 0);

		/*���õײ�������������оƬ״̬��ʵ�ֻ���*/
		;
		
		/*�޸�mux��par״̬*/
		dot_3AH_Para.localInfo.muxSta = OAM_MUX_DISCARD;
		dot_3AH_Para.localInfo.parSta = OAM_PAR_LB;

		/*!!!!!!�˴�����ʹrevision����֮ǰ��ͬ������ָʾ
		localInfo�����˸ı䣬��������loopbackʧ��!!!!!!!*/	
		dot_3AH_Para.localInfo.infoRevision++;

		/*����LOCAL_TLV Infomaiton OAMPDUָʾ״̬�����ı�*/
		hctel_oamSendPKt(OAM_INFOPDU_LOCALTLV);
	}
	else
	if(lbCmd == OAM_LOOPCMD_DISABLE)
	{
		/*�˳�loopbackģʽ*/
		DOT3_AH_LOG("\r\n OAM_RCV: Exit loopback mode! \r\n", 0, 0, 0, 0, 0, 0);

		/*���õײ�������������оƬ״̬��ʵ�ֻ���*/
		;
		
		/*�޸�mux��par״̬*/
		dot_3AH_Para.localInfo.muxSta = OAM_MUX_FWD;
		dot_3AH_Para.localInfo.parSta = OAM_PAR_FWD;

		/*!!!!!!�˴�����ʹrevision����֮ǰ��ͬ������ָʾ
		localInfo�����˸ı䣬��������loopbackʧ��!!!!!!!*/	
		dot_3AH_Para.localInfo.infoRevision++;

		/*����LOCAL_TLV Infomaiton OAMPDUָʾ״̬�����ı�*/
		hctel_oamSendPKt(OAM_INFOPDU_LOCALTLV);				
	}	
}

/*
	����OAMPDU���ݰ���
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
			/*���OAMPDUͷ��*/
			pktLen += hctel_oamFillPktHead(oamPktHdrInfo, OAM_CODE_INFORMATION);	

			/*ͨ��Infomation TLVָ�봦������*/
			oamInfoTLV = (Dot_3AH_Info_TLV_Type*)(net_tx_buf + pktLen);		

			/*------NO TLV-------*/
			oamInfoTLV->TLVType = OAM_INFOTLV_END;
			oamInfoTLV->info_TLV_len = 0;
			pktLen += 2;
			
			break;
		}
		
		case OAM_INFOPDU_LOCALTLV:
		{
			/*���OAMPDUͷ��*/
			pktLen += hctel_oamFillPktHead(oamPktHdrInfo, OAM_CODE_INFORMATION);	

			/*ͨ��Infomation TLVָ�봦������*/
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
			/*���OAMPDUͷ��*/
			pktLen += hctel_oamFillPktHead(oamPktHdrInfo, OAM_CODE_INFORMATION);	

			/*ͨ��Infomation TLVָ�봦������*/
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
			/*���OAMPDUͷ��*/
			pktLen += hctel_oamFillPktHead(oamPktHdrInfo, OAM_CODE_LOOPCTL);	
			net_tx_buf[pktLen++] = OAM_LOOPCMD_ENABLE;
			break;
		}

		case OAM_LBPDU_DISABLE:
		{
			/*���OAMPDUͷ��*/
			pktLen += hctel_oamFillPktHead(oamPktHdrInfo, OAM_CODE_LOOPCTL);	
			net_tx_buf[pktLen++] = OAM_LOOPCMD_DISABLE;		
			break;
		}

		default:
		{
			return ERROR;
		}
	}

	/*���������������з���*/
	eth_send(NULL, net_tx_buf, pktLen);
	return OK;
}

/*
	����������յ���802.3ahЭ�����ݰ���
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

	/*1.���Ŀ�ĵ�ַ�Ƿ�Ϊoam����ָ�����鲥��ַ*/
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

	/*2.���Э�����ͼ�������*/
	if((ntohs(oamPktHdrInfo->proType) != OAM_TYPE_802_3AH) ||
	   (oamPktHdrInfo->subType != DOT_3AH_SUB_TYPE))
	{
		DOT3_AH_LOG("\r\n OAM_RCV:Protocol Type Error!(type:%x,sub:%x) \r\n", 
					(int)ntohs(oamPktHdrInfo->proType), (int)oamPktHdrInfo->subType, 0, 0, 0, 0);
		return ERROR;		
	}
	pktLen += 3;/*protocolType + subtype*/

	DOT3_AH_LOG("\r\n OAM_RCV:PDU_Type:(code = %d)\r\n", oamPktHdrInfo->code, 0, 0, 0, 0, 0);
	
	/*3.�������ݰ�*/
	if(dot_3AH_Para.localPdu != OAM_ANY)/*local_pdu:LF_INFO RX_INFO or INFO*/
	{		
		/*��ʱֻ�������Infomation OAMPDU*/
		if(oamPktHdrInfo->code == OAM_CODE_INFORMATION)
		{	
			/*����lost_link_timer*/
			dot_3AH_Para.timer_lostlink = 0;
			
			/*��ȡFlag�������Ϣ*/
			hctel_RmtFlagFieldGet(ntohs(oamPktHdrInfo->flags));
			pktLen += 3;/*flags(2) + code(1)*/
			
			/*�ȴ����յ��Զ˷����İ���LOCAL_TLV��Infomation OAMPDU*/
			for(i=0; i<TLV_NUM_IN_ONE_OAMPDU; i++)
			{
				/*ͨ��TLVָ�봦��TLV��Ϣ*/
				oamInfoTLV = (Dot_3AH_Info_TLV_Type*)(pktbuf + pktLen);			

				DOT3_AH_LOG("\r\n OAM_RCV:TLV_Type:(%d)\r\n", oamInfoTLV->TLVType, 0, 0, 0, 0, 0);
				
				/*���յ��Զ˷����İ���LOCAL_TLV��Infomation OAMPDU*/
				if(oamInfoTLV->TLVType == OAM_INFOTLV_LOCAL)
				{
					if(oamInfoTLV->info_TLV_len == OAM_INFOTLV_LEN)
					{
						DOT3_AH_LOG("\r\n OAM_RCV:remote_state_valid is TRUE \r\n", 0, 0, 0, 0, 0, 0);
						
						/*ָʾ���յ��˰���LOCAL_TLV��Infomation OAMPDU*/
						dot_3AH_Para.remote_state_valid = TRUE;	
						
						/*��ȡ�Զ˵�LOCAL_TLV��Ϣ*/
						pktLen += hctel_RmtLocalTLVGet(oamInfoTLV);

						/*�յ�������ѭ��*/	
						break;
					}
					else
					{
						DOT3_AH_LOG("\r\n OAM_RCV:LOCAL_TLV len error!\r\n", 0, 0, 0, 0, 0, 0);
					}
				}
			}

			/*�Ѿ����յ�����LOCAL_TLV����ȷInfomation OAMPDU*/
			if(dot_3AH_Para.remote_state_valid == TRUE)
			{
				for(i=0; i<TLV_NUM_IN_ONE_OAMPDU; i++)
				{
					/*ͨ��TLVָ�봦��TLV��Ϣ*/
					oamInfoTLV = (Dot_3AH_Info_TLV_Type*)(pktbuf + pktLen);				
					
					/*����յ��˰���LOCAL_TLV��REMOTE_TLV��Infomation OAMPDU*/
					if(oamInfoTLV->TLVType == OAM_INFOTLV_REMOTE)
					{
						/*���Զ˷��͵Ĺ�����REMOTE_TLV��Ϣ�뱾��LocalInfo�Ƿ�ƥ��*/
						if(hctel_RemoteTLVSatisfiedOk(oamInfoTLV))
						{
							DOT3_AH_LOG("\r\n OAM_RCV:local_satisfied is TRUE \r\n", 0, 0, 0, 0, 0, 0);
							
							/*���ƥ��,��local_satisfied == TRUE*/
							dot_3AH_Para.local_satisfied = TRUE;
							
							/*ƥ���ֱ������ѭ��*/
							break;
						}
						else
						{
							DOT3_AH_LOG("\r\n OAM_RCV:local_satisfied is FALSE \r\n", 0, 0, 0, 0, 0, 0);
							
							/*���ƥ��,��local_satisfied == FALSE*/
							dot_3AH_Para.local_satisfied = FALSE;
						}

						/*remote_TLV ���ȹ̶�Ϊ16*/	
						pktLen += 16;
					}
				}

				/*���Զ�˷��͵�REMOTE_TLV��Ϣ�뱾��LocalInfoƥ��*/	
				if(dot_3AH_Para.local_satisfied == TRUE)
				{
					DOT3_AH_LOG("\r\n OAM_RCV:local_stable is TRUE \r\n", 0, 0, 0, 0, 0, 0);
					dot_3AH_Para.localFlags.local_stable = TRUE; /*ָʾ���˵������Discovery�����Ѿ����*/
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
		/*���յ�Infomation OAMPDU*/
		if(oamPktHdrInfo->code == OAM_CODE_INFORMATION)
		{
			/*����lost_link_timer*/
			dot_3AH_Para.timer_lostlink = 0;
			
			/*��ȡFlag�������Ϣ*/
			hctel_RmtFlagFieldGet(ntohs(oamPktHdrInfo->flags));
			pktLen += 3;/*flags(2) + code(1)*/					
			
			for(i=0; i<TLV_NUM_IN_ONE_OAMPDU; i++)
			{				
				/*ͨ��TLVָ�봦��TLV��Ϣ*/
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

				/*�ɹ����յ�����LOCAL_TLV��REMOTE_TLV����Ϣ*/
				if((localTLVRcved == TRUE) && (remoteTLVRcved == TRUE))
				{	
					/*���㳬ʱͳ�Ƽ�����*/
					dot_3AH_Para.timer_lostlink = 0;
					
					/*���Զ˷��͵Ĺ�����REMOTE_TLV��Ϣ�뱾��LocalInfo�Ƿ�ƥ��*/
					if(hctel_RemoteTLVSatisfiedOk(oamInfoRmtTLV) == FALSE)
					{
						DOT3_AH_LOG("\r\n OAM_RCV:REMOTE_TLV is not satisfied! \r\n", 0, 0, 0, 0, 0, 0);
						
						/*����Զ�˷��͵�REMOTE_TLV�뱾��localInfo��ƥ��*/
						dot_3AH_Para.localFlags.local_stable = FALSE;
						dot_3AH_Para.local_satisfied = FALSE;
						return ERROR;
					}
					
					/*��ȡԶ�˵�LOCAL_TLV*/
					hctel_RmtLocalTLVGet(oamInfoLocalTLV);

					DOT3_AH_LOG("\r\n OAM_RCV:Infomation OAMPDU rcv ok! \r\n", 0, 0, 0, 0, 0, 0);

					/*����Զ˻ָ���loopback��Ӧ���ݰ�*/
					hctel_oamLoopbackRspProcess();
					break;
				}
			}
		}
		else
		/*���յ�LinkEvent OAMPDU*/
		if(oamPktHdrInfo->code == OAM_CODE_EVENTNOTIFI)
		{
			;
		}	
		else
		/*���յ�Mib Req OAMPDU*/
		if(oamPktHdrInfo->code == OAM_CODE_VARREQ)
		{
			;
		}	
		else
		/*���յ�Mib Rsp OAMPDU*/
		if(oamPktHdrInfo->code == OAM_CODE_VARRES)
		{
			;
		}
		/*���յ�LoopBack OAMPDU*/
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
	Discovery״̬����ϵͳ��Ҫÿ��1sִ��һ�θú�����
	����ʵʱ����Discovery״̬��
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
		/*û���յ�Զ��Infomation OAMPDU*/
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
		/*���յ���Զ��Infomation OAMPDU*/
		{
			/*������Զ�˵�OAM���ò�ƥ��*/
			if(dot_3AH_Para.local_satisfied == FALSE)
			{
				discoveryOk = FALSE;
				
				dot_3AH_Para.discoverySta = OAM_DISCV_SEND_LOCAL_REMOTE;
				dot_3AH_Para.localPdu = OAM_INFO;		
				DOT3_AH_LOG("\r\n OAM_Discovery: dis sta SEND_LOCAL_REMOET! \r\n", 0, 0, 0, 0, 0, 0);
			}
			/*������Զ�˵�OAM����ƥ��*/
			else
			{				
				/*ָʾ:"Local DTE Discovery process has completed"*/
				dot_3AH_Para.localFlags.local_evaluating = FALSE;
				dot_3AH_Para.localFlags.local_stable = TRUE;

				/*��ʾԶ��OAM Discoveryû�����*/	
				if(dot_3AH_Para.remoteFlags.local_stable == FALSE)
				{
					discoveryOk = FALSE;
					
					dot_3AH_Para.discoverySta = OAM_DISCV_SEND_LOCAL_REMOTE_OK;
					dot_3AH_Para.localPdu = OAM_INFO;	
					DOT3_AH_LOG("\r\n OAM_Discovery: dis sta SEND_LOCAL_REMOET_OK! \r\n", 0, 0, 0, 0, 0, 0);
				}
				/*��ʾԶ��OAM Disvovery���*/
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
	��ʱ����Infomation OAMPDU,��Ҫÿ��1sִ��һ�Σ�
*/
STATUS hctel_oamSendPduPeriod(void)
{	
	if(dot_3AH_Para.dot_3ah_Enable == FALSE)
	{
		return ERROR;
	}
	
	/*��ʱֻ����LinkFault����λ��noTLV InfomationOAMPDU*/
	if(dot_3AH_Para.discoverySta == OAM_DISCV_FAULT)
	{
		hctel_oamSendPKt(OAM_INFOPDU_NOTLV);
	}
	else
	/*��ʱֻ���Ͱ���localTLV��InformationOAMPDU*/
	if(dot_3AH_Para.discoverySta == OAM_DISCV_ACTIVE_SEND_LOCAL)
	{
		hctel_oamSendPKt(OAM_INFOPDU_LOCALTLV);	
	}
	else
	/*��ʱֻ���Ͱ���LocalTLV��RemoteTLV��InfomationOAMPDU*/
	if((dot_3AH_Para.discoverySta == OAM_DISCV_SEND_LOCAL_REMOTE) ||
	   (dot_3AH_Para.discoverySta == OAM_DISCV_SEND_LOCAL_REMOTE_OK))

	{
		hctel_oamSendPKt(OAM_INFOPDU_LOCAL_RMT_TLV);	
	}
	else
	/*����������OAMPDU*/
	if(dot_3AH_Para.discoverySta == OAM_DISCV_SEND_ANY)
	{			
		hctel_oamSendPKt(OAM_INFOPDU_LOCAL_RMT_TLV);	
	}
	
	return OK;
}

/*��ʱ���˿�link״̬;*/
void hctel_oamLinkStaCheck(void)
{
	if(dot_3AH_Para.dot_3ah_Enable == FALSE)
	{
		return ;
	}

	/*�������������鿴Link״̬*/
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

/*��ʱ���link_lost_done_timer�Ƿ����,ÿ��1sִ��һ��*/
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

/*���loopback��Ӧ�Ƿ�ʱ*/
void hctel_oamLBRspTOutCheck(void)
{
	if(dot_3AH_Para.start_loopback || dot_3AH_Para.end_loopback)
	{	
	    dot_3AH_Para.timer_lbrep++;
		if(dot_3AH_Para.timer_lbrep > OAM_TIMER_LBREP)
		{
			/*ָʾ������Ӧ��ʱ*/
			dot_3AH_Para.lbrep_timeout = TRUE;
		}
	}
}

/*Խ�޸澯����*/
hctel_oamEvenPara_Type localExceed;

/*
*���޼����ڳ�ʼ������
*����:��
*����ֵ:��
*    --------- ����鴴�� 20131127
*/
void lkEvtMonitorInit(void)
{
	/*ʱ�����ʼ��*/
	*((UINT16*)&localExceed.timestamp[0]) = 0;
	/*��֡��������λ��0*/
	*((UINT32*)&localExceed.errfrm_errRuntal[0]) = 1;
	/*��֡��������λ��0*/
	*((UINT32*)&localExceed.errfrm_errRuntal[4]) = 0;
	
	/*��֡��ش��ڷ�Χ1000ms-60000ms��Ĭ��ֵ1000ms*/
	*((UINT16*)localExceed.errfrm_win) = 10;
	/*��֡������޷�Χ 1-4294967295֡��Ĭ��ֵ1֡*/
	*((UINT32*)localExceed.errfrm_thr) = 1 ;
	/*��֡���ڼ�ش���Χ1488~892800000֡��Ĭ��ֵ892800000*/
	*((UINT32*)localExceed.errfrmprd_win) = 892800000;
	/*��֡���ڼ�ش�����1~892800000֡��Ĭ��ֵ1*/
	*((UINT32*)localExceed.errfrmprd_thr) = 1;
	/*��֡����ʱ�䴰��Χ10-900s��Ĭ��ֵ60s*/
	*((UINT16*)localExceed.errfrmsec_win) = 600;
	/*��֡��������1-60s��Ĭ��ֵ1s*/
	*((UINT16*)localExceed.errfrmsec_thr) = 1;
	/*��֡�ź����ڼ�ش���Χ1-60����Ĭ��ֵ1��*/
	*((UINT32*)&localExceed.errsymprd_win[4]) = 1;
	*((UINT32*)&localExceed.errsymprd_win[0]) = 0;
	/*��֡�ź����ڼ������1~892800000��Ĭ��ֵ1*/
	*((UINT32*)&localExceed.errsymprd_thr[4]) = 1;
	*((UINT32*)&localExceed.errsymprd_thr[0]) = 0;
}


/*��ʼ��802.3ah OAM��ز���*/
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
	��������loopback����ʹ��Զ�˻��غ�
	��ֹԶ�˻��أ�
*/
STATUS hctel_oamLoopbackCtrl(BOOL enLB)
{
	if(dot_3AH_Para.dot_3ah_Enable == FALSE)
	{
		DOT3_AH_LOG("\r\nLB_CTRL: dot3ah is disable! \r\n",0,0,0,0,0,0);
		return ERROR;
	}

	/*PASSIVEģʽ������������Զ�˻���*/
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

	/*�������״̬����������������ϴ���loopback*/
	if(enLB)
	{
		/*�Զ��豸��par״̬����FWD״̬�������Ѿ����ڻ���״̬*/
		if(dot_3AH_Para.remoteInfo.parSta != OAM_PAR_FWD)
		{
			DOT3_AH_LOG("\r\nLB_CTRL: remote par state is not forwording!\r\n",0,0,0,0,0,0);
			return ERROR;
		}	
		
		/*�ı䱾��par��mux״̬ΪDISCARD*/
		dot_3AH_Para.localInfo.muxSta = OAM_MUX_DISCARD;
		dot_3AH_Para.localInfo.parSta = OAM_PAR_DISCARD;

		/*���õײ�������������оƬ״̬��ʵ�ֻ���*/
		;


		/*!!��ؽ��иò���!!*/
		dot_3AH_Para.localInfo.infoRevision++;

		/*����Infomaion OAMPDUָʾ״̬�ı�*/
		hctel_oamSendPKt(OAM_INFOPDU_LOCAL_RMT_TLV);	

		/*����loopback Enable����*/
		hctel_oamSendPKt(OAM_LBPDU_ENABLE);
	
		dot_3AH_Para.start_loopback = TRUE;
		DOT3_AH_LOG("\r\nLB_CTRL: start remote loopback!\r\n",0,0,0,0,0,0);
	}
	else
	{
		/*�ı䱾��par��mux״̬ΪDISCARD*/
		dot_3AH_Para.localInfo.muxSta = OAM_MUX_DISCARD;
		dot_3AH_Para.localInfo.parSta = OAM_PAR_DISCARD;

		/*���õײ�������������оƬ״̬*/
		;


		/*!!��ؽ��иò���!!*/
		dot_3AH_Para.localInfo.infoRevision++;

		/*����Infomaion OAMPDUָʾ״̬�ı�*/
		hctel_oamSendPKt(OAM_INFOPDU_LOCAL_RMT_TLV);	

		/*����loopback Enable����*/
		hctel_oamSendPKt(OAM_LBPDU_DISABLE);
		
		dot_3AH_Para.end_loopback = TRUE;
		DOT3_AH_LOG("\r\nLB_CTRL: end remote loopback!\r\n",0,0,0,0,0,0);
	}
	
	dot_3AH_Para.lbrep_timeout = FALSE;
	
	/*��ʱ����ʼ��ʱ*/
	dot_3AH_Para.timer_lbrep = 0;
	
	return OK;
}
#endif

