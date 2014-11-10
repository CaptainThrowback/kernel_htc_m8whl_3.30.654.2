/*
 * Copyright (c) 2012-2014 The Linux Foundation. All rights reserved.
 *
 * Previously licensed under the ISC license by Qualcomm Atheros, Inc.
 *
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/*
 * Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * Previously licensed under the ISC license by Qualcomm Atheros, Inc.
 *
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/**=========================================================================

  \file  vos_trace.c

  \brief virtual Operating System Servies (vOS)

   Trace, logging, and debugging definitions and APIs

   Copyright 2008,2011 (c) Qualcomm, Incorporated.  All Rights Reserved.

   Qualcomm Confidential and Proprietary.

  ========================================================================*/


#include <vos_trace.h>
#include <aniGlobal.h>
#include <wlan_logging_sock_svc.h>

#define VOS_TRACE_BUFFER_SIZE ( 512 )

#define VOS_TRACE_LEVEL_TO_MODULE_BITMASK( _level ) ( ( 1 << (_level) ) )

typedef struct
{
   
   
   
   
   
   
   
   
   v_U16_t moduleTraceLevel;

   
   unsigned char moduleNameStr[ 4 ];   

} moduleTraceInfo;

#define VOS_DEFAULT_TRACE_LEVEL \
   ((1<<VOS_TRACE_LEVEL_FATAL)|(1<<VOS_TRACE_LEVEL_ERROR))

moduleTraceInfo gVosTraceInfo[ VOS_MODULE_ID_MAX ] =
{
   [VOS_MODULE_ID_BAP]        = { VOS_DEFAULT_TRACE_LEVEL, "BAP" },
   [VOS_MODULE_ID_TL]         = { VOS_DEFAULT_TRACE_LEVEL, "TL " },
   [VOS_MODULE_ID_WDI]        = { VOS_DEFAULT_TRACE_LEVEL, "WDI" },
   [VOS_MODULE_ID_HDD]        = { VOS_DEFAULT_TRACE_LEVEL, "HDD" },
   [VOS_MODULE_ID_SME]        = { VOS_DEFAULT_TRACE_LEVEL, "SME" },
   [VOS_MODULE_ID_PE]         = { VOS_DEFAULT_TRACE_LEVEL, "PE " },
   [VOS_MODULE_ID_WDA]        = { VOS_DEFAULT_TRACE_LEVEL, "WDA" },
   [VOS_MODULE_ID_SYS]        = { VOS_DEFAULT_TRACE_LEVEL, "SYS" },
   [VOS_MODULE_ID_VOSS]       = { VOS_DEFAULT_TRACE_LEVEL, "VOS" },
   [VOS_MODULE_ID_SAP]        = { VOS_DEFAULT_TRACE_LEVEL, "SAP" },
   [VOS_MODULE_ID_HDD_SOFTAP] = { VOS_DEFAULT_TRACE_LEVEL, "HSP" },
   [VOS_MODULE_ID_PMC]        = { VOS_DEFAULT_TRACE_LEVEL, "PMC" },
   [VOS_MODULE_ID_HDD_DATA]   = { VOS_DEFAULT_TRACE_LEVEL, "HDP" },
   [VOS_MODULE_ID_HDD_SAP_DATA] = { VOS_DEFAULT_TRACE_LEVEL, "SDP" },
};
static spinlock_t ltraceLock;

static tvosTraceRecord gvosTraceTbl[MAX_VOS_TRACE_RECORDS];
static tvosTraceData gvosTraceData;
static tpvosTraceCb vostraceCBTable[VOS_MODULE_ID_MAX];
static tpvosTraceCb vostraceRestoreCBTable[VOS_MODULE_ID_MAX];
void vos_trace_setLevel( VOS_MODULE_ID module, VOS_TRACE_LEVEL level )
{
   
   if ( level >= VOS_TRACE_LEVEL_MAX )
   {
      printk(KERN_WARNING "[WLAN][WRN] %s: Invalid trace level %d passed in!\n", __func__, level);
      return;
   }

   
   
   
   if ( VOS_TRACE_LEVEL_NONE == level )
   {
      gVosTraceInfo[ module ].moduleTraceLevel = VOS_TRACE_LEVEL_NONE;
   }
   else
   {
      
      gVosTraceInfo[ module ].moduleTraceLevel |= VOS_TRACE_LEVEL_TO_MODULE_BITMASK( level );
   }
}

