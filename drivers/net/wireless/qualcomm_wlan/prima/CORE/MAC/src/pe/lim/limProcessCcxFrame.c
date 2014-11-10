/*
 * Copyright (c) 2011,2013 Qualcomm Atheros, Inc. 
 * All Rights Reserved. 
 * Qualcomm Atheros Confidential and Proprietary. 
 * This file limProcessCcxFrame.cc contains the code
 * for processing IAPP Frames.
 * Author:      Chas Mannemala
 * Date:        05/23/03
 * History:-
 * Date           Modified by    Modification Information
 * --------------------------------------------------------------------
 *
 */
#ifdef FEATURE_WLAN_CCX
#include "palTypes.h"
#include "wniApi.h"
#include "sirApi.h"
#include "aniGlobal.h"
#include "utilsApi.h"
#include "limTypes.h"
#include "limUtils.h"
#include "limAssocUtils.h"
#include "limSerDesUtils.h"
#include "limSendSmeRspMessages.h"
#include "limSendMessages.h"

tANI_U8 AironetSnapHdr[] = {0xAA, 0xAA, 0x03, 0x00, 0x40, 0x96, 0x00, 0x00};
tANI_U8 CiscoOui[] = {0x00, 0x40, 0x96, 0x00};


void  limCleanupIappPackFrm(tpAniSirGlobal pMac, tpCcxPackIappFrm pPackedFrm) {

   tANI_U8 counter;

   if (!pPackedFrm)
       return;

   if (pPackedFrm->pRadioMeasRepHdr)
       palFreeMemory(pMac->hHdd, pPackedFrm->pRadioMeasRepHdr);

   for (counter=0; counter < SIR_CCX_MAX_MEAS_IE_REPS; counter++)
        if( pPackedFrm->MeasRepInfo[counter].pMeasRepIe)
            palFreeMemory(pMac->hHdd, pPackedFrm->MeasRepInfo[counter].pMeasRepIe);

   if (pPackedFrm->pAdjApRepHdr)
       palFreeMemory(pMac->hHdd, pPackedFrm->pAdjApRepHdr);

   if (pPackedFrm->AdjApRepInfo.pAdjApIe)
       palFreeMemory(pMac->hHdd, pPackedFrm->AdjApRepInfo.pAdjApIe);

   if (pPackedFrm->AssocReasonInfo.pAssocReasonIe)
       palFreeMemory(pMac->hHdd, pPackedFrm->AssocReasonInfo.pAssocReasonIe);

   return;
}


void  limCleanupCcxCtxt(tpAniSirGlobal pMac, tpPESession pSessionEntry) {

   tpCcxMeasReq pCurMeasReq = NULL; 
   tANI_U8 ieCounter;

   if(!pSessionEntry)
      return;

   pCurMeasReq = &(pSessionEntry->ccxContext.curMeasReq);

   if (!pCurMeasReq->isValid)
       return;

   PELOGE(limLog( pMac, LOGE, "Cleaning up CCX RM context\n");)  

   for (ieCounter=0; ieCounter < pCurMeasReq->numMeasReqIe; ieCounter++)
        if(pCurMeasReq->pCurMeasReqIe[ieCounter])
           palFreeMemory(pMac->hHdd, pCurMeasReq->pCurMeasReqIe[ieCounter]);
    
   pCurMeasReq->isValid = VOS_FALSE;

   return;
}

static eCcxPackUnpackStatus
limUnpackMeasReqIe(tANI_U8 *pBuf, tpCcxMeasReqIeInfo pMeasReqIeInfo) {

   tpMeasRequestIe pMeasIe = (tpMeasRequestIe)pBuf;
   tpBcnRequestFields pBcnReqFields = NULL;
   tANI_U32 ieLen=0;

   ieLen = vos_le16_to_cpu(pMeasIe->Length);

   if (ieLen < sizeof(tMeasRequestIe) -sizeof(pMeasIe->Eid) -sizeof(pMeasIe->Length))
       return eCCX_UNPACK_IE_ERR;

   pMeasIe->Eid = vos_le16_to_cpu(pMeasIe->Eid);
   pMeasIe->Length = vos_le16_to_cpu(pMeasIe->Length);
   pMeasIe->MeasToken = vos_le16_to_cpu(pMeasIe->MeasToken);
   pMeasReqIeInfo->pMeasReqIe = pMeasIe;

   if (pMeasIe->MeasType == SIR_CCX_IAPP_RADIO_BEACON_REQUEST) {
       pBcnReqFields = (tpBcnRequestFields)(pBuf + sizeof(tMeasRequestIe));
       pBcnReqFields->MeasDuration = vos_le16_to_cpu(pBcnReqFields->MeasDuration);
       pMeasReqIeInfo->pBcnReqFields = pBcnReqFields;
    }

    return eCCX_UNPACK_SUCCESS;
}


static eCcxPackUnpackStatus
limUnpackNeighborListIe(tANI_U8 *pBuf, tpCcxNeighborIeInfo pNeighborIeInfo) {

   tpNeighborListIe pNeighborIe = (tpNeighborListIe)pBuf;
   tANI_U32 remIeLen=0, ieLen=0;
   tANI_U8 *pIeBuf =NULL, *pSubIe=NULL;
   tANI_U8 subIeEidSize=1, subIeLenSize=1;
   tANI_U16 curSubEid, curSubIeSize;

   ieLen = pNeighborIe->Length;

   if (ieLen < sizeof(tNeighborListIe) -sizeof(pNeighborIe->Eid) -sizeof(pNeighborIe->Length))
       return eCCX_UNPACK_IE_ERR;

   pNeighborIeInfo->pNeighborIe = pNeighborIe;
   remIeLen = pNeighborIe->Length - (sizeof(tNeighborListIe) -sizeof(pNeighborIe->Eid) -sizeof(pNeighborIe->Length));
   pIeBuf = pBuf + sizeof(tNeighborListIe);

   while (remIeLen) {

         if (remIeLen <= (subIeEidSize+subIeLenSize))
             return eCCX_UNPACK_IE_ERR;

          pSubIe = pIeBuf;
          curSubEid = *((tANI_U8 *)pSubIe);
          curSubIeSize = *((tANI_U8 *)(pSubIe+subIeEidSize));

          remIeLen -= (subIeEidSize + subIeLenSize);
          pIeBuf += (subIeEidSize + subIeLenSize);

          if (remIeLen < (curSubIeSize))
              return eCCX_UNPACK_IE_ERR;

           switch (curSubEid) {

                   case SIR_CCX_EID_NEIGHBOR_LIST_RF_SUBIE:
                        pNeighborIeInfo->pRfSubIe = (tpNeighborListRfSubIe)pSubIe;
                   break;
                   case SIR_CCX_EID_NEIGHBOR_LIST_TSF_SUBIE:
                        pNeighborIeInfo->pTsfSubIe = (tpNeighborListTsfSubIe)pSubIe;
                        pNeighborIeInfo->pTsfSubIe->TsfOffset = vos_le16_to_cpu(pNeighborIeInfo->pTsfSubIe->TsfOffset);
                        pNeighborIeInfo->pTsfSubIe->BcnInterval = vos_le16_to_cpu(pNeighborIeInfo->pTsfSubIe->BcnInterval);
                   break;
                   default:
                   break;
            }

            remIeLen -= curSubIeSize;
            pIeBuf +=curSubIeSize;

    }

    return eCCX_UNPACK_SUCCESS;
}


