#if !defined( __CCXGLOBAL_H )
#define __CCXGLOBAL_H

/**=========================================================================

  \file  ccxGlobal.h

  \brief Definitions for CCX specific defines.

  Copyright (c) 2012-2013 Qualcomm Atheros, Inc.
  All Rights Reserved.
  Qualcomm Atheros Confidential and Proprietary.

  ========================================================================*/


#define SIR_CCX_SNAP_HDR_LENGTH   8
#define SIR_MAC_CCX_OUI_LEN       4
#define SIR_MAC_CCX_MAX_SSID_LEN 32
#define SIR_CCX_IAPP_TYPE_REPORT                0x30
#define SIR_CCX_IAPP_SUBTYPE_ADJACENTAP         0x0

#define SIR_CCX_IAPP_TYPE_RADIO_MEAS             0x32
#define SIR_CCX_IAPP_SUBTYPE_RADIO_REQUEST       0x1
#define SIR_CCX_IAPP_SUBTYPE_REPORT              0x81

#define SIR_CCX_IAPP_RADIO_UNUSED_REQUEST         0x0
#define SIR_CCX_IAPP_RADIO_CHNL_LOAD_REQUEST      0x1
#define SIR_CCX_IAPP_RADIO_NOISE_HIST_REQUEST     0x2
#define SIR_CCX_IAPP_RADIO_BEACON_REQUEST         0x3
#define SIR_CCX_IAPP_RADIO_FRAME_REQUEST          0x4
#define SIR_CCX_IAPP_RADIO_HIDDEN_NODE_REQUEST    0x5
#define SIR_CCX_IAPP_RADIO_TSM_REQUEST            0x6

#define SIR_CCX_MAX_MEAS_REQ                      1
#define SIR_CCX_MAX_MEAS_IE_REQS                  8
#define SIR_CCX_MAX_MEAS_IE_REPS                  8
#define SIR_CCX_MAX_NEIGHBOR_IE_REPS              30
#define SIR_BCN_REPORT_MAX_BSS_DESC                4

#define SIR_CCX_IAPP_TYPE_ROAM                   0x33
#define SIR_CCX_IAPP_SUBTYPE_NEIGHBOR_LIST       0x81
#define SIR_CCX_IAPP_SUBTYPE_DIRECTED_ROAM       0x82

#define SIR_CCX_EID_MEAS_REQUEST_IE              0x26
#define SIR_CCX_EID_MEAS_REPORT_IE               0x27
#define SIR_CCX_EID_NEIGHBOR_LIST_IE             0x28
#define SIR_CCX_EID_ADJACENT_AP_REPORT_IE        0x9b
#define SIR_CCX_EID_NEW_ASSOC_REASON_IE          0x9c
#define SIR_CCX_EID_NEIGHBOR_LIST_RF_SUBIE       0x1
#define SIR_CCX_EID_NEIGHBOR_LIST_TSF_SUBIE      0x2

#define SIR_CCX_ASSOC_REASON_UNSPECIFIED         0x0
#define SIR_CCX_ASSOC_REASON_NORMAL_ROAM         0x1
#define SIR_CCX_ASSOC_REASON_LOAD_BALANCE        0x2
#define SIR_CCX_ASSOC_REASON_TSPEC_REJECT        0x3
#define SIR_CCX_ASSOC_REASON_DIRECT_ROAM         0x4
#define SIR_CCX_ASSOC_REASON_FIRST_ASSOC         0x5
#define SIR_CCX_ASSOC_REASON_IN_CELLUAR_ROAM     0x6
#define SIR_CCX_ASSOC_REASON_OUT_CELLUAR_ROAM    0x7
#define SIR_CCX_ASSOC_REASON_BETTER_AP           0x8
#define SIR_CCX_ASSOC_REASON_DEAUTH_ROAM         0x9
typedef struct sSirAdjacentApRepInd
{
   tANI_U16     messageType; 
   tANI_U16     length;
   tSirMacAddr  bssid;
   tSirMacAddr  prevApMacAddr;
   tANI_U8      channelNum;
   tSirMacSSid  prevApSSID;
   tANI_U16     clientDissSecs;
   tANI_U8      roamReason;
   tANI_U16     tsmRoamdelay;
} tSirAdjacentApRepInd, *tpSirAdjacentApRepInd;