void vos_trace_setValue( VOS_MODULE_ID module, VOS_TRACE_LEVEL level, v_U8_t on)
{
   
   if ( level < 0  || level >= VOS_TRACE_LEVEL_MAX )
   {
      printk(KERN_WARNING "[WLAN][WRN] %s: Invalid trace level %d passed in!\n", __func__, level);
      return;
   }

   
   if ( module < 0 || module >= VOS_MODULE_ID_MAX )
   {
      printk(KERN_WARNING "[WLAN][WRN] %s: Invalid module id %d passed in!\n", __func__, module);
      return;
   }

   
   
   if ( VOS_TRACE_LEVEL_NONE == level )
   {
      gVosTraceInfo[ module ].moduleTraceLevel = VOS_TRACE_LEVEL_NONE;
   }
   
   
   else if ( VOS_TRACE_LEVEL_ALL == level )
   {
      gVosTraceInfo[ module ].moduleTraceLevel = 0xFFFF;
   }

   else
   {
      if (on)
         
         gVosTraceInfo[ module ].moduleTraceLevel |= VOS_TRACE_LEVEL_TO_MODULE_BITMASK( level );
      else
         
         gVosTraceInfo[ module ].moduleTraceLevel &= ~(VOS_TRACE_LEVEL_TO_MODULE_BITMASK( level ));
   }
}


v_BOOL_t vos_trace_getLevel( VOS_MODULE_ID module, VOS_TRACE_LEVEL level )
{
   v_BOOL_t traceOn = VOS_FALSE;

   if ( ( VOS_TRACE_LEVEL_NONE == level ) ||
        ( VOS_TRACE_LEVEL_ALL  == level ) ||
        ( level >= VOS_TRACE_LEVEL_MAX  )    )
   {
      traceOn = VOS_FALSE;
   }
   else
   {
      traceOn = ( level & gVosTraceInfo[ module ].moduleTraceLevel ) ? VOS_TRUE : VOS_FALSE;
   }

   return( traceOn );
}

void vos_snprintf(char *strBuffer, unsigned  int size, char *strFormat, ...)
{
    va_list val;

    va_start( val, strFormat );
    snprintf (strBuffer, size, strFormat, val);
    va_end( val );
}

#ifdef VOS_ENABLE_TRACING

void vos_trace_msg( VOS_MODULE_ID module, VOS_TRACE_LEVEL level, char *strFormat, ... )
{
   char strBuffer[VOS_TRACE_BUFFER_SIZE];
   int n;

   
   
   if ( gVosTraceInfo[ module ].moduleTraceLevel & VOS_TRACE_LEVEL_TO_MODULE_BITMASK( level ) )
   {
      
      
      
      
      
      static const char * TRACE_LEVEL_STR[] = { "  ", "F ", "E ", "W ", "I ", "IH", "IM", "IL", "D" };
      va_list val;
      va_start(val, strFormat);

      
      n = snprintf(strBuffer, VOS_TRACE_BUFFER_SIZE, "wlan: [%d:%2s:%3s] ",
                   in_interrupt() ? 0 : current->pid,
                   (char *) TRACE_LEVEL_STR[ level ],
                   (char *) gVosTraceInfo[ module ].moduleNameStr );

      
      if ((n >= 0) && (n < VOS_TRACE_BUFFER_SIZE))
      {
         vsnprintf(strBuffer + n, VOS_TRACE_BUFFER_SIZE - n, strFormat, val );

#ifdef WLAN_LOGGING_SOCK_SVC_ENABLE
         wlan_log_to_user(level, (char *)strBuffer, strlen(strBuffer));
#else
         printk(KERN_WARNING "[WLAN][WRN] %s\n", strBuffer);
#endif
      }
      va_end(val);
   }
}