static eCcxPackUnpackStatus
limUnpackIappFrame(tANI_U8 *pIappFrm, tANI_U32 frameLen, tpCcxUnpackIappFrm pUnpackIappFrm) 
{
   tpCcxIappHdr pIappHdr = (tpCcxIappHdr)pIappFrm;
   tANI_U16 ieLen=1, eidLen= 1, curEid, curIeSize;
   tANI_U16 remLen = 0;
   tANI_U8 *pBuf=NULL;
   tANI_U8 numCount=0;
   eCcxPackUnpackStatus retStatus=eCCX_UNPACK_SUCCESS, status=eCCX_UNPACK_SUCCESS;

   if (frameLen <= sizeof(tCcxIappHdr)) {
       return eCCX_UNPACK_FRM_ERR;
   }

   pIappHdr->IappLen = vos_be16_to_cpu(pIappHdr->IappLen);
   if (pIappHdr->IappLen != (frameLen-SIR_CCX_SNAP_HDR_LENGTH)){
       return eCCX_UNPACK_FRM_ERR;
   }

   remLen = frameLen;
   pBuf = pIappFrm;

   if (pIappHdr->IappType == SIR_CCX_IAPP_TYPE_RADIO_MEAS &&
       pIappHdr->FuncType == SIR_CCX_IAPP_SUBTYPE_RADIO_REQUEST) {
  
       pUnpackIappFrm->pRadioMeasReqHdr = (tpCcxRadioMeasRequest)pIappHdr;
       
       pUnpackIappFrm->pRadioMeasReqHdr->DiagToken = vos_be16_to_cpu(pUnpackIappFrm->pRadioMeasReqHdr->DiagToken);
       remLen -= sizeof(tCcxRadioMeasRequest);
       pBuf += sizeof(tCcxRadioMeasRequest);
       eidLen=ieLen=2;
    }
       
   if (pIappHdr->IappType == SIR_CCX_IAPP_TYPE_ROAM &&
       pIappHdr->FuncType == SIR_CCX_IAPP_SUBTYPE_NEIGHBOR_LIST) {
       pUnpackIappFrm->pNeighborListHdr = (tpCcxNeighborListReport)pIappHdr;
       remLen -= sizeof(tCcxNeighborListReport);
       pBuf += sizeof(tCcxNeighborListReport);
    }
 

    while (remLen) {

           if (remLen <= (eidLen+ieLen))
               return eCCX_UNPACK_FRM_ERR;

           
           if (eidLen == 2) {
               curEid = vos_le16_to_cpu(*((tANI_U16 *)pBuf));
           } else 
               curEid = *((tANI_U8 *)pBuf);

           if (ieLen == 2) {
               curIeSize = vos_le16_to_cpu(*((tANI_U16 *)(pBuf+eidLen)));
           } else 
               curIeSize = *((tANI_U8 *)(pBuf+eidLen));


           if (remLen < (curIeSize + eidLen +ieLen))
               return eCCX_UNPACK_FRM_ERR;

         
            switch (curEid) {

                    case SIR_CCX_EID_MEAS_REQUEST_IE:
                         numCount=0;
                         while ((numCount < SIR_CCX_MAX_MEAS_IE_REQS) && 
                                (pUnpackIappFrm->MeasReqInfo[numCount].pMeasReqIe))
                                 numCount++;

                          if (numCount < SIR_CCX_MAX_MEAS_IE_REQS)
                              status = limUnpackMeasReqIe(pBuf, &(pUnpackIappFrm->MeasReqInfo[numCount]));

                    break;
                    case SIR_CCX_EID_NEIGHBOR_LIST_IE:
                         numCount=0;
                         while ((numCount < SIR_CCX_MAX_NEIGHBOR_IE_REPS) && 
                                (pUnpackIappFrm->NeighborInfo[numCount].pNeighborIe))
                                 numCount++;

                          if (numCount < SIR_CCX_MAX_NEIGHBOR_IE_REPS)
                              status = limUnpackNeighborListIe(pBuf, &(pUnpackIappFrm->NeighborInfo[numCount]));
                    break;
                    default:
                    break;
            }

           if (status)
               retStatus = status;

           pBuf+= (eidLen+ieLen+curIeSize);
           remLen-= (eidLen+ieLen+curIeSize);
    }

    return retStatus;
}

static tSirRetStatus
limProcessCcxBeaconRequest( tpAniSirGlobal pMac, 
                           tpCcxMeasReq pCurMeasReq,
                           tpPESession pSessionEntry )
{
#ifdef WLAN_FEATURE_VOWIFI
   tSirMsgQ mmhMsg;
   tpSirBeaconReportReqInd pSmeBcnReportReq;
   tpBcnRequest pBeaconReq = NULL;
   tANI_U8 counter;

   
   if(eHAL_STATUS_SUCCESS != palAllocateMemory(pMac->hHdd,
                                                (void **) &pSmeBcnReportReq,
                                                (sizeof( tSirBeaconReportReqInd )+pCurMeasReq->numMeasReqIe))) {
      limLog( pMac, LOGP, FL( "Memory Allocation Failure!!! CCX  BcnReq Ind to SME\n" ));
      return eSIR_FAILURE;
   }

   PELOGE(limLog(pMac, LOGE, FL("Sending Beacon Report Req to SME\n")););
   palZeroMemory( pMac->hHdd, pSmeBcnReportReq, (sizeof( tSirBeaconReportReqInd )+ pCurMeasReq->numMeasReqIe));

   pSmeBcnReportReq->messageType = eWNI_SME_BEACON_REPORT_REQ_IND;
   pSmeBcnReportReq->length = sizeof( tSirBeaconReportReqInd );
   palCopyMemory( pMac->hHdd, pSmeBcnReportReq->bssId, pSessionEntry->bssId, sizeof(tSirMacAddr) );
   pSmeBcnReportReq->uDialogToken = pCurMeasReq->DiagToken;
   pSmeBcnReportReq->channelInfo.channelNum = 255;
   pSmeBcnReportReq->channelList.numChannels = pCurMeasReq->numMeasReqIe;

   for (counter=0; counter < pCurMeasReq->numMeasReqIe; counter++) {
        pBeaconReq = (tpBcnRequest)pCurMeasReq->pCurMeasReqIe[counter];
        pSmeBcnReportReq->fMeasurementtype[counter] = pBeaconReq->ScanMode;
        pSmeBcnReportReq->measurementDuration[counter] = SYS_TU_TO_MS(pBeaconReq->MeasDuration);
        pSmeBcnReportReq->channelList.channelNumber[counter] = pBeaconReq->ChanNum;
   }

   
   mmhMsg.type    = eWNI_SME_BEACON_REPORT_REQ_IND;
   mmhMsg.bodyptr = pSmeBcnReportReq;
   MTRACE(macTraceMsgTx(pMac, pSessionEntry->peSessionId, mmhMsg.type));
   limSysProcessMmhMsgApi(pMac, &mmhMsg,  ePROT);
#endif
   return eSIR_SUCCESS;
}



static eCcxPackUnpackStatus
limPackRadioMeasRepHdr(tpAniSirGlobal pMac, tpPESession pSessionEntry, tANI_U8 *pFrmBuf, tpCcxRadioMeasReport pMeasRepHdr, tANI_U16 totalPayload) {

   tANI_U16 totalSize=0;

   palCopyMemory(pMac->hHdd, pMeasRepHdr->IappHdr.AironetSnap, AironetSnapHdr, SIR_CCX_SNAP_HDR_LENGTH);
   sirCopyMacAddr(pMeasRepHdr->IappHdr.SrcMac, pSessionEntry->selfMacAddr);
   pMeasRepHdr->DiagToken =vos_cpu_to_be16(pMeasRepHdr->DiagToken); 

   totalSize = totalPayload + sizeof(tCcxRadioMeasReport)- SIR_CCX_SNAP_HDR_LENGTH;
   pMeasRepHdr->IappHdr.IappLen = vos_cpu_to_be16(totalSize);

   palCopyMemory(pMac->hHdd, pFrmBuf, pMeasRepHdr, sizeof(tCcxRadioMeasReport)); 

   return eCCX_PACK_SUCCESS;
}


static eCcxPackUnpackStatus
limPackMeasRepIe(tpAniSirGlobal pMac, tANI_U8 *pFrmBuf, tpCcxMeasRepIeInfo pMeasRepIeInfo, tANI_U16 remBuffSize, tANI_U16 *ieBuffSize) {

   tpMeasReportIe pMeasRepIe = (tpMeasReportIe)pMeasRepIeInfo->pMeasRepIe;
   tpBcnReportFields pBcnRepFields = pMeasRepIeInfo->pBcnRepFields;
   tpTrafStrmMetrics pTsmFields = pMeasRepIeInfo->pTrafStrmFields;
   tANI_U16 ieSize = 0, ieTotalSize=0;

   ieSize = pMeasRepIe->Length;
   ieTotalSize = ieSize + sizeof(pMeasRepIe->Eid)+ sizeof(pMeasRepIe->Length);

   if (ieTotalSize > remBuffSize)
       return eCCX_PACK_BUFF_OVERFLOW;

   pMeasRepIe->Eid = vos_cpu_to_le16(SIR_CCX_EID_MEAS_REPORT_IE);
   pMeasRepIe->MeasToken = vos_cpu_to_le16(pMeasRepIe->MeasToken);

   if (pBcnRepFields) {
       pBcnRepFields->MeasDuration = vos_cpu_to_le16(pBcnRepFields->MeasDuration);
       pBcnRepFields->BcnInterval = vos_cpu_to_le16(pBcnRepFields->BcnInterval);
       pBcnRepFields->CapabilityInfo = vos_cpu_to_le16(pBcnRepFields->CapabilityInfo);
       pBcnRepFields->ParentTsf = vos_cpu_to_le32(pBcnRepFields->ParentTsf);
   } else if (pTsmFields) {
       pTsmFields->UplinkPktQueueDly = vos_cpu_to_le16(pTsmFields->UplinkPktQueueDly);
       pTsmFields->UplinkPktQueueDlyHist[0] = vos_cpu_to_le16(pTsmFields->UplinkPktQueueDlyHist[0]);
       pTsmFields->UplinkPktQueueDlyHist[1] = vos_cpu_to_le16(pTsmFields->UplinkPktQueueDlyHist[1]);
       pTsmFields->UplinkPktQueueDlyHist[2] = vos_cpu_to_le16(pTsmFields->UplinkPktQueueDlyHist[2]);
       pTsmFields->UplinkPktQueueDlyHist[3] = vos_cpu_to_le16(pTsmFields->UplinkPktQueueDlyHist[3]);
       pTsmFields->UplinkPktTxDly = vos_cpu_to_le32(pTsmFields->UplinkPktTxDly);
       pTsmFields->UplinkPktLoss = vos_cpu_to_le16(pTsmFields->UplinkPktLoss);
       pTsmFields->UplinkPktCount = vos_cpu_to_le16(pTsmFields->UplinkPktCount);
       pTsmFields->RoamingDly = vos_cpu_to_le16(pTsmFields->RoamingDly);
    }

    pMeasRepIe->Length = vos_cpu_to_le16(ieSize);
    palCopyMemory(pMac->hHdd, pFrmBuf, (unsigned char *)pMeasRepIeInfo->pMeasRepIe, ieTotalSize); 
    
    if (ieBuffSize)
        *ieBuffSize = ieTotalSize;

    return eCCX_PACK_SUCCESS;
}



