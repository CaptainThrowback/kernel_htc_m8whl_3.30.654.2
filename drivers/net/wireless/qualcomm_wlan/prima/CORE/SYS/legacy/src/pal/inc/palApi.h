/*
 * Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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

/** ------------------------------------------------------------------------- *
    ------------------------------------------------------------------------- *


    \file palApi.h

    \brief Exports and types for the Platform Abstraction Layer interfaces.

    $Id$

    Copyright (C) 2006 Airgo Networks, Incorporated
    This file contains all the interfaces for thge Platform Abstration Layer
    functions.  It is intended to be included in all modules that are using
    the PAL interfaces.

   ========================================================================== */
#ifndef PALAPI_H__
#define PALAPI_H__

#include "halTypes.h"



/** ---------------------------------------------------------------------------

    \fn palReadRegister

    \brief chip and bus agnostic funtion to read a register value

    \param hHdd - HDD context handle

    \param regAddress - address (offset) of the register to be read from the start
    of register space.

    \param pRegValue - pointer to the memory where the register contents are written

    \return eHalStatus - status of the register read.  Note that this function
    can fail.  In particular, when the card is removed, this function will return
    a failure.

  -------------------------------------------------------------------------------*/
eHalStatus palReadRegister( tHddHandle hHdd, tANI_U32 regAddress, tANI_U32 *pRegValue );


/** ---------------------------------------------------------------------------

    \fn palWriteRegister

    \brief chip and bus agnostic funtion to write a register value

    \param hHdd - HDD context handle

    \param regAddress - address (offset) of the register to be read from the start
    of register space.

    \param pRegValue - pointer to the value being written into the register

    \return eHalStatus - status of the register read.  Note that this function
    can fail.  In particular, when the card is removed, this function will return
    a failure.

  -------------------------------------------------------------------------------*/
eHalStatus palWriteRegister( tHddHandle hHdd, tANI_U32 regAddress, tANI_U32 regValue );

/** ---------------------------------------------------------------------------

    \fn palAsyncWriteRegister

    \brief chip and bus agnostic async funtion to write a register value

    \param hHdd - HDD context handle

    \param regAddress - address (offset) of the register to be written from the start
    of register space.

    \param regValue - value being written into the register

    \return eHalStatus - status of the register write.  Note that this function
    can fail.  In particular, when the card is removed, this function will return
    a failure.

  -------------------------------------------------------------------------------*/

eHalStatus palAsyncWriteRegister( tHddHandle hHdd, tANI_U32 regAddress, tANI_U32 regValue );


eHalStatus palReadDeviceMemory( tHddHandle hHdd, tANI_U32 memOffset, tANI_U8 *pBuffer, tANI_U32 numBytes );

/** ---------------------------------------------------------------------------

    \fn palWriteDeviceMemory

    \brief chip and bus agnostic funtion to write memory to the chip

    \param hHdd - HDD context handle

    \param memOffset - address (offset) of the memory from the top of the on-chip
    memory that will be written.

    \param pBuffer - pointer to a buffer that has the source data that will be
    written to the chip.

    \param numBytes - the number of bytes to be written.

    \return eHalStatus - status of the memory read.  Note that this function
    can fail.  In particular, when the card is removed, this function will return
    a failure.

  -------------------------------------------------------------------------------*/
eHalStatus palWriteDeviceMemory( tHddHandle hHdd, tANI_U32 memOffset, tANI_U8 *pBuffer, tANI_U32 numBytes );


#ifndef FEATURE_WLAN_PAL_MEM_DISABLE

#ifdef MEMORY_DEBUG
#define palAllocateMemory(hHdd, ppMemory, numBytes) palAllocateMemory_debug(hHdd, ppMemory, numBytes, __FILE__, __LINE__)
eHalStatus palAllocateMemory_debug( tHddHandle hHdd, void **ppMemory, tANI_U32 numBytes, char* fileName, tANI_U32 lineNum );
#else
eHalStatus palAllocateMemory( tHddHandle hHdd, void **ppMemory, tANI_U32 numBytes );
#endif


eHalStatus palFreeMemory( tHddHandle hHdd, void *pMemory );



/** ---------------------------------------------------------------------------

    \fn palFillMemory

    \brief OS agnostic funtion to fill host memory with a specified byte value

    \param hHdd - HDD context handle

    \param pMemory - pointer to memory that will be filled.

    \param numBytes - the number of bytes to be filled.

    \param fillValue - the byte to be written to fill the memory with.

    \return eHalStatus - status of the register read.  Note that this function
    can fail.  In the case of a failure, a non-successful return code will be
    returned and no memory will be allocated (the *ppMemory will be NULL so don't
    try to use it unless the status returns success).

  -------------------------------------------------------------------------------*/
eHalStatus palFillMemory( tHddHandle hHdd, void *pMemory, tANI_U32 numBytes, tANI_BYTE fillValue );