void vos_trace_display(void)
{
   VOS_MODULE_ID moduleId;

   printk(KERN_WARNING "[WLAN][WRN]      1)FATAL  2)ERROR  3)WARN  4)INFO  5)INFO_H  6)INFO_M  7)INFO_L 8)DEBUG\n");
   for (moduleId = 0; moduleId < VOS_MODULE_ID_MAX; ++moduleId)
   {
      printk(KERN_WARNING "[WLAN][WRN] %2d)%s    %s        %s       %s       %s        %s         %s         %s        %s\n",
             (int)moduleId,
             gVosTraceInfo[moduleId].moduleNameStr,
             (gVosTraceInfo[moduleId].moduleTraceLevel & (1 << VOS_TRACE_LEVEL_FATAL)) ? "X":" ",
             (gVosTraceInfo[moduleId].moduleTraceLevel & (1 << VOS_TRACE_LEVEL_ERROR)) ? "X":" ",
             (gVosTraceInfo[moduleId].moduleTraceLevel & (1 << VOS_TRACE_LEVEL_WARN)) ? "X":" ",
             (gVosTraceInfo[moduleId].moduleTraceLevel & (1 << VOS_TRACE_LEVEL_INFO)) ? "X":" ",
             (gVosTraceInfo[moduleId].moduleTraceLevel & (1 << VOS_TRACE_LEVEL_INFO_HIGH)) ? "X":" ",
             (gVosTraceInfo[moduleId].moduleTraceLevel & (1 << VOS_TRACE_LEVEL_INFO_MED)) ? "X":" ",
             (gVosTraceInfo[moduleId].moduleTraceLevel & (1 << VOS_TRACE_LEVEL_INFO_LOW)) ? "X":" ",
             (gVosTraceInfo[moduleId].moduleTraceLevel & (1 << VOS_TRACE_LEVEL_DEBUG)) ? "X":" "
         );
   }
}

void vos_trace_hex_dump( VOS_MODULE_ID module, VOS_TRACE_LEVEL level,
                                void *data, int buf_len )
{
    char *buf = (char *)data;
    int i;
    for (i=0; (i+7)<buf_len; i+=8)
    {
        vos_trace_msg( module, level,
                 "%02x %02x %02x %02x %02x %02x %02x %02x \n",
                 buf[i],
                 buf[i+1],
                 buf[i+2],
                 buf[i+3],
                 buf[i+4],
                 buf[i+5],
                 buf[i+6],
                 buf[i+7]);
    }

    
    for (; i < buf_len; i++)
    {
        vos_trace_msg( module, level, "%02x ", buf[i]);
        if ((i+1) == buf_len)
            vos_trace_msg( module, level, "\n");
    }

}

#endif

void vosTraceEnable(v_U32_t bitmask_of_moduleId, v_U8_t enable)
{
    int i;
    if (bitmask_of_moduleId)
    {
       for (i=0; i<VOS_MODULE_ID_MAX; i++)
       {
           if (((bitmask_of_moduleId >> i) & 1 ))
           {
             if(enable)
             {
                if (NULL != vostraceRestoreCBTable[i])
                {
                   vostraceCBTable[i] = vostraceRestoreCBTable[i];
                }
             }
             else
             {
                vostraceRestoreCBTable[i] = vostraceCBTable[i];
                vostraceCBTable[i] = NULL;
             }
           }
       }
    }

    else
    {
      if(enable)
      {
         for (i=0; i<VOS_MODULE_ID_MAX; i++)
         {
             if (NULL != vostraceRestoreCBTable[i])
             {
                vostraceCBTable[i] = vostraceRestoreCBTable[i];
             }
         }
      }
      else
      {
         for (i=0; i<VOS_MODULE_ID_MAX; i++)
         {
            vostraceRestoreCBTable[i] = vostraceCBTable[i];
            vostraceCBTable[i] = NULL;
         }
      }
    }
}

void vosTraceInit()
{
    v_U8_t i;
    gvosTraceData.head = INVALID_VOS_TRACE_ADDR;
    gvosTraceData.tail = INVALID_VOS_TRACE_ADDR;
    gvosTraceData.num = 0;
    gvosTraceData.enable = TRUE;
    gvosTraceData.dumpCount = DEFAULT_VOS_TRACE_DUMP_COUNT;
    gvosTraceData.numSinceLastDump = 0;

    for (i=0; i<VOS_MODULE_ID_MAX; i++)
    {
        vostraceCBTable[i] = NULL;
        vostraceRestoreCBTable[i] = NULL;
    }
}