static eCcxPackUnpackStatus
limPackAdjacentApRepHdr(tpAniSirGlobal pMac, tpPESession pSessionEntry, tANI_U8 *pFrmBuf, tpCcxAdjacentApReport pAdjRepHdr, tANI_U16 totalPayload) {

   tANI_U16 totalSize=0;

   palCopyMemory(pMac->hHdd, pAdjRepHdr->IappHdr.AironetSnap, AironetSnapHdr, SIR_CCX_SNAP_HDR_LENGTH);
   sirCopyMacAddr(pAdjRepHdr->IappHdr.SrcMac, pSessionEntry->selfMacAddr);

   totalSize = totalPayload + sizeof(tCcxAdjacentApReport)- SIR_CCX_SNAP_HDR_LENGTH;
   pAdjRepHdr->IappHdr.IappLen = vos_cpu_to_be16(totalSize);

   palCopyMemory(pMac->hHdd, pFrmBuf, pAdjRepHdr, sizeof(tCcxAdjacentApReport)); 

   return eCCX_PACK_SUCCESS;
}

static eCcxPackUnpackStatus
limPackAdjApRepIe(tpAniSirGlobal pMac, tANI_U8 *pFrmBuf, tpCcxAdjApRepIeInfo pAdjApIeInfo, tANI_U16 remBuffSize, tANI_U16 *ieBuffSize) {

   tpAdjacentApRepIe pAdjApRepIe = pAdjApIeInfo->pAdjApIe;
   tANI_U16 ieSize = 0, ieTotalSize=0;

   ieSize = pAdjApRepIe->Length;
   ieTotalSize = ieSize + sizeof(pAdjApRepIe->Eid)+ sizeof(pAdjApRepIe->Length);

   if (ieTotalSize > remBuffSize)
       return eCCX_PACK_BUFF_OVERFLOW;

   pAdjApRepIe->Eid = vos_cpu_to_be16(SIR_CCX_EID_ADJACENT_AP_REPORT_IE);
   pAdjApRepIe->Length = vos_cpu_to_be16(ieSize);
   pAdjApRepIe->ChannelNum = vos_cpu_to_be16(pAdjApRepIe->ChannelNum);
   pAdjApRepIe->SsidLen = vos_cpu_to_be16(pAdjApRepIe->SsidLen);
   pAdjApRepIe->ClientDissSecs = vos_cpu_to_be16(pAdjApRepIe->ClientDissSecs);
   palCopyMemory(pMac->hHdd, pAdjApRepIe->CiscoOui, CiscoOui, sizeof(CiscoOui)); 
   palCopyMemory(pMac->hHdd, pFrmBuf, (unsigned char *)pAdjApRepIe, ieTotalSize); 
    
    if (ieBuffSize)
        *ieBuffSize = ieTotalSize;

    return eCCX_PACK_SUCCESS;
}  


static eCcxPackUnpackStatus
limPackAssocReasonIe(tpAniSirGlobal pMac, tANI_U8 *pFrmBuf, tpCcxAssocReasonIeInfo pAssocReasonInfo, tANI_U16 remBuffSize, tANI_U16 *ieBuffSize) {

   tpAssocReasonIe pAssocReasonIe = pAssocReasonInfo->pAssocReasonIe;
   tANI_U16 ieSize = 0, ieTotalSize=0;

   ieSize = pAssocReasonIe->Length;
   ieTotalSize = ieSize + sizeof(pAssocReasonIe->Eid)+ sizeof(pAssocReasonIe->Length);

   if (ieTotalSize > remBuffSize)
       return eCCX_PACK_BUFF_OVERFLOW;

   pAssocReasonIe->Eid = vos_cpu_to_be16(SIR_CCX_EID_NEW_ASSOC_REASON_IE);
   pAssocReasonIe->Length = vos_cpu_to_be16(ieSize);
   palCopyMemory(pMac->hHdd, pAssocReasonIe->CiscoOui, CiscoOui, sizeof(CiscoOui)); 
   palCopyMemory(pMac->hHdd, pFrmBuf, (unsigned char *)pAssocReasonIe, ieTotalSize); 
    
    if (ieBuffSize)
        *ieBuffSize = ieTotalSize;

    return eCCX_PACK_SUCCESS;
}

static eCcxPackUnpackStatus
limPackIappFrm(tpAniSirGlobal pMac, tpPESession pSessionEntry, tANI_U8 *pBuf,  tpCcxPackIappFrm pPopFrm, tANI_U16 nBuffSize ) {

   tANI_U8 *pFrmBuf = pBuf;
   tANI_U16 buffConsumed=0, totalPayload=0;
   tANI_U16 remSize=nBuffSize, nStatus, totalFrmSize=0;
   tANI_U8 counter=0;

   if( pPopFrm->pRadioMeasRepHdr )
   {
      if (sizeof(tCcxRadioMeasReport) > remSize) {
          limLog( pMac, LOGE, FL("Failure in packing Meas Rep Hdr in Iapp Frm\n") );
          return eCCX_PACK_BUFF_OVERFLOW;
      }

      pFrmBuf += sizeof(tCcxRadioMeasReport);
      remSize -= sizeof(tCcxRadioMeasReport);

      while (pPopFrm->MeasRepInfo[counter].pMeasRepIe) {

         nStatus = limPackMeasRepIe(pMac, pFrmBuf, &pPopFrm->MeasRepInfo[counter], remSize, &buffConsumed);

         if (nStatus) {
             limLog( pMac, LOGE, FL("Failure in packing Meas Report IE in Iapp Frm\n") );
             return nStatus;
         }

         pFrmBuf += buffConsumed;
         totalPayload += buffConsumed;
         remSize -= buffConsumed;
         counter++;

         limLog( pMac, LOGE, FL("Meas IE %d\n\n"),buffConsumed );
         if (counter >= SIR_CCX_MAX_MEAS_IE_REPS)
             break;
       }

      nStatus = limPackRadioMeasRepHdr(pMac, pSessionEntry, pBuf, pPopFrm->pRadioMeasRepHdr, totalPayload);

      totalFrmSize = totalPayload + sizeof(tCcxRadioMeasReport);

      limLog( pMac, LOGE, FL("totalFrmSize %d\n\n"),totalFrmSize);

      if (nStatus) {
          limLog( pMac, LOGE, FL("Failure in packing Meas Rep Hdr in Iapp Frm\n") );
          return nStatus;
      }

    } else if( pPopFrm->pAdjApRepHdr)
    {

      if (sizeof(tCcxAdjacentApReport) > remSize) {
          limLog( pMac, LOGE, FL("Failure in packing Adjacent Ap report in Iapp Frm\n") );
          return eCCX_PACK_BUFF_OVERFLOW;
      }

      pFrmBuf += sizeof(tCcxAdjacentApReport);
      remSize -= sizeof(tCcxAdjacentApReport);

      if (pPopFrm->AdjApRepInfo.pAdjApIe) {

          nStatus = limPackAdjApRepIe(pMac, pFrmBuf, &pPopFrm->AdjApRepInfo, remSize, &buffConsumed);
          if (nStatus) {
              limLog( pMac, LOGE, FL("Failure in packing Adjacent AP Report IE in Iapp Frm\n") );
              return nStatus;
          }

          pFrmBuf += buffConsumed;
          remSize -= buffConsumed;
          totalPayload += buffConsumed;
      }

      if (pPopFrm->AssocReasonInfo.pAssocReasonIe) {

          nStatus = limPackAssocReasonIe(pMac, pFrmBuf, &pPopFrm->AssocReasonInfo, remSize, &buffConsumed);
          if (nStatus) {
              limLog( pMac, LOGE, FL("Failure in packing Assoc Reason IE in Iapp Frm\n") );
              return nStatus;
          }

          pFrmBuf += buffConsumed;
          remSize -= buffConsumed;
          totalPayload += buffConsumed;
      }

      nStatus = limPackAdjacentApRepHdr(pMac, pSessionEntry, pBuf, pPopFrm->pAdjApRepHdr, totalPayload);

      if (nStatus) {
          limLog( pMac, LOGE, FL("Failure in packing Adjacent Ap Rep Hdr in Iapp Frm\n") );
          return nStatus;
      }

    }

    return eCCX_PACK_SUCCESS;
}
 
   
tSirRetStatus
limSendIappFrame(tpAniSirGlobal pMac, tpPESession pSessionEntry, tpCcxPackIappFrm pPopFrm, tANI_U16 nPayload) {

   tSirRetStatus statusCode = eSIR_SUCCESS;
   tpSirMacDataHdr3a pMacHdr;
   tANI_U16 nBytes, nStatus;
   void *pPacket;
   tANI_U8 *pFrame;
   eHalStatus halstatus;
   tANI_U8              txFlag = 0;

   if ( pSessionEntry == NULL ){
      limLog( pMac, LOGE, FL("(psession == NULL) in limSendIappFrame\n") );
      return eSIR_SUCCESS;
   }

   nBytes = nPayload + sizeof(tSirMacDataHdr3a);
   
   halstatus = palPktAlloc( pMac->hHdd, HAL_TXRX_FRM_802_11_MGMT, ( tANI_U16 )nBytes, 
                            ( void** ) &pFrame, ( void** ) &pPacket );

   if ( ! HAL_STATUS_SUCCESS ( halstatus ) )
   {
      limLog( pMac, LOGP, FL("Failed to allocate %d bytes for a IAPP Frm\n"), nBytes);
      return eSIR_FAILURE;
   }

   
   palZeroMemory( pMac->hHdd, pFrame, nBytes);

   
   pMacHdr = ( tpSirMacDataHdr3a ) pFrame;
   pMacHdr->fc.protVer = SIR_MAC_PROTOCOL_VERSION;
   pMacHdr->fc.type    = SIR_MAC_DATA_FRAME;
   pMacHdr->fc.subType = SIR_MAC_DATA_QOS_DATA;
   pMacHdr->fc.toDS = 1;
   pMacHdr->fc.wep = (pSessionEntry->encryptType == eSIR_ED_NONE)? 0 : 1;

   
   sirCopyMacAddr(pMacHdr->addr1,pSessionEntry->bssId);
   
   sirCopyMacAddr(pMacHdr->addr2,pSessionEntry->selfMacAddr);
   
   sirCopyMacAddr(pMacHdr->addr3,pSessionEntry->bssId);
 
   
   nStatus = limPackIappFrm(pMac, pSessionEntry, pFrame + sizeof(tSirMacDataHdr3a), pPopFrm, nPayload );

   if (nStatus) {
       limLog( pMac, LOGE, FL( "Failure in packing IAPP Frame %d\n" ), nStatus);
       palPktFree( pMac->hHdd, HAL_TXRX_FRM_802_11_MGMT, ( void* ) pFrame, ( void* ) pPacket );
       return eSIR_FAILURE;
   }

#if 0
{

   tANI_U16 i;
   tANI_U8 *pBuff;
   pBuff = pFrame;
   printk("FRM\n");
   for (i=0;i<nBytes;i++)
        printk("%x ", pBuff[i]);
   printk("\n");
}
#endif

   limLog( pMac, LOGW, FL( "Sending IAPP Frame %d\n" ), nBytes);

   if( eHAL_STATUS_SUCCESS !=
         (halstatus = halTxFrame( pMac,
                                  pPacket,
                                  (tANI_U16) nBytes,
                                  HAL_TXRX_FRM_802_11_DATA,
                                  ANI_TXDIR_TODS,
                                  7,
                                  limTxComplete,
                                  pFrame, txFlag )))
   {
      PELOGE(limLog( pMac, LOGE, FL( "halTxFrame FAILED! Status [%d]\n" ), halstatus );)
      
      return eSIR_FAILURE;
   }

   return statusCode;
} 