typedef __ani_attr_pre_packed struct sCcxIappHdr
{
    tANI_U8 AironetSnap[SIR_CCX_SNAP_HDR_LENGTH];
    tANI_U16 IappLen;
    tANI_U8  IappType;
    tANI_U8  FuncType;
    tANI_U8  DestMac[SIR_MAC_ADDR_LENGTH];
    tANI_U8  SrcMac[SIR_MAC_ADDR_LENGTH];
} __ani_attr_packed tCcxIappHdr, *tpCcxIappHdr;

typedef __ani_attr_pre_packed struct sAdjacentApRepIe 
{
    tANI_U16      Eid;
    tANI_U16      Length;
    tANI_U8       CiscoOui[SIR_MAC_CCX_OUI_LEN];
    tANI_U8       Bssid[SIR_MAC_ADDR_LENGTH];
    tANI_U16      ChannelNum;
    tANI_U16      SsidLen;
    tANI_U8       Ssid[SIR_MAC_CCX_MAX_SSID_LEN];
    tANI_U16      ClientDissSecs;
} __ani_attr_packed tAdjacentApRepIe, *tpAdjacentApRepIe;

typedef __ani_attr_pre_packed struct sAssocReasonIe 
{
    tANI_U16      Eid;
    tANI_U16      Length;
    tANI_U8       CiscoOui[SIR_MAC_CCX_OUI_LEN];
    tANI_U8       AssocReason;
} __ani_attr_packed tAssocReasonIe, *tpAssocReasonIe;

typedef __ani_attr_pre_packed struct sNeighborListIe 
{
    tANI_U8       Eid;
    tANI_U8       Length;
    tANI_U8       Bssid[SIR_MAC_ADDR_LENGTH];
    tANI_U8       CurChannel;
    tANI_U8       ChannelBand;
    tANI_U8       PhyType;
} __ani_attr_packed tNeighborListIe, *tpNeighborListIe;

typedef __ani_attr_pre_packed struct sNeighborListRfSubIe 
{
    tANI_U8       SubEid;
    tANI_U8       Length;
    tANI_U8       MinRecvSigPwr;
    tANI_U8       ApTxPwr;
    tANI_U8       RoamHys;
    tANI_U8       AdaptScanThres;
    tANI_U8       TransitionTime;
} __ani_attr_packed tNeighborListRfSubIe, *tpNeighborListRfSubIe;

typedef __ani_attr_pre_packed struct sNeighborListTsfSubIe 
{
    tANI_U8       SubEid;
    tANI_U8       Length;
    tANI_U16      TsfOffset;
    tANI_U16      BcnInterval;
} __ani_attr_packed tNeighborListTsfSubIe, *tpNeighborListTsfSubIe;

typedef __ani_attr_pre_packed struct sMeasReqMode 
{
    tANI_U8       Parallel: 1;
    tANI_U8       Enable: 1;
    tANI_U8       NotUsed: 1;
    tANI_U8       Report: 1;
    tANI_U8       Reserved: 4;
} __ani_attr_packed tMeasReqMode, *tpMeasReqMode;

typedef __ani_attr_pre_packed struct sMeasRepMode 
{
    tANI_U8       Parallel: 1;
    tANI_U8       Incapable: 1;
    tANI_U8       Refused: 1;
    tANI_U8       Reserved: 5;
} __ani_attr_packed tMeasRepMode, *tpMeasRepMode;

typedef __ani_attr_pre_packed struct sMeasRequestIe 
{
    tANI_U16      Eid;
    tANI_U16      Length;
    tANI_U16      MeasToken;
    tMeasReqMode  MeasReqMode;
    tANI_U8       MeasType;
} __ani_attr_packed tMeasRequestIe, *tpMeasRequestIe;

typedef __ani_attr_pre_packed struct sMeasReportIe 
{
    tANI_U16      Eid;
    tANI_U16      Length;
    tANI_U16      MeasToken;
    tMeasRepMode  MeasRepMode;
    tANI_U8       MeasType;
} __ani_attr_packed tMeasReportIe, *tpMeasReportIe;

typedef __ani_attr_pre_packed struct sBcnRequest 
{
    tMeasRequestIe MeasReqIe;
    tANI_U8        ChanNum;
    tANI_U8        ScanMode;
    tANI_U16       MeasDuration;
} __ani_attr_packed tBcnRequest, *tpBcnRequest;

typedef __ani_attr_pre_packed struct sBcnRequestFields 
{
    tANI_U8        ChanNum;
    tANI_U8        ScanMode;
    tANI_U16       MeasDuration;
} __ani_attr_packed tBcnRequestFields, *tpBcnRequestFields;