eHalStatus palCopyMemory( tHddHandle hHdd, void *pDst, const void *pSrc, tANI_U32 numBytes );

/** ---------------------------------------------------------------------------

    \fn palFillMemory

    \brief OS agnostic funtion to fill host memory with a specified byte value

    \param hHdd - HDD context handle

    \param pMemory - pointer to memory that will be filled.

    \param numBytes - the number of bytes to be filled.

    \param fillValue - the byte to be written to fill the memory with.

    \return eHalStatus - status of the register read.  Note that this function
    can fail.  In the case of a failure, a non-successful return code will be
    returned and no memory will be allocated (the *ppMemory will be NULL so don't
    try to use it unless the status returns success).

  -------------------------------------------------------------------------------*/
ANI_INLINE_FUNCTION
eHalStatus palZeroMemory( tHddHandle hHdd, void *pMemory, tANI_U32 numBytes )
{
    return( palFillMemory( hHdd, pMemory, numBytes, 0 ) );
}


tANI_BOOLEAN palEqualMemory( tHddHandle hHdd, void *pMemory1, void *pMemory2, tANI_U32 numBytes );
#endif
eHalStatus palFillDeviceMemory( tHddHandle hHdd, tANI_U32 memOffset, tANI_U32 numBytes, tANI_BYTE fillValue );


ANI_INLINE_FUNCTION
eHalStatus palZeroDeviceMemory( tHddHandle hHdd, tANI_U32 memOffset, tANI_U32 numBytes )
{
    return( palFillDeviceMemory( hHdd, memOffset, numBytes, 0 ) );
}

eHalStatus palPktAlloc(tHddHandle hHdd, eFrameType frmType, tANI_U16 size, void **data, void **ppPacket) ;


void palPktFree( tHddHandle hHdd, eFrameType frmType, void* buf, void *pPacket);



eHalStatus palSpinLockAlloc( tHddHandle hHdd, tPalSpinLockHandle *pHandle );
eHalStatus palSpinLockFree( tHddHandle hHdd, tPalSpinLockHandle hSpinLock );
eHalStatus palSpinLockTake( tHddHandle hHdd, tPalSpinLockHandle hSpinLock );
eHalStatus palSpinLockGive( tHddHandle hHdd, tPalSpinLockHandle hSpinLock );


eHalStatus palSendMBMessage(tHddHandle hHdd, void *pBuf);

extern void palGetUnicastStats(tHddHandle hHdd, tANI_U32 *tx, tANI_U32 *rx);


tANI_U32 palGetTickCount(tHddHandle hHdd);

eHalStatus palReadRegMemory( tHddHandle hHdd, tANI_U32 memOffset, tANI_U8 *pBuffer, tANI_U32 numBytes );

/** ---------------------------------------------------------------------------

    \fn palAsyncWriteRegMemory

    \brief chip and bus agnostic function to write memory to the PHY register space as memory
    i.e. to write more than 4 bytes from the contiguous register space. In USB interface, this
    API does the write asynchronously.

    \param hHdd - HDD context handle

    \param memOffset - address (offset) of the memory from the top of the on-chip
    memory that will be written.

    \param pBuffer - pointer to a buffer that has the source data that will be
    written to the chip.

    \param numBytes - the number of bytes to be written.

    \return eHalStatus - status of the memory read.  Note that this function
    can fail.  In particular, when the card is removed, this function will return
    a failure.

  -------------------------------------------------------------------------------*/
eHalStatus palAsyncWriteRegMemory( tHddHandle hHdd, tANI_U32 memOffset, tANI_U8 *pBuffer, tANI_U32 numBytes );

/** ---------------------------------------------------------------------------

    \fn palWriteRegMemory
    \brief chip and bus agnostic function to write memory to the PHY register space as memory
    i.e. to write more than 4 bytes from the contiguous register space. The difference from the
    above routine is, in USB interface, this routine performs the write synchronously where as
    the above routine performs it asynchronously.

    \param hHdd - HDD context handle

    \param memOffset - address (offset) of the memory from the top of the on-chip
    memory that will be written.

    \param pBuffer - pointer to a buffer that has the source data that will be
    written to the chip.

    \param numBytes - the number of bytes to be written.

    \return eHalStatus - status of the memory read.  Note that this function
    can fail.  In particular, when the card is removed, this function will return
    a failure.

  -------------------------------------------------------------------------------*/
eHalStatus palWriteRegMemory( tHddHandle hHdd, tANI_U32 memOffset, tANI_U8 *pBuffer, tANI_U32 numBytes );


eHalStatus palWaitRegVal( tHddHandle hHdd, tANI_U32 reg, tANI_U32 mask,
                             tANI_U32 waitRegVal, tANI_U32 perIterWaitInNanoSec,
                             tANI_U32 numIter, tANI_U32 *pReadRegVal );