tSirRetStatus limProcessIappRadioMeasFrame(tpAniSirGlobal pMac, tpPESession pSessionEntry, tANI_U8 *pBd) {

   tANI_U8 *pIappFrm = WDA_GET_RX_MPDU_DATA(pBd);
   tANI_U32 frameLen = WDA_GET_RX_PAYLOAD_LEN(pBd);
   tpCcxRadioMeasRequest pRadioMeasFrm = (tpCcxRadioMeasRequest)pIappFrm;
   tCcxUnpackIappFrm unpackedRadioMeasFrm;
   tpMeasRequestIe pCurCacheIe=NULL, pCurMeasReqIe=NULL;
   tCcxPackIappFrm popRadioMeasRepFrm;
   tpMeasReportIe pFailedMeasRepIe=NULL;
   tpCcxRadioMeasReport pRadioMeasRepHdr=NULL;
   tpCcxPEContext pCcxContext = &pSessionEntry->ccxContext;
   tpCcxMeasReq pCurMeasReqCtxt= NULL;
   eCcxPackUnpackStatus parseStatus;
   tANI_U8 counter, numReqCount=0,numFailedReqs=0;
   tANI_U32 numBytes=0;
   tANI_U16 iappFrmSize=0;
   
   PELOGW(limLog(pMac, LOGW, FL("Procesing CCX Radio Meas Request...\n")););

   
   VOS_ASSERT(pRadioMeasFrm->IappHdr.IappType == SIR_CCX_IAPP_TYPE_RADIO_MEAS);

   if (pRadioMeasFrm->IappHdr.FuncType != SIR_CCX_IAPP_SUBTYPE_RADIO_REQUEST)
       return eSIR_SUCCESS;

   palZeroMemory(pMac->hHdd, &popRadioMeasRepFrm, sizeof(popRadioMeasRepFrm));
   palZeroMemory(pMac->hHdd, &unpackedRadioMeasFrm, sizeof(unpackedRadioMeasFrm));
   parseStatus = limUnpackIappFrame(pIappFrm, frameLen, &unpackedRadioMeasFrm);
   
   if (parseStatus) {
       PELOGE(limLog(pMac, LOGE, FL("Parsing Error for CCX Radio Meas Req %d\n"), parseStatus););
       return eSIR_SUCCESS;
   } 

   if (!pCcxContext->curMeasReq.isValid) {
       pCurMeasReqCtxt = &pCcxContext->curMeasReq;
       palZeroMemory(pMac->hHdd, pCurMeasReqCtxt, sizeof(tCcxMeasReq));
       pCurMeasReqCtxt->DiagToken = unpackedRadioMeasFrm.pRadioMeasReqHdr->DiagToken;
       pCurMeasReqCtxt->MeasDly = unpackedRadioMeasFrm.pRadioMeasReqHdr->MeasDly;
       pCurMeasReqCtxt->ActivationOffset = unpackedRadioMeasFrm.pRadioMeasReqHdr->ActivationOffset;
   }

   while (unpackedRadioMeasFrm.MeasReqInfo[numReqCount].pMeasReqIe) {

      pCurMeasReqIe = unpackedRadioMeasFrm.MeasReqInfo[numReqCount].pMeasReqIe;
          
      if ((pCurMeasReqIe->MeasType == SIR_CCX_IAPP_RADIO_BEACON_REQUEST) &&
          ((pCurMeasReqCtxt) && (pCurMeasReqCtxt->numMeasReqIe < SIR_CCX_MAX_MEAS_IE_REQS))){

           tpBcnRequest pBeaconReq = (tpBcnRequest)pCurMeasReqIe;
               
           numBytes = (pCurMeasReqIe->Length + sizeof(pCurMeasReqIe->Eid)+ sizeof(pCurMeasReqIe->Length));

           if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,
                                                        (void **)&(pCurMeasReqCtxt->pCurMeasReqIe[pCurMeasReqCtxt->numMeasReqIe]),
                                                         numBytes)) {
               limLog( pMac, LOGP, FL( "Memory Allocation Failure!!! CCX Beacon Request IE\n" ));
               goto CCX_RADIO_CLEANUP;
            }

            pCurCacheIe = pCurMeasReqCtxt->pCurMeasReqIe[pCurMeasReqCtxt->numMeasReqIe];
            palCopyMemory( pMac->hHdd, pCurCacheIe, pBeaconReq, numBytes );
            pCcxContext->curMeasReq.numMeasReqIe++;

            PELOGW(limLog(pMac, LOGW, FL("Recvd CCXBcnReq token %d scanMode %d chan %d dur %d\n"), pBeaconReq->MeasReqIe.MeasToken,
                              pBeaconReq->ScanMode, pBeaconReq->ChanNum, pBeaconReq->MeasDuration););
       } else {
               
               if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,  
                                                             (void **)& popRadioMeasRepFrm.MeasRepInfo[numFailedReqs].pMeasRepIe,
                                                             sizeof(tMeasReportIe))) {
                   limLog( pMac, LOGP, FL( "Memory Allocation Failure!!! CCX Meas Report IE\n" ));
                   goto CCX_RADIO_CLEANUP;
                }

                iappFrmSize += sizeof(tMeasReportIe);
                pFailedMeasRepIe = popRadioMeasRepFrm.MeasRepInfo[numFailedReqs].pMeasRepIe;
                palZeroMemory(pMac->hHdd, pFailedMeasRepIe, sizeof(tMeasReportIe));
                pFailedMeasRepIe->MeasToken = pCurMeasReqIe->MeasToken; 
                pFailedMeasRepIe->MeasType = pCurMeasReqIe->MeasType; 
                pFailedMeasRepIe->Length = (sizeof(tMeasReportIe)
                                            - sizeof(pFailedMeasRepIe->Length)
                                            - sizeof(pFailedMeasRepIe->Eid));

                if ((pCurMeasReqIe->MeasType != SIR_CCX_IAPP_RADIO_BEACON_REQUEST))
                    pFailedMeasRepIe->MeasRepMode.Incapable  = 1;
                else
                    pFailedMeasRepIe->MeasRepMode.Refused  = 1;

                numFailedReqs++;
                PELOGW(limLog(pMac, LOGW, FL("Recvd CCXRadioMeasReq type %d token %d isRef %d isIn %d\n"), pCurMeasReqIe->MeasType,
                              pCurMeasReqIe->MeasToken, pFailedMeasRepIe->MeasRepMode.Refused, pFailedMeasRepIe->MeasRepMode.Incapable););
       }
       numReqCount++;

       if (numReqCount >= SIR_CCX_MAX_MEAS_IE_REQS)
           break;
   }


   if (pCurMeasReqCtxt && pCurMeasReqCtxt->numMeasReqIe) {
   
       if (eSIR_SUCCESS != limProcessCcxBeaconRequest(pMac, pCurMeasReqCtxt, pSessionEntry)) {
           PELOGE(limLog(pMac, LOGE, FL("Process Beacon Request Returned Failure \n")););

           for (counter=0; counter < SIR_CCX_MAX_MEAS_IE_REQS; counter++) {
                pCurCacheIe = pCurMeasReqCtxt->pCurMeasReqIe[counter];
                if (pCurCacheIe)
                    palFreeMemory(pMac->hHdd, pCurCacheIe);
           }
           goto CCX_RADIO_CLEANUP;
       }
       pCurMeasReqCtxt->isValid = VOS_TRUE;
   }
 
   if (numFailedReqs) {

       if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,  
                                                    (void **)& popRadioMeasRepFrm.pRadioMeasRepHdr,
                                                    sizeof(tCcxRadioMeasReport))) {
           limLog( pMac, LOGP, FL( "Memory Allocation Failure!!! CCX Meas Report Hdr\n" ));
           goto CCX_RADIO_CLEANUP;
        }

       iappFrmSize += sizeof(tCcxRadioMeasReport);
       pRadioMeasRepHdr = popRadioMeasRepFrm.pRadioMeasRepHdr;
       palZeroMemory(pMac->hHdd, pRadioMeasRepHdr, sizeof(tCcxRadioMeasReport));
       pRadioMeasRepHdr->DiagToken = unpackedRadioMeasFrm.pRadioMeasReqHdr->DiagToken;
       pRadioMeasRepHdr->IappHdr.IappType = SIR_CCX_IAPP_TYPE_RADIO_MEAS;
       pRadioMeasRepHdr->IappHdr.FuncType = SIR_CCX_IAPP_SUBTYPE_REPORT;
       sirCopyMacAddr(pRadioMeasRepHdr->IappHdr.SrcMac, pSessionEntry->selfMacAddr);
       
       limSendIappFrame(pMac, pSessionEntry, &popRadioMeasRepFrm, iappFrmSize);
   }

  CCX_RADIO_CLEANUP:
  limCleanupIappPackFrm(pMac, &popRadioMeasRepFrm);

   return eSIR_SUCCESS;
}