void vos_trace(v_U8_t module, v_U8_t code, v_U8_t session, v_U32_t data)
{
    tpvosTraceRecord rec = NULL;
    unsigned long flags;


    if (!gvosTraceData.enable)
    {
        return;
    }
    
    if (NULL == vostraceCBTable[module])
    {
        return;
    }

    
    spin_lock_irqsave(&ltraceLock, flags);

    gvosTraceData.num++;

    if (gvosTraceData.num > MAX_VOS_TRACE_RECORDS)
    {
        gvosTraceData.num = MAX_VOS_TRACE_RECORDS;
    }

    if (INVALID_VOS_TRACE_ADDR == gvosTraceData.head)
    {
        
        gvosTraceData.head = 0;
        gvosTraceData.tail = 0;
    }
    else
    {
        
        v_U32_t tail = gvosTraceData.tail + 1;

        if (MAX_VOS_TRACE_RECORDS == tail)
        {
            tail = 0;
        }

        if (gvosTraceData.head == tail)
        {
            
            if (MAX_VOS_TRACE_RECORDS == ++gvosTraceData.head)
            {
                gvosTraceData.head = 0;
            }
        }

        gvosTraceData.tail = tail;
    }

    rec = &gvosTraceTbl[gvosTraceData.tail];
    rec->code = code;
    rec->session = session;
    rec->data = data;
    rec->time = vos_timer_get_system_time();
    rec->module =  module;
    gvosTraceData.numSinceLastDump ++;
    spin_unlock_irqrestore(&ltraceLock, flags);
}


VOS_STATUS vos_trace_spin_lock_init()
{
    spin_lock_init(&ltraceLock);

    return VOS_STATUS_SUCCESS;
}

void vosTraceRegister(VOS_MODULE_ID moduleID, tpvosTraceCb vostraceCb)
{
    vostraceCBTable[moduleID] = vostraceCb;
}

void vosTraceDumpAll(void *pMac, v_U8_t code, v_U8_t session,
                     v_U32_t count, v_U32_t bitmask_of_module)
{
    tvosTraceRecord pRecord;
    tANI_S32 i, tail;


    if (!gvosTraceData.enable)
    {
        VOS_TRACE( VOS_MODULE_ID_SYS,
                   VOS_TRACE_LEVEL_ERROR, "Tracing Disabled");
        return;
    }

    VOS_TRACE( VOS_MODULE_ID_SYS, VOS_TRACE_LEVEL_ERROR,
               "Total Records: %d, Head: %d, Tail: %d",
               gvosTraceData.num, gvosTraceData.head, gvosTraceData.tail);

    
    spin_lock(&ltraceLock);

    if (gvosTraceData.head != INVALID_VOS_TRACE_ADDR)
    {
        i = gvosTraceData.head;
        tail = gvosTraceData.tail;

        if (count)
        {
            if (count > gvosTraceData.num)
            {
                count = gvosTraceData.num;
            }
            if (tail >= (count - 1))
            {
                i = tail - count + 1;
            }
            else if (count != MAX_VOS_TRACE_RECORDS)
            {
                i = MAX_VOS_TRACE_RECORDS - ((count - 1) - tail);
            }
        }

        pRecord = gvosTraceTbl[i];
        gvosTraceData.numSinceLastDump = 0;
        spin_unlock(&ltraceLock);
        for (;;)
        {
            if ((code == 0 || (code == pRecord.code)) &&
                    (vostraceCBTable[pRecord.module] != NULL))
            {
                if (0 == bitmask_of_module)
                {
                   vostraceCBTable[pRecord.module](pMac, &pRecord, (v_U16_t)i);
                }
                else
                {
                   if (bitmask_of_module & (1 << pRecord.module))
                   {
                      vostraceCBTable[pRecord.module](pMac, &pRecord, (v_U16_t)i);
                   }
                }
            }

            if (i == tail)
            {
                break;
            }
            i += 1;

            spin_lock(&ltraceLock);
            if (MAX_VOS_TRACE_RECORDS == i)
            {
                i = 0;
                pRecord= gvosTraceTbl[0];
            }
            else
            {
                pRecord = gvosTraceTbl[i];
            }
            spin_unlock(&ltraceLock);
        }
    }
    else
    {
        spin_unlock(&ltraceLock);
    }
}