typedef __ani_attr_pre_packed struct sCcxRadioMeasRequest
{
    tCcxIappHdr   IappHdr;
    tANI_U16      DiagToken;
    tANI_U8       MeasDly;
    tANI_U8       ActivationOffset;
} __ani_attr_packed tCcxRadioMeasRequest, *tpCcxRadioMeasRequest;

typedef __ani_attr_pre_packed struct sCcxRadioMeasReport
{
    tCcxIappHdr   IappHdr;
    tANI_U16      DiagToken;
} __ani_attr_packed tCcxRadioMeasReport, *tpCcxRadioMeasReport;

typedef __ani_attr_pre_packed struct sCcxNeighborListReport
{
    tCcxIappHdr   IappHdr;
} __ani_attr_packed tCcxNeighborListReport, *tpCcxNeighborListReport;

typedef __ani_attr_pre_packed struct sCcxAdjacentApReport
{
    tCcxIappHdr   IappHdr;
} __ani_attr_packed tCcxAdjacentApReport, *tpCcxAdjacentApReport;



typedef enum eCcxPackUnpackStatus
{
    eCCX_UNPACK_SUCCESS = 0,
    eCCX_PACK_SUCCESS = 0,
    eCCX_UNPACK_IE_ERR,
    eCCX_UNPACK_FRM_ERR,
    eCCX_PACK_FRM_ERR,
    eCCX_PACK_IE_ERR,
    eCCX_PACK_BUFF_OVERFLOW,
    eCCX_ERROR_UNKNOWN
} eCcxPackUnpackStatus;

typedef struct sCcxMeasReqIeInfo
{
    tpMeasRequestIe    pMeasReqIe;
    tpBcnRequestFields pBcnReqFields;

} tCcxMeasReqIeInfo, *tpCcxMeasReqIeInfo;

typedef struct sCcxNeighborIeInfo
{
    tpNeighborListIe       pNeighborIe;
    tpNeighborListRfSubIe  pRfSubIe;
    tpNeighborListTsfSubIe pTsfSubIe;
} tCcxNeighborIeInfo, *tpCcxNeighborIeInfo;

typedef struct sCcxUnpackIappFrm
{
    tpCcxRadioMeasRequest   pRadioMeasReqHdr;
    tCcxMeasReqIeInfo       MeasReqInfo[SIR_CCX_MAX_MEAS_IE_REQS];
    tpCcxNeighborListReport pNeighborListHdr;
    tCcxNeighborIeInfo      NeighborInfo[SIR_CCX_MAX_NEIGHBOR_IE_REPS];
} tCcxUnpackIappFrm, *tpCcxUnpackIappFrm;


typedef struct sCcxMeasRepIeInfo
{
    tpMeasReportIe    pMeasRepIe;
    tpBcnReportFields pBcnRepFields;
    tpTrafStrmMetrics pTrafStrmFields;
    tANI_U8           *pBuf;
} tCcxMeasRepIeInfo, *tpCcxMeasRepIeInfo;

typedef struct sCcxAdjApRepIeInfo
{
    tpAdjacentApRepIe pAdjApIe;
} tCcxAdjApRepIeInfo, *tpCcxAdjApRepIeInfo;

typedef struct sCcxAssocReasonIeInfo
{
    tpAssocReasonIe pAssocReasonIe;
} tCcxAssocReasonIeInfo, *tpCcxAssocReasonIeInfo;

typedef struct sCcxPackIappFrm
{
    tpCcxRadioMeasReport  pRadioMeasRepHdr;
    tCcxMeasRepIeInfo     MeasRepInfo[SIR_CCX_MAX_MEAS_IE_REPS];
    tpCcxAdjacentApReport pAdjApRepHdr;
    tCcxAdjApRepIeInfo    AdjApRepInfo;
    tCcxAssocReasonIeInfo AssocReasonInfo;
} tCcxPackIappFrm, *tpCcxPackIappFrm;


typedef struct sCcxMeasReq
{
    tANI_U8         isValid;
    tANI_U16        DiagToken;
    tANI_U8         MeasDly;
    tANI_U8         ActivationOffset;
    tANI_U8         numMeasReqIe;
    tAniBool        RepSent[SIR_CCX_MAX_MEAS_IE_REQS];
    tpMeasRequestIe pCurMeasReqIe[SIR_CCX_MAX_MEAS_IE_REQS];
} tCcxMeasReq, *tpCcxMeasReq;



#endif 