tSirRetStatus limProcessIappRoamFrame(tpAniSirGlobal pMac, tpPESession pSessionEntry, tANI_U8 *pBd) 
{
#ifdef WLAN_FEATURE_VOWIFI
   tANI_U8 *pIappFrm = WDA_GET_RX_MPDU_DATA(pBd);
   tANI_U32 frameLen = WDA_GET_RX_PAYLOAD_LEN(pBd);
   tpCcxNeighborListReport pNeighborFrm = (tpCcxNeighborListReport)pIappFrm;
   tCcxUnpackIappFrm unpackedNeighborFrm;
   tpNeighborListIe pCurNeighborIe=NULL;
   eCcxPackUnpackStatus unpackStatus;
   tpSirNeighborReportInd pSmeNeighborRpt = NULL;
   tpSirNeighborBssDescripton pCurBssDesc=NULL;
   tpNeighborListRfSubIe  pCurRfSubIe=NULL;
   tpNeighborListTsfSubIe pCurTsfSubIe=NULL;
   tSirMsgQ mmhMsg;
   tANI_U8 i,numNeighborCount=0;
   tANI_U32 length;

   limLog(pMac, LOGE, FL("Procesing CCX Roam Frame...\n"));
   
   VOS_ASSERT(pNeighborFrm->IappHdr.IappType == SIR_CCX_IAPP_TYPE_ROAM);

   
   if (pNeighborFrm->IappHdr.FuncType != SIR_CCX_IAPP_SUBTYPE_NEIGHBOR_LIST)
       return eSIR_SUCCESS;

   palZeroMemory(pMac->hHdd, &unpackedNeighborFrm, sizeof(unpackedNeighborFrm));
   unpackStatus = limUnpackIappFrame(pIappFrm, frameLen, &unpackedNeighborFrm);
   
   if (unpackStatus) {
       limLog(pMac, LOGE, FL("Parser Returned Error Status for Neighbor List Report %d\n"), unpackStatus);
       return eSIR_SUCCESS;
   } 

   while (unpackedNeighborFrm.NeighborInfo[numNeighborCount].pNeighborIe)
          numNeighborCount++;

   if (!numNeighborCount) {
       limLog(pMac, LOGE, FL("No Neighbors found in Neighbor List Frm\n"));
       return eSIR_SUCCESS;
   }

   length = (sizeof( tSirNeighborReportInd )) +
            (sizeof( tSirNeighborBssDescription ) * (numNeighborCount - 1) ) ; 

   
   if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd, (void **) &pSmeNeighborRpt, length ) )
   {
      limLog( pMac, LOGP, "%s:%d:Unable to allocate memory\n", __func__, __LINE__ );
      return eSIR_MEM_ALLOC_FAILED;

   }
   palZeroMemory( pMac->hHdd, pSmeNeighborRpt, length ); 

            
   for( i = 0 ; i < numNeighborCount ; i++ )
   {
      pCurNeighborIe = unpackedNeighborFrm.NeighborInfo[i].pNeighborIe;
      pCurRfSubIe = unpackedNeighborFrm.NeighborInfo[i].pRfSubIe;
      pCurTsfSubIe = unpackedNeighborFrm.NeighborInfo[i].pTsfSubIe;
      pCurBssDesc = &pSmeNeighborRpt->sNeighborBssDescription[i];

      pCurBssDesc->length = sizeof( tSirNeighborBssDescription ); 
      palCopyMemory( pMac->hHdd, pCurBssDesc->bssId, pCurNeighborIe->Bssid, sizeof(tSirMacAddr) );
      pCurBssDesc->channel = pCurNeighborIe->CurChannel;
      pCurBssDesc->phyType = pCurNeighborIe->PhyType;
      pCurBssDesc->bssidInfo.ccxInfo.channelBand = pCurNeighborIe->ChannelBand;

      if (pCurTsfSubIe) {
          pCurBssDesc->bssidInfo.ccxInfo.tsfOffset = pCurTsfSubIe->TsfOffset;
          pCurBssDesc->bssidInfo.ccxInfo.beaconInterval = pCurTsfSubIe->BcnInterval;
      }
      if (pCurRfSubIe) {
          pCurBssDesc->bssidInfo.ccxInfo.minRecvSigPower = pCurRfSubIe->MinRecvSigPwr;
          pCurBssDesc->bssidInfo.ccxInfo.apTxPower = pCurRfSubIe->ApTxPwr;
          pCurBssDesc->bssidInfo.ccxInfo.roamHysteresis = pCurRfSubIe->RoamHys;
          pCurBssDesc->bssidInfo.ccxInfo.adaptScanThres = pCurRfSubIe->AdaptScanThres;
          pCurBssDesc->bssidInfo.ccxInfo.transitionTime = pCurRfSubIe->TransitionTime;
      }

      limLog(pMac, LOGE, FL("Neighbor:\n"));
      limPrintMacAddr(pMac, pCurBssDesc->bssId, LOGE);
      limLog(pMac, LOGE, FL("Chan: %d PhyType: %d BcnInt: %d minRecvSigPwr: %d\n"),
                             pCurBssDesc->channel,
                             pCurBssDesc->phyType,
                             pCurBssDesc->bssidInfo.ccxInfo.beaconInterval,
                             pCurBssDesc->bssidInfo.ccxInfo.minRecvSigPower);
   }

   pSmeNeighborRpt->messageType = eWNI_SME_NEIGHBOR_REPORT_IND;
   pSmeNeighborRpt->length = length;
   pSmeNeighborRpt->numNeighborReports = numNeighborCount;
   palCopyMemory( pMac->hHdd, pSmeNeighborRpt->bssId, pSessionEntry->bssId, sizeof(tSirMacAddr) );

   
   mmhMsg.type    = pSmeNeighborRpt->messageType;
   mmhMsg.bodyptr = pSmeNeighborRpt;
   MTRACE(macTraceMsgTx(pMac, pSessionEntry->peSessionId, mmhMsg.type));
   limSysProcessMmhMsgApi(pMac, &mmhMsg,  ePROT);
#endif
   return eHAL_STATUS_SUCCESS;
}


