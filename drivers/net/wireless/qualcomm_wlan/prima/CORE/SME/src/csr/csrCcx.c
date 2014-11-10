/** ------------------------------------------------------------------------- *
    ------------------------------------------------------------------------- *


    \file csrCcx.c

    Implementation supporting routines for CCX.

    Copyright (c) 2011-2013 Qualcomm Atheros, Inc.
    All Rights Reserved.
    Qualcomm Atheros Confidential and Proprietary.

    Copyright (C) 2011 Qualcomm, Incorporated

   ========================================================================== */
#ifdef FEATURE_WLAN_NON_INTEGRATED_SOC
#include "halInternal.h" 
#endif

#include "aniGlobal.h"
#include "palApi.h"
#include "csrSupport.h"
#include "csrInsideApi.h"
#include "smsDebug.h"
#include "smeQosInternal.h"
#include "csrNeighborRoam.h"
#ifdef FEATURE_WLAN_CCX
#include "vos_utils.h"
#include "csrCcx.h"
#endif 



#ifdef FEATURE_WLAN_CCX



static void
csrCcxAddCckmMic(eCsrAuthType negotiatedAuthType, tANI_U8 *my_addr, tANI_U8 *bssid,
                 tANI_U8 *rsn_ie, tANI_U8 rsn_ie_len, tCsrCcxCckmInfo *cckm_info, 
                 tCsrCckmReassocReqIe *cckm_ie)
{
    tANI_U8 cckm_mic_buf[512], pos = 0 , mic[20];
    tANI_U8 prev_pos=0;
#ifdef MIC_DEBUG
    int i = 0;
#endif

    
    vos_mem_set(&cckm_mic_buf[0], sizeof(cckm_mic_buf), 0);
    
    vos_mem_copy(&cckm_mic_buf[pos], my_addr, WNI_CFG_STA_ID_LEN);
    pos += WNI_CFG_STA_ID_LEN;

    
#ifdef MIC_DEBUG
    printk("My addr\n");
    for(i=0; i<pos; i++)
          printk("%s: %02x\n", __func__, cckm_mic_buf[i]);
#endif
    prev_pos = pos;

    vos_mem_copy(&cckm_mic_buf[pos], bssid, WNI_CFG_STA_ID_LEN);
    pos += WNI_CFG_STA_ID_LEN;

#ifdef MIC_DEBUG
    printk("bssid\n");
    for(i=prev_pos; i<pos; i++)
          printk("%s: %02x\n", __func__, cckm_mic_buf[i]);
#endif
    prev_pos = pos;

    vos_mem_copy(&cckm_mic_buf[pos], rsn_ie, rsn_ie_len);
    pos += rsn_ie_len;

#ifdef MIC_DEBUG
    printk("rsnie\n");
    for(i=prev_pos; i<pos; i++)
          printk("%s: %02x\n", __func__, cckm_mic_buf[i]);
#endif
    prev_pos = pos;

    vos_mem_copy(&cckm_mic_buf[pos], cckm_ie->cur_tsf_timer.data,
                 sizeof(cckm_ie->cur_tsf_timer));
    pos += sizeof(cckm_ie->cur_tsf_timer);

#ifdef MIC_DEBUG
    printk("TSF\n");
    for(i=prev_pos; i<pos; i++)
          printk("%s: %02x\n", __func__, cckm_mic_buf[i]);
#endif
    prev_pos = pos;

    vos_mem_copy(&cckm_mic_buf[pos], &cckm_ie->cckm_reassoc_req_num,
                 sizeof(cckm_ie->cckm_reassoc_req_num));
    pos += sizeof(cckm_ie->cckm_reassoc_req_num);

#ifdef MIC_DEBUG
    printk("req_num\n");
    for(i=prev_pos; i<pos; i++)
          printk("%s: %02x\n", __func__, cckm_mic_buf[i]);
#endif
    prev_pos = pos;

#ifdef MIC_DEBUG
    printk("krk\n");
    for(i=0; i<16; i++)
          printk("%s: %02x\n", __func__, cckm_info->krk[i]);
#endif

#if 1
    if (negotiatedAuthType == eCSR_AUTH_TYPE_CCKM_RSN) {
        vos_sha1_hmac_str(0, cckm_mic_buf, pos, cckm_info->krk, sizeof(cckm_info->krk), 
                mic); 
    } else {
        vos_md5_hmac_str(0, cckm_mic_buf, pos, cckm_info->krk, sizeof(cckm_info->krk), 
                mic); 
    } 
#else
    
    vos_sha1_hmac_str(0, cckm_mic_buf, pos, cckm_info->krk, sizeof(cckm_info->krk), 
            mic); 
#endif
    vos_mem_copy(cckm_ie->cckm_mic, mic, sizeof(cckm_ie->cckm_mic));
#ifdef MIC_DEBUG
    printk("mic\n");
    for(i=0; i<8; i++)
          printk("%s: %02x\n", __func__, mic[i]);
#endif
}

const tANI_U8  ouiAironet[] = { 0x00, 0x40, 0x96 };