eHalStatus palReadModifyWriteReg( tHddHandle hHdd, tANI_U32 reg, tANI_U32 andMask, tANI_U32 orMask );

eHalStatus palSemaphoreAlloc( tHddHandle hHdd, tPalSemaphoreHandle *pHandle, tANI_S32 count );
eHalStatus palSemaphoreFree( tHddHandle hHdd, tPalSemaphoreHandle hSemaphore );
eHalStatus palSemaphoreTake( tHddHandle hHdd, tPalSemaphoreHandle hSemaphore );
eHalStatus palSemaphoreGive( tHddHandle hHdd, tPalSemaphoreHandle hSemaphore );
eHalStatus palMutexAlloc( tHddHandle hHdd, tPalSemaphoreHandle *pHandle) ;
eHalStatus palMutexAllocLocked( tHddHandle hHdd, tPalSemaphoreHandle *pHandle) ;

eAniBoolean pal_in_interrupt(void) ;
void pal_local_bh_disable(void) ;
void pal_local_bh_enable(void) ;

tANI_U32 pal_be32_to_cpu(tANI_U32 x) ;
tANI_U32 pal_cpu_to_be32(tANI_U32 x) ;
tANI_U16 pal_be16_to_cpu(tANI_U16 x) ;
tANI_U16 pal_cpu_to_be16(tANI_U16 x) ;


#if defined( ANI_LITTLE_BYTE_ENDIAN )

ANI_INLINE_FUNCTION unsigned long i_htonl( unsigned long ul )
{
  return( ( ( ul & 0x000000ff ) << 24 ) |
          ( ( ul & 0x0000ff00 ) <<  8 ) |
          ( ( ul & 0x00ff0000 ) >>  8 ) |
          ( ( ul & 0xff000000 ) >> 24 )   );
}

ANI_INLINE_FUNCTION unsigned short i_htons( unsigned short us )
{
  return( ( ( us >> 8 ) & 0x00ff ) + ( ( us << 8 ) & 0xff00 ) );
}

ANI_INLINE_FUNCTION unsigned short i_ntohs( unsigned short us )
{
  return( i_htons( us ) );
}

ANI_INLINE_FUNCTION unsigned long i_ntohl( unsigned long ul )
{
  return( i_htonl( ul ) );
}

#endif 


ANI_INLINE_FUNCTION tANI_U8 * pal_set_U32(tANI_U8 *ptr, tANI_U32 value)
{
#if defined( ANI_BIG_BYTE_ENDIAN )
     *(ptr) = ( tANI_U8 )( value >> 24 );
     *(ptr + 1) = ( tANI_U8 )( value >> 16 );
     *(ptr + 2) = ( tANI_U8 )( value >> 8 );
     *(ptr + 3) = ( tANI_U8 )( value );
#else
    *(ptr + 3) = ( tANI_U8 )( value >> 24 );
    *(ptr + 2) = ( tANI_U8 )( value >> 16 );
    *(ptr + 1) = ( tANI_U8 )( value >> 8 );
    *(ptr) = ( tANI_U8 )( value );
#endif

    return (ptr + 4);
}


ANI_INLINE_FUNCTION tANI_U8 * pal_set_U16(tANI_U8 *ptr, tANI_U16 value)
{
#if defined( ANI_BIG_BYTE_ENDIAN )
     *(ptr) = ( tANI_U8 )( value >> 8 );
     *(ptr + 1) = ( tANI_U8 )( value );
#else
    *(ptr + 1) = ( tANI_U8 )( value >> 8 );
    *(ptr) = ( tANI_U8 )( value );
#endif

    return (ptr + 2);
}


ANI_INLINE_FUNCTION tANI_U8 * pal_get_U16(tANI_U8 *ptr, tANI_U16 *pValue)
{
#if defined( ANI_BIG_BYTE_ENDIAN )
    *pValue = (((tANI_U16) (*ptr << 8)) |
            ((tANI_U16) (*(ptr+1))));
#else
    *pValue = (((tANI_U16) (*(ptr+1) << 8)) |
            ((tANI_U16) (*ptr)));
#endif

    return (ptr + 2);
}


ANI_INLINE_FUNCTION tANI_U8 * pal_get_U32(tANI_U8 *ptr, tANI_U32 *pValue)
{
#if defined( ANI_BIG_BYTE_ENDIAN )
    *pValue = ( (tANI_U32)(*(ptr) << 24) |
             (tANI_U32)(*(ptr+1) << 16) |
             (tANI_U32)(*(ptr+2) << 8) |
             (tANI_U32)(*(ptr+3)) );
#else
    *pValue = ( (tANI_U32)(*(ptr+3) << 24) |
             (tANI_U32)(*(ptr+2) << 16) |
             (tANI_U32)(*(ptr+1) << 8) |
             (tANI_U32)(*(ptr)) );
#endif

    return (ptr + 4);
}


#endif