void
limProcessIappFrame(tpAniSirGlobal pMac, tANI_U8 *pBd,tpPESession psessionEntry)
{
    tANI_U8 *pBody;
    tpCcxIappHdr pIappHdr;

  
    if (!psessionEntry || !pMac || !pBd) {
        PELOGE(limLog(pMac, LOGE, FL("Invalid Context passed on to ProcessIappFrame\n"));)
        VOS_ASSERT(0);
        return;
    }

    pBody = WDA_GET_RX_MPDU_DATA(pBd);
    pIappHdr = (tpCcxIappHdr) pBody;

    switch (pIappHdr->IappType)
    {
#ifndef FEATURE_DISABLE_RM
        case SIR_CCX_IAPP_TYPE_RADIO_MEAS:
             limProcessIappRadioMeasFrame(pMac, psessionEntry, pBd);
           break;
#endif 
        case SIR_CCX_IAPP_TYPE_ROAM:
             limProcessIappRoamFrame(pMac, psessionEntry, pBd);
            break;
        case SIR_CCX_IAPP_TYPE_REPORT:
        default:
             PELOGE(limLog(pMac, LOGE, FL("Dropping IappFrameType %d\n"), pIappHdr->IappType);)
            break;
    }

    return;
}

#ifdef WLAN_FEATURE_VOWIFI
tSirRetStatus
ccxProcessBeaconReportXmit( tpAniSirGlobal pMac, tpSirBeaconReportXmitInd pBcnReport)
{
   tSirRetStatus status = eSIR_SUCCESS;
   tCcxPackIappFrm packRadioMeasRepFrm;
   tpCcxRadioMeasReport pRadioMeasRepHdr = NULL;
   tpCcxPEContext pCcxContext = NULL;
   tpCcxMeasReq pCurMeasReq = NULL; 
   tpCcxMeasRepIeInfo pMeasRepIeInfo = NULL; 
   tpBcnReportFields pBcnRepFields = NULL;
   tpMeasRequestIe pCurMeasReqIe=NULL;
   tpPESession pSessionEntry ;
   tANI_U8 sessionId,bssCounter,ieCounter, repCounter=0;
   tANI_U16 totalIeSize=0,totalFrmSize=0, totalIeLength=0;


   if(NULL == pBcnReport)
      return eSIR_FAILURE;

  if ((pSessionEntry = peFindSessionByBssid(pMac,pBcnReport->bssId,&sessionId))==NULL){
       PELOGE(limLog(pMac, LOGE,FL("session does not exist for given bssId\n"));)
       return eSIR_FAILURE;
   }

   pCcxContext = &pSessionEntry->ccxContext;

   if (pCcxContext->curMeasReq.isValid)
       pCurMeasReq = &pCcxContext->curMeasReq;

   if ( pCurMeasReq == NULL ){
       PELOGE(limLog( pMac, LOGE, "Received report xmit while there is no request pending in PE\n");)
       return eSIR_FAILURE;
   }
 
   if ( pCurMeasReq->DiagToken != pBcnReport->uDialogToken  ){
       PELOGE(limLog( pMac, LOGE, FL( "Received DiagToken doesnt match, SME: %d PE: %d\n"), pBcnReport->uDialogToken, pCurMeasReq->DiagToken););
       return eSIR_FAILURE;
   }

   PELOGE(limLog( pMac, LOGE, "Recvd CCXBcnRepXmit numBss %d\n", pBcnReport->numBssDesc);)  

   palZeroMemory(pMac->hHdd, &packRadioMeasRepFrm, sizeof(tCcxPackIappFrm));
   if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,  
                                                (void **)& packRadioMeasRepFrm.pRadioMeasRepHdr,
                                                sizeof(tCcxRadioMeasReport))) {
       limLog( pMac, LOGP, FL( "Memory Allocation Failure!!! CCX Meas Report Hdr\n" ));
       goto BCN_REP_CLEANUP;
    }

    totalFrmSize += sizeof(tCcxRadioMeasReport); 
    pRadioMeasRepHdr = packRadioMeasRepFrm.pRadioMeasRepHdr;
    palZeroMemory(pMac->hHdd, pRadioMeasRepHdr, sizeof(tCcxRadioMeasReport));
    pRadioMeasRepHdr->DiagToken = pCurMeasReq->DiagToken;
    pRadioMeasRepHdr->IappHdr.IappType = SIR_CCX_IAPP_TYPE_RADIO_MEAS;
    pRadioMeasRepHdr->IappHdr.FuncType = SIR_CCX_IAPP_SUBTYPE_REPORT;
    sirCopyMacAddr(pRadioMeasRepHdr->IappHdr.SrcMac, pSessionEntry->selfMacAddr);

   for (bssCounter=0; bssCounter < pBcnReport->numBssDesc; bssCounter++) {

        if (!pBcnReport->pBssDescription[bssCounter]) {
            PELOGE(limLog( pMac, LOGE, "BSS Description NOT filled\n");)  
            break;
        }

        if (repCounter >= SIR_CCX_MAX_MEAS_IE_REPS){
            PELOGE(limLog( pMac, LOGE, "Exceeded MAX Meas Rep %d\n", repCounter);)  
            break;
        }

        totalIeSize = (tANI_U8)GET_IE_LEN_IN_BSS( pBcnReport->pBssDescription[bssCounter]->length );
        totalIeLength = sizeof(tMeasReportIe)+ sizeof(tBcnReportFields)+ totalIeSize;

        pMeasRepIeInfo = &packRadioMeasRepFrm.MeasRepInfo[repCounter];

        pCurMeasReqIe = NULL;
        for (ieCounter=0; ieCounter < pCurMeasReq->numMeasReqIe; ieCounter++) {
            if((pCurMeasReq->pCurMeasReqIe[ieCounter]) && 
               (((tpBcnRequest)(pCurMeasReq->pCurMeasReqIe[ieCounter]))->ChanNum == 
                  pBcnReport->pBssDescription[bssCounter]->channelId)) { 
               pCurMeasReqIe = pCurMeasReq->pCurMeasReqIe[ieCounter];
               break;
            }
        }
 
        if (!pCurMeasReqIe){
            PELOGE(limLog( pMac, LOGE, "No Matching RRM Req. Skipping BssDesc\n");)  
            continue;
        }

        if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,  
                                                     (void **)& pMeasRepIeInfo->pMeasRepIe,
                                                     totalIeLength)) {
            limLog( pMac, LOGP, FL( "Memory Allocation Failure!!! CCX Meas Report IE\n" ));
            goto BCN_REP_CLEANUP;
        }

        PELOGE(limLog( pMac, LOGE, "Including BssDescription:\n");)  
        limPrintMacAddr(pMac, pBcnReport->pBssDescription[bssCounter]->bssId, LOGE);

        palZeroMemory(pMac->hHdd, pMeasRepIeInfo->pMeasRepIe, sizeof(tMeasReportIe)+sizeof(tBcnReportFields));
        pMeasRepIeInfo->pBcnRepFields = (tpBcnReportFields)((tANI_U8 *)pMeasRepIeInfo->pMeasRepIe + sizeof(tMeasReportIe));
        pMeasRepIeInfo->pBuf = ((tANI_U8 *)pMeasRepIeInfo->pBcnRepFields + sizeof(tBcnReportFields));

        pMeasRepIeInfo->pMeasRepIe->MeasToken = pCurMeasReqIe->MeasToken; 
        pMeasRepIeInfo->pMeasRepIe->MeasType = SIR_CCX_IAPP_RADIO_BEACON_REQUEST;
        pMeasRepIeInfo->pMeasRepIe->Length = (totalIeLength  
                                              - sizeof(pMeasRepIeInfo->pMeasRepIe->Length)
                                              - sizeof(pMeasRepIeInfo->pMeasRepIe->Eid));

        pBcnRepFields = pMeasRepIeInfo->pBcnRepFields;
        pBcnRepFields->ChanNum = pBcnReport->pBssDescription[bssCounter]->channelId;
        pBcnRepFields->Spare = 0;
        pBcnRepFields->MeasDuration = ((tpBcnRequest)pCurMeasReqIe)->MeasDuration;
        pBcnRepFields->PhyType = pBcnReport->pBssDescription[bssCounter]->nwType;
        pBcnRepFields->RecvSigPower = pBcnReport->pBssDescription[bssCounter]->rssi;
        pBcnRepFields->ParentTsf = pBcnReport->pBssDescription[bssCounter]->parentTSF; 
        pBcnRepFields->TargetTsf[0] = pBcnReport->pBssDescription[bssCounter]->timeStamp[0];
        pBcnRepFields->TargetTsf[1] = pBcnReport->pBssDescription[bssCounter]->timeStamp[1];
        pBcnRepFields->BcnInterval = pBcnReport->pBssDescription[bssCounter]->beaconInterval;
        pBcnRepFields->CapabilityInfo = pBcnReport->pBssDescription[bssCounter]->capabilityInfo;
        palCopyMemory(pMac->hHdd, pBcnRepFields->Bssid, pBcnReport->pBssDescription[bssCounter]->bssId, sizeof(tSirMacAddr));
        palCopyMemory(pMac->hHdd, pMeasRepIeInfo->pBuf, pBcnReport->pBssDescription[bssCounter]->ieFields, totalIeSize);

        totalFrmSize += totalIeLength;
        pCurMeasReq->RepSent[ieCounter] = VOS_TRUE; 
        repCounter++;
   }

   if (pBcnReport->fMeasureDone) {

       for (ieCounter=0; ieCounter < pCurMeasReq->numMeasReqIe; ieCounter++) {

            if((pCurMeasReq->RepSent[ieCounter]))
               continue; 

            if (repCounter >= SIR_CCX_MAX_MEAS_IE_REPS){
                PELOGE(limLog( pMac, LOGE, "Exceeded MAX Meas Rep %d\n", repCounter);)  
                break;
            }

            pMeasRepIeInfo = &packRadioMeasRepFrm.MeasRepInfo[repCounter];
            pCurMeasReqIe = pCurMeasReq->pCurMeasReqIe[ieCounter];
            if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,  
                                                         (void **)& pMeasRepIeInfo->pMeasRepIe,
                                                         sizeof(tMeasReportIe))) {
               limLog( pMac, LOGP, FL( "Memory Allocation Failure!!! CCX Meas Report IE\n" ));
               goto BCN_REP_CLEANUP;
            }

            palZeroMemory(pMac->hHdd, pMeasRepIeInfo->pMeasRepIe, sizeof(tMeasReportIe));
            pMeasRepIeInfo->pMeasRepIe->MeasToken = pCurMeasReqIe->MeasToken; 
            pMeasRepIeInfo->pMeasRepIe->MeasType = SIR_CCX_IAPP_RADIO_BEACON_REQUEST;
            pMeasRepIeInfo->pMeasRepIe->Length = (sizeof(tMeasReportIe) 
                                              - sizeof(pMeasRepIeInfo->pMeasRepIe->Length)
                                              - sizeof(pMeasRepIeInfo->pMeasRepIe->Eid));
            PELOGE(limLog( pMac, LOGE, "Refused MeasToken: %d\n", pCurMeasReqIe->MeasToken);)  
            pMeasRepIeInfo->pMeasRepIe->MeasRepMode.Refused  = 1;
            totalFrmSize += sizeof(tMeasReportIe);
            repCounter++;
      }
   }

   status = limSendIappFrame(pMac, pSessionEntry, &packRadioMeasRepFrm, totalFrmSize);

