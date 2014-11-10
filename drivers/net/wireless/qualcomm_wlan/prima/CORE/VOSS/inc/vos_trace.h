/*
 * Copyright (c) 2011-2014 The Linux Foundation. All rights reserved.
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


#if !defined( __VOS_TRACE_H )
#define __VOS_TRACE_H



#include  <vos_types.h>  
#include  <stdarg.h>       
#include  <vos_status.h>
#include  <i_vos_types.h>


typedef enum 
{
   
   
   VOS_TRACE_LEVEL_NONE = 0,
   
   
   
   
   VOS_TRACE_LEVEL_FATAL,
   VOS_TRACE_LEVEL_ERROR, 
   VOS_TRACE_LEVEL_WARN,  
   VOS_TRACE_LEVEL_INFO,
   VOS_TRACE_LEVEL_INFO_HIGH,
   VOS_TRACE_LEVEL_INFO_MED,
   VOS_TRACE_LEVEL_INFO_LOW,
   VOS_TRACE_LEVEL_DEBUG,

   
   
   VOS_TRACE_LEVEL_ALL, 

   
   
   
   VOS_TRACE_LEVEL_MAX   
    
} VOS_TRACE_LEVEL;

#define ASSERT_BUFFER_SIZE ( 512 )

#define VOS_ENABLE_TRACING 
#define MAX_VOS_TRACE_RECORDS 4000
#define INVALID_VOS_TRACE_ADDR 0xffffffff
#define DEFAULT_VOS_TRACE_DUMP_COUNT 0

#include  <i_vos_trace.h>   

#ifdef TRACE_RECORD

#define CASE_RETURN_STRING( str )           \
    case ( ( str ) ): return( (tANI_U8*)(#str) );

#define MTRACE(p) p
#define NO_SESSION 0xFF

#else
#define MTRACE(p) {  }

#ifndef printf
#define printf(fmt, args...)    printk(KERN_INFO "[WLAN] "fmt, ## args)
#endif

#ifndef errprintf
#define errprintf(fmt, args...) printk(KERN_WARNING "[WLAN][ERR] "fmt, ## args)
#endif

#ifndef wrnprintf
#define wrnprintf(fmt, args...) printk(KERN_WARNING "[WLAN][WRN] "fmt, ## args)
#endif

#define htc_printf(fmt, args...) printf(fmt, ## args)

#define HTC_KERNEL_FEEDBACK(x) errprintf x

#endif

typedef struct  svosTraceRecord
{
    v_U32_t time;
    v_U8_t module;
    v_U8_t code;
    v_U8_t session;
    v_U32_t data;
}tvosTraceRecord, *tpvosTraceRecord;

typedef struct svosTraceData
{
    
    
    
    v_U32_t head;
    v_U32_t tail;
    v_U32_t num;
    v_U16_t numSinceLastDump;

    
    v_U8_t enable;
    v_U16_t dumpCount; 

}tvosTraceData;




void vos_trace_setLevel( VOS_MODULE_ID module, VOS_TRACE_LEVEL level );

v_BOOL_t vos_trace_getLevel( VOS_MODULE_ID module, VOS_TRACE_LEVEL level );

typedef void (*tpvosTraceCb) (void *pMac, tpvosTraceRecord, v_U16_t);
void vos_trace(v_U8_t module, v_U8_t code, v_U8_t session, v_U32_t data);
void vosTraceRegister(VOS_MODULE_ID, tpvosTraceCb);
VOS_STATUS vos_trace_spin_lock_init(void);
void vosTraceInit(void);
void vosTraceEnable(v_U32_t, v_U8_t enable);
void vosTraceDumpAll(void*, v_U8_t, v_U8_t, v_U32_t, v_U32_t);
#endif
