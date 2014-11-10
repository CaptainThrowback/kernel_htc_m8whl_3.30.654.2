
/**=========================================================================
  
  \file  ccxApi.h
  
  \brief CCX APIs
  
  Copyright (c) 2012 Qualcomm Atheros, Inc.
  All Rights Reserved.
  Qualcomm Atheros Confidential and Proprietary.
  
  ========================================================================*/


#ifndef __CCX_API_H__
#define __CCX_API_H__

#ifdef WLAN_FEATURE_VOWIFI
extern tSirRetStatus
ccxProcessBeaconReportXmit( tpAniSirGlobal pMac, tpSirBeaconReportXmitInd pBcnReport);
#endif

tSirRetStatus limActivateTSMStatsTimer(tpAniSirGlobal pMac,
                                            tpPESession psessionEntry);
tSirRetStatus limProcessTsmTimeoutHandler(tpAniSirGlobal pMac,tpSirMsgQ  limMsg);
void limProcessHalCcxTsmRsp(tpAniSirGlobal pMac, tpSirMsgQ limMsg);

tSirRetStatus limProcessAdjacentAPRepMsg (tpAniSirGlobal pMac, tANI_U32 *pMsgBuf);

void limCleanupCcxCtxt(tpAniSirGlobal pMac, tpPESession pSessionEntry);
#endif