BCN_REP_CLEANUP:

   if ( pCurMeasReq && pBcnReport->fMeasureDone) {
       limCleanupCcxCtxt(pMac, pSessionEntry); 
   }

   for (bssCounter=0; bssCounter < pBcnReport->numBssDesc; bssCounter++) 
        palFreeMemory(pMac->hHdd, pBcnReport->pBssDescription[bssCounter]);

   limCleanupIappPackFrm(pMac, &packRadioMeasRepFrm);
   return status;
}
#endif


tSirRetStatus limActivateTSMStatsTimer(tpAniSirGlobal pMac,
                                            tpPESession psessionEntry)
{
    if(psessionEntry->ccxContext.tsm.tsmInfo.state)
    {
        tANI_U32   val;
        val = SYS_TU_TO_MS(psessionEntry->ccxContext.tsm.tsmInfo.msmt_interval);
        val = SYS_MS_TO_TICKS(val);
        PELOGW(limLog(pMac, LOGW, FL("CCX:Start the TSM Timer=%ld\n"),val);)
        if(TX_SUCCESS != tx_timer_change (
                         &pMac->lim.limTimers.gLimCcxTsmTimer,
                         val,val ))
        {
            PELOGE(limLog(pMac, LOGE, "Unable to change CCX TSM Stats timer\n");)
            return eSIR_FAILURE;
        }
        pMac->lim.limTimers.gLimCcxTsmTimer.sessionId = 
                                          psessionEntry->peSessionId;
        if (TX_SUCCESS != 
                 tx_timer_activate(&pMac->lim.limTimers.gLimCcxTsmTimer))
        {
            PELOGE(limLog(pMac, LOGP, "could not activate TSM Stats timer\n");)

            return eSIR_FAILURE;
        }
    }
    else 
    {
        limDeactivateAndChangeTimer(pMac,eLIM_TSM_TIMER);
    }
    return eSIR_SUCCESS;
}


tSirRetStatus limProcessTsmTimeoutHandler(tpAniSirGlobal pMac,tpSirMsgQ  limMsg)
{
    tANI_U8  sessionId = pMac->lim.limTimers.gLimCcxTsmTimer.sessionId;
    tpPESession psessionEntry;
    tpAniGetTsmStatsReq pCcxTSMStats = NULL;
    tSirMsgQ msg;

    PELOGW(limLog(pMac, LOG2, FL("Entering limProcessTsmTimeoutHandler\n"));)
    psessionEntry = peFindSessionBySessionId(pMac,sessionId);

    if(NULL==psessionEntry)
    {
        PELOGE(limLog(pMac, LOGE, FL("Invalid  Session ID\n"));)
        return eSIR_FAILURE;
    }
    if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,
                      (void **)&pCcxTSMStats, sizeof(tAniGetTsmStatsReq)))
    {
       PELOGE(limLog(pMac, LOGP, FL("palAllocateMemory() failed\n"));)
       return eSIR_MEM_ALLOC_FAILED;
    }

    palZeroMemory( pMac->hHdd, (tANI_U8 *)pCcxTSMStats, sizeof(tAniGetTsmStatsReq));

    pCcxTSMStats->tid= psessionEntry->ccxContext.tsm.tid;
    palCopyMemory(pMac->hHdd,pCcxTSMStats->bssId,psessionEntry->bssId,sizeof(tSirMacAddr));
    pCcxTSMStats->tsmStatsCallback = NULL;

    msg.type = WDA_TSM_STATS_REQ;
    msg.bodyptr = pCcxTSMStats;
    msg.bodyval = 0;
    PELOGW(limLog(pMac, LOG2, FL("Posting CCX TSM STATS REQ to WDA\n"));)
    if(eSIR_SUCCESS != wdaPostCtrlMsg(pMac, &msg))
    {
       PELOGW(limLog(pMac, LOGE, FL("wdaPostCtrlMsg() failed\n"));)
       if (NULL != pCcxTSMStats)
       {
           palFreeMemory(pMac->hHdd, (tANI_U8*)pCcxTSMStats);
       }
       return eSIR_FAILURE;
    }
    return eSIR_SUCCESS;
}



