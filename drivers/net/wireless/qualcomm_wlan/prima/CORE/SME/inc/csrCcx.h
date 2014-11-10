/** ------------------------------------------------------------------------- * 
    ------------------------------------------------------------------------- *  

  
    \file csrCcx.h
  
    Definitions used only by CCX extensions to CSR.
  
    Copyright (C) 2011,2013 Qualcomm, Incorporated
  
 
   ========================================================================== */
#ifndef CSR_CCX_H__
#define CSR_CCX_H__


#include "csrSupport.h"
#include "smeInside.h"
#include "vos_nvitem.h"

#ifdef FEATURE_WLAN_CCX


#define __ATTRIB_PACK __attribute__((__packed__))

#define IEEE80211_IE_LEN(a) a[0]
#define IEEE80211_ELEM_LEN(a) (a-2)

#define CCX_CCKM_ELEMID                 0x9c
#define CCX_CCKM_REASSOC_REQ_IE_LEN     sizeof(tCsrCckmReassocReqIe)
#define CCX_CCKM_OUI_TYPE               0
#define CCX_TPC_IELEN                   6
#define CCX_AIRONET_ELEMID              0x85
#define CCX_AIRONET_DEVICEID            0x66
#define CCX_TSRS_OUI_TYPE 8
#define CCX_TSRS_MIN_LEN  6
#define CCX_TSRS_MAX_LEN  13

#define CAC_DATA_RATE               2000
#define QBSSLOAD_PER_2000_DATA_RATE 32   



typedef struct tagCsrCckmReassocReqIe {
        tANI_U8  cckm_id;
        tANI_U8  cckm_len;
        tANI_U8  cckm_oui[3];
        tANI_U8  cckm_oui_type;
        union {
            tANI_U8     data[8];
            tANI_U64    tsf;
        } cur_tsf_timer;
        tANI_U32 cckm_reassoc_req_num;
        tANI_U8  cckm_mic[8];
} __ATTRIB_PACK tCsrCckmReassocReqIe;



tANI_U8 csrConstructCcxCckmIe( tHalHandle hHal, tCsrRoamSession *pSession, 
        tCsrRoamProfile *pProfile, tSirBssDescription *pBssDescription, 
        void *pRsnIe, tANI_U8 rsn_ie_len, void *pCckmIe);

void ProcessIAPPNeighborAPList(void *context);
void csrNeighborRoamIndicateVoiceBW( tpAniSirGlobal pMac, v_U32_t peak_data_rate, int AddmissionCheckFlag );

#endif 
#endif 