tANI_U8 csrConstructCcxCckmIe( tHalHandle hHal, tCsrRoamSession *pSession, 
        tCsrRoamProfile *pProfile, tSirBssDescription *pBssDescription, 
        void *pRsnIe, tANI_U8 rsn_ie_len, void *pCckmIe) 
{
    tANI_U8 cbCcxCckmIe = 0;
    tCsrCckmReassocReqIe cckm_ie;
    tANI_U32 timer_diff = 0;
    tANI_U32 timeStamp[2];


    do {
        if ( !csrIsProfileCCX( pProfile) ) 
        {
            break;
        } 
        
        cckm_ie.cckm_id = CCX_CCKM_ELEMID; 
        cckm_ie.cckm_len = IEEE80211_ELEM_LEN(CCX_CCKM_REASSOC_REQ_IE_LEN); 
        vos_mem_copy( cckm_ie.cckm_oui, 
                ouiAironet, 
                sizeof ( cckm_ie.cckm_oui ));
        cckm_ie.cckm_oui_type = CCX_CCKM_OUI_TYPE ; 

        
        timer_diff = vos_timer_get_system_time() - pBssDescription->scanSysTimeMsec;
        
        timer_diff = (tANI_U32)(timer_diff * SYSTEM_TIME_MSEC_TO_USEC); 

        timeStamp[0] = pBssDescription->timeStamp[0];
        timeStamp[1] = pBssDescription->timeStamp[1];

        

        UpdateCCKMTSF(&(timeStamp[0]), &(timeStamp[1]), &timer_diff);

        

        vos_mem_copy( cckm_ie.cur_tsf_timer.data, 
                (void *) &timeStamp[0], 
                sizeof ( cckm_ie.cur_tsf_timer.data ));
        
        cckm_ie.cckm_reassoc_req_num = ++(pSession->ccxCckmInfo.reassoc_req_num);

        csrCcxAddCckmMic(pProfile->negotiatedAuthType, pSession->selfMacAddr, 
                pBssDescription->bssId, pRsnIe, rsn_ie_len, 
                &pSession->ccxCckmInfo, &cckm_ie);

        
        vos_mem_copy(pCckmIe, &cckm_ie, CCX_CCKM_REASSOC_REQ_IE_LEN);

        
        cbCcxCckmIe = CCX_CCKM_REASSOC_REQ_IE_LEN;

    } while(0);

    return (cbCcxCckmIe);
}


void ProcessIAPPNeighborAPList(void *context)
{
    tpAniSirGlobal pMac = PMAC_STRUCT(context);
    tpCsrNeighborRoamControlInfo    pNeighborRoamInfo = &pMac->roam.neighborRoamInfo;
#ifdef WLAN_FEATURE_VOWIFI
    VOS_STATUS  status = VOS_STATUS_SUCCESS;
#endif

    if (csrNeighborRoamIsCCXAssoc(pMac) == FALSE) return;

    
    switch (pNeighborRoamInfo->neighborRoamState)
    {
        case eCSR_NEIGHBOR_ROAM_STATE_CONNECTED:
        case eCSR_NEIGHBOR_ROAM_STATE_CFG_CHAN_LIST_SCAN:
            
#ifdef WLAN_FEATURE_VOWIFI
            status = csrNeighborRoamCreateChanListFromNeighborReport(pMac);
            if (VOS_STATUS_SUCCESS == status)
            {
                smsLog(pMac, LOGE, FL("IAPP Neighbor list callback received as expected in state %d."), 
                    pNeighborRoamInfo->neighborRoamState);
                pNeighborRoamInfo->roamChannelInfo.IAPPNeighborListReceived = eANI_BOOLEAN_TRUE;
            }
#endif
            break;

        default:
            smsLog(pMac, LOGE, FL("Neighbor list callback not expected in state %d, Ignoring.."), pNeighborRoamInfo->neighborRoamState);
            break;
    }
    return;
}

void csrNeighborRoamIndicateVoiceBW( tpAniSirGlobal pMac, v_U32_t peak_data_rate, int AddmissionCheckFlag )
{
    tpCsrNeighborRoamControlInfo    pNeighborRoamInfo = &pMac->roam.neighborRoamInfo;

    if (csrNeighborRoamIsCCXAssoc(pMac) == FALSE) 
    {
        pNeighborRoamInfo->isVOAdmitted = FALSE;
        return;
    }

    pNeighborRoamInfo->isVOAdmitted = AddmissionCheckFlag;
    if (AddmissionCheckFlag)
    {
        if (peak_data_rate >= CAC_DATA_RATE)
            pNeighborRoamInfo->MinQBssLoadRequired = ((peak_data_rate)/CAC_DATA_RATE) * QBSSLOAD_PER_2000_DATA_RATE;
        else 
            pNeighborRoamInfo->isVOAdmitted = FALSE; 
    }

    smsLog(pMac, LOGE, FL("Data Rate = %x MinQBssLoadRequired = %x Check = %d\n"),
        (unsigned int)peak_data_rate, (unsigned int)pNeighborRoamInfo->MinQBssLoadRequired, 
        pNeighborRoamInfo->isVOAdmitted);
}


#endif 