void limProcessHalCcxTsmRsp(tpAniSirGlobal pMac, tpSirMsgQ limMsg)
{
    tpAniGetTsmStatsRsp pCcxTSMStats = (tpAniGetTsmStatsRsp)limMsg->bodyptr;
    tCcxPackIappFrm packRadioMeasRepFrm;
    tpCcxRadioMeasReport pRadioMeasRepHdr = NULL;
    tpPESession pSessionEntry;
    tANI_U16 totalFrmSize=0,totalIeLength=0;
    tANI_U8  sessionId = pMac->lim.limTimers.gLimCcxTsmTimer.sessionId;
    tpCcxMeasRepIeInfo pMeasRepIeInfo = NULL; 
    tSirRetStatus status;

    if(!pCcxTSMStats)
    {
        PELOGE(limLog( pMac, LOGE,"pCcxTSMStats is NULL" );)
        return;
    }
    pSessionEntry = peFindSessionBySessionId(pMac,sessionId);
    palCopyMemory(pMac->hHdd,&pSessionEntry->ccxContext.tsm.tsmMetrics,
                       &pCcxTSMStats->tsmMetrics, offsetof(tTrafStrmMetrics,RoamingCount)); 

    PELOGE(limLog( pMac, LOG2,"--> Entering limProcessHalCcxTsmRsp" );)
    PELOGE(limLog( pMac, LOGW,"TSM Report:" );)
    PELOGE(limLog( pMac, LOGW,"----------:\n" );)
    PELOGE(limLog( pMac, LOGW,"UplinkPktQueueDly = %d:\n",
               pSessionEntry->ccxContext.tsm.tsmMetrics.UplinkPktQueueDly );)
               
    PELOGE(limLog( pMac, LOGW,FL("UplinkPktQueueDlyHist = %d %d %d %d:\n"),
               pSessionEntry->ccxContext.tsm.tsmMetrics.UplinkPktQueueDlyHist[0],
               pSessionEntry->ccxContext.tsm.tsmMetrics.UplinkPktQueueDlyHist[1],
               pSessionEntry->ccxContext.tsm.tsmMetrics.UplinkPktQueueDlyHist[2],
               pSessionEntry->ccxContext.tsm.tsmMetrics.UplinkPktQueueDlyHist[3]);)

    PELOGE(limLog( pMac, LOGW,"UplinkPktTxDly = %d:\n",
               pSessionEntry->ccxContext.tsm.tsmMetrics.UplinkPktTxDly );)

    PELOGE(limLog( pMac, LOGW,"UplinkPktLoss = %d:\n",
               pSessionEntry->ccxContext.tsm.tsmMetrics.UplinkPktLoss );)

    PELOGE(limLog( pMac, LOGW,"UplinkPktCount = %d:\n",
               pSessionEntry->ccxContext.tsm.tsmMetrics.UplinkPktCount );)

    PELOGE(limLog( pMac, LOGW,"RoamingCount = %d:\n",
               pSessionEntry->ccxContext.tsm.tsmMetrics.RoamingCount );)

    PELOGE(limLog( pMac, LOGW,"RoamingDly = %d:\n",
               pSessionEntry->ccxContext.tsm.tsmMetrics.RoamingDly );)

    palZeroMemory(pMac->hHdd, &packRadioMeasRepFrm, sizeof(tCcxPackIappFrm));
    if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,  
                              (void **)& packRadioMeasRepFrm.pRadioMeasRepHdr,
                               sizeof(tCcxRadioMeasReport))) {
       PELOGE(limLog( pMac, LOGP,  
                     "Memory Allocation Failure!!! CCX Meas Report Hdr\n" );)
       return;
    }

    totalFrmSize += sizeof(tCcxRadioMeasReport);

    pRadioMeasRepHdr = packRadioMeasRepFrm.pRadioMeasRepHdr;
    palZeroMemory(pMac->hHdd, pRadioMeasRepHdr, sizeof(tCcxRadioMeasReport));
    pRadioMeasRepHdr->DiagToken = 0x0;
    pRadioMeasRepHdr->IappHdr.IappType = SIR_CCX_IAPP_TYPE_RADIO_MEAS;
    pRadioMeasRepHdr->IappHdr.FuncType = SIR_CCX_IAPP_SUBTYPE_REPORT;
    sirCopyMacAddr(pRadioMeasRepHdr->IappHdr.SrcMac, pSessionEntry->selfMacAddr);
    totalIeLength = sizeof(tMeasReportIe) + sizeof(tTrafStrmMetrics);

    pMeasRepIeInfo = &packRadioMeasRepFrm.MeasRepInfo[0];
    if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,  
                                   (void **)&pMeasRepIeInfo->pMeasRepIe,
                                    totalIeLength))
    {
        PELOGE(limLog( pMac, LOGP, 
                   "Memory Allocation Failure!!! CCX Meas Report IE\n" );)
        return;
    }
    palZeroMemory(pMac->hHdd, (void*)pMeasRepIeInfo->pMeasRepIe, totalIeLength);
    pMeasRepIeInfo->pMeasRepIe->Eid = SIR_CCX_EID_MEAS_REPORT_IE;
    pMeasRepIeInfo->pMeasRepIe->Length = (totalIeLength
                            - sizeof(pMeasRepIeInfo->pMeasRepIe->Length)
                            - sizeof(pMeasRepIeInfo->pMeasRepIe->Eid));
    pMeasRepIeInfo->pMeasRepIe->MeasToken =0x0; 
    pMeasRepIeInfo->pMeasRepIe->MeasType = SIR_CCX_IAPP_RADIO_TSM_REQUEST;
    pMeasRepIeInfo->pTrafStrmFields= (tTrafStrmMetrics *)
                                     ((tANI_U8*)pMeasRepIeInfo->pMeasRepIe
                                      + sizeof(tMeasReportIe));

    palCopyMemory(pMac->hHdd,pMeasRepIeInfo->pTrafStrmFields,
                       &pSessionEntry->ccxContext.tsm.tsmMetrics,
                       sizeof(tTrafStrmMetrics));

    totalFrmSize += totalIeLength;
    status = limSendIappFrame(pMac, pSessionEntry, 
                                &packRadioMeasRepFrm, totalFrmSize);

    
    palZeroMemory(pMac->hHdd, &pSessionEntry->ccxContext.tsm.tsmMetrics,
                                                 sizeof(tTrafStrmMetrics));

    limActivateTSMStatsTimer(pMac, pSessionEntry);

    if(pCcxTSMStats != NULL)
        palFreeMemory( pMac->hHdd, (void *)pCcxTSMStats );
    
    if (packRadioMeasRepFrm.pRadioMeasRepHdr)
        palFreeMemory(pMac->hHdd, packRadioMeasRepFrm.pRadioMeasRepHdr);

    if(pMeasRepIeInfo->pMeasRepIe)
        palFreeMemory(pMac->hHdd, pMeasRepIeInfo->pMeasRepIe);
}

tSirRetStatus 
limProcessAdjacentAPRepMsg (tpAniSirGlobal pMac,tANI_U32 *pMsgBuf)
{
   tpSirAdjacentApRepInd pAdjApRep = (tpSirAdjacentApRepInd )pMsgBuf;
   tSirRetStatus status = eSIR_SUCCESS;
   tCcxPackIappFrm packAdjApFrm;
   tpCcxAdjacentApReport pAdjRepHdr = NULL;
   tpAdjacentApRepIe pAdjRepIe = NULL;
   tpAssocReasonIe pAssocIe = NULL;
   tpPESession pSessionEntry ;
   tANI_U8 sessionId;
   tANI_U16 totalFrmSize=0;

   if(NULL == pAdjApRep)
      return eSIR_FAILURE;

   if ((pSessionEntry = peFindSessionByBssid(pMac,pAdjApRep->bssid,&sessionId))==NULL){
       PELOGE(limLog(pMac, LOGE,FL("session does not exist for given bssId\n"));)
       return eSIR_FAILURE;
   }

   PELOGE(limLog( pMac, LOGE, "Recvd CCX Adjacent AP Report\n");)  

   palZeroMemory(pMac->hHdd, &packAdjApFrm, sizeof(tCcxPackIappFrm));
   if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,  
                                                (void **)& packAdjApFrm.pAdjApRepHdr,
                                                sizeof(tCcxRadioMeasReport))) {
       limLog( pMac, LOGP, FL( "Memory Allocation Failure!!! CCX Adj Report Hdr\n" ));
       goto ADJ_REP_CLEANUP;
   }
   
   pSessionEntry->ccxContext.tsm.tsmMetrics.RoamingDly = pAdjApRep->tsmRoamdelay;
   
   totalFrmSize += sizeof(tCcxAdjacentApReport); 
   pAdjRepHdr = packAdjApFrm.pAdjApRepHdr;
   palZeroMemory(pMac->hHdd, pAdjRepHdr, sizeof(tCcxAdjacentApReport));
   pAdjRepHdr->IappHdr.IappType = SIR_CCX_IAPP_TYPE_REPORT;
   pAdjRepHdr->IappHdr.FuncType = SIR_CCX_IAPP_SUBTYPE_ADJACENTAP;
   sirCopyMacAddr(pAdjRepHdr->IappHdr.SrcMac, pSessionEntry->selfMacAddr);

   if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,  
                                                (void **)& packAdjApFrm.AdjApRepInfo.pAdjApIe,
                                                sizeof(tAdjacentApRepIe))) {
       limLog( pMac, LOGP, FL( "Memory Allocation Failure!!! CCX Adjacent AP report IE\n" ));
       goto ADJ_REP_CLEANUP;
   }

   
   totalFrmSize += sizeof(tAdjacentApRepIe); 
   pAdjRepIe = packAdjApFrm.AdjApRepInfo.pAdjApIe;
   palZeroMemory(pMac->hHdd, pAdjRepIe, sizeof(tAdjacentApRepIe));
   pAdjRepIe->Eid = SIR_CCX_EID_ADJACENT_AP_REPORT_IE;
   pAdjRepIe->Length = sizeof(tAdjacentApRepIe) -
                       sizeof(pAdjRepIe->Eid)-sizeof(pAdjRepIe->Length);
   sirCopyMacAddr(pAdjRepIe->Bssid, pAdjApRep->prevApMacAddr);
   pAdjRepIe->ChannelNum = pAdjApRep->channelNum;
   pAdjRepIe->SsidLen = pAdjApRep->prevApSSID.length;
   palCopyMemory( pMac->hHdd, pAdjRepIe->Ssid, pAdjApRep->prevApSSID.ssId, pAdjRepIe->SsidLen );
   pAdjRepIe->ClientDissSecs = pAdjApRep->clientDissSecs;

   if( eHAL_STATUS_SUCCESS != palAllocateMemory( pMac->hHdd,  
                                                (void **)& packAdjApFrm.AssocReasonInfo.pAssocReasonIe,
                                                sizeof(tAssocReasonIe))) {
       limLog( pMac, LOGP, FL( "Memory Allocation Failure!!! CCX Adjacent AP report IE\n" ));
       goto ADJ_REP_CLEANUP;
   }

   
   totalFrmSize += sizeof(tAssocReasonIe); 
   pAssocIe = packAdjApFrm.AssocReasonInfo.pAssocReasonIe;
   palZeroMemory(pMac->hHdd, pAssocIe, sizeof(tAssocReasonIe));
   pAssocIe->Eid = SIR_CCX_EID_NEW_ASSOC_REASON_IE;
   pAssocIe->Length = sizeof(tAssocReasonIe) - 
                      sizeof(pAssocIe->Eid)-sizeof(pAssocIe->Length);
   pAssocIe->AssocReason = pAdjApRep->roamReason;
 
   status = limSendIappFrame(pMac, pSessionEntry, &packAdjApFrm, totalFrmSize);

ADJ_REP_CLEANUP:

   limCleanupIappPackFrm(pMac, &packAdjApFrm);

   return status;
}    

#endif 
