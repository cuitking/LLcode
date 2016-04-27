
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ringbuffer.h"
#include <string.h>
#ifdef _DEBUG
#include "Win32.h"
#endif // _DEBUG

/***************************************************************************
 * Initialize FIFO.
 * numBytes must be power of 2, returns -1 if not.
 */
long RingBuffer_Init( RingBuffer *rbuf, long numBytes, void *dataPtr )
{
	if (rbuf == NULL) return -1;
    if( ((numBytes-1) & numBytes) != 0) return -1; /* Not Power of two. */
    rbuf->bufferSize = numBytes;
    rbuf->buffer = (char *)dataPtr;
    RingBuffer_Flush( rbuf );
    rbuf->bigMask = (numBytes*2)-1;
    rbuf->smallMask = (numBytes)-1;
    return 0;
}
/***************************************************************************
** Return number of bytes available for reading. */
long RingBuffer_GetReadAvailable( RingBuffer *rbuf )
{
	if (rbuf == NULL) return 0;
    return ( (rbuf->writeIndex - rbuf->readIndex) & rbuf->bigMask );
}
/***************************************************************************
** Return number of bytes available for writing. */
long RingBuffer_GetWriteAvailable( RingBuffer *rbuf )
{
	if (rbuf == NULL) return 0;
    return ( rbuf->bufferSize - RingBuffer_GetReadAvailable(rbuf));
}

/***************************************************************************
** Clear buffer. Should only be called when buffer is NOT being read. */
void RingBuffer_Flush( RingBuffer *rbuf )
{
	if (rbuf == NULL) return;
#ifdef _DEBUG
	SwitchToThread();
#endif
    rbuf->writeIndex = rbuf->readIndex = 0;
#ifdef _DEBUG
	SwitchToThread();
#endif
}

/***************************************************************************
** Get address of region(s) to which we can write data.
** If the region is contiguous, size2 will be zero.
** If non-contiguous, size2 will be the size of second region.
** Returns room available to be written or numBytes, whichever is smaller.
*/
long RingBuffer_GetWriteRegions( RingBuffer *rbuf, long numBytes,
                                 void **dataPtr1, long *sizePtr1,
                                 void **dataPtr2, long *sizePtr2 )
{
	long   index;
	long   available;
	if (rbuf == NULL) return 0;
    available = RingBuffer_GetWriteAvailable( rbuf );
    if( numBytes > available ) numBytes = available;
    /* Check to see if write is not contiguous. */
    index = rbuf->writeIndex & rbuf->smallMask;
    if( (index + numBytes) > rbuf->bufferSize )
    {
        /* Write data in two blocks that wrap the buffer. */
        long   firstHalf = rbuf->bufferSize - index;
        *dataPtr1 = &rbuf->buffer[index];
        *sizePtr1 = firstHalf;
        *dataPtr2 = &rbuf->buffer[0];
        *sizePtr2 = numBytes - firstHalf;
    }
    else
    {
        *dataPtr1 = &rbuf->buffer[index];
        *sizePtr1 = numBytes;
        *dataPtr2 = NULL;
        *sizePtr2 = 0;
    }
    return numBytes;
}


/***************************************************************************
*/
long RingBuffer_AdvanceWriteIndex( RingBuffer *rbuf, long numBytes )
{
	if (rbuf == NULL) return 0;
#ifdef _DEBUG
	SwitchToThread();
#endif
    rbuf->writeIndex = (rbuf->writeIndex + numBytes) & rbuf->bigMask;
#ifdef _DEBUG
	SwitchToThread();
#endif
	return rbuf->writeIndex;
}

/***************************************************************************
** Get address of region(s) from which we can read data.
** If the region is contiguous, size2 will be zero.
** If non-contiguous, size2 will be the size of second region.
** Returns room available to be written or numBytes, whichever is smaller.
*/
long RingBuffer_GetReadRegions( RingBuffer *rbuf, long numBytes,
                                void **dataPtr1, long *sizePtr1,
                                void **dataPtr2, long *sizePtr2 )
{
	long   index;
	long   available;
	if (rbuf == NULL) return 0;
    available = RingBuffer_GetReadAvailable( rbuf );
    if( numBytes > available ) numBytes = available;
    /* Check to see if read is not contiguous. */
    index = rbuf->readIndex & rbuf->smallMask;
    if( (index + numBytes) > rbuf->bufferSize )
    {
        /* Write data in two blocks that wrap the buffer. */
        long firstHalf = rbuf->bufferSize - index;
        *dataPtr1 = &rbuf->buffer[index];
        *sizePtr1 = firstHalf;
        *dataPtr2 = &rbuf->buffer[0];
        *sizePtr2 = numBytes - firstHalf;
    }
    else
    {
        *dataPtr1 = &rbuf->buffer[index];
        *sizePtr1 = numBytes;
        *dataPtr2 = NULL;
        *sizePtr2 = 0;
    }
    return numBytes;
}
/***************************************************************************
*/
long RingBuffer_AdvanceReadIndex( RingBuffer *rbuf, long numBytes )
{
	if (rbuf == NULL) return 0;
#ifdef _DEBUG
	SwitchToThread();
#endif
    rbuf->readIndex = (rbuf->readIndex + numBytes) & rbuf->bigMask;
#ifdef _DEBUG
	SwitchToThread();
#endif
	return rbuf->readIndex;
}

/***************************************************************************
** Return bytes written. */
long RingBuffer_Write( RingBuffer *rbuf, void *data, long numBytes )
{
    long size1, size2, numWritten;
    void *data1, *data2;
	if (rbuf == NULL) return 0;
    numWritten = RingBuffer_GetWriteRegions( rbuf, numBytes, &data1, &size1, &data2, &size2 );
    if( size2 > 0 )
    {
        memcpy( data1, data, size1 );
        data = ((char *)data) + size1;
        memcpy( data2, data, size2 );
    }
    else
    {
        memcpy( data1, data, size1 );
    }
    RingBuffer_AdvanceWriteIndex( rbuf, numWritten );
    return numWritten;
}

/***************************************************************************
** Return bytes read. */
long RingBuffer_Read( RingBuffer *rbuf, void *data, long numBytes )
{
    long size1, size2, numRead;
    void *data1, *data2;
	if (rbuf == NULL) return 0;
    numRead = RingBuffer_GetReadRegions( rbuf, numBytes, &data1, &size1, &data2, &size2 );
    if( size2 > 0 )
    {
        memcpy( data, data1, size1 );
        data = ((char *)data) + size1;
        memcpy( data, data2, size2 );
    }
    else
    {
        memcpy( data, data1, size1 );
    }
    RingBuffer_AdvanceReadIndex( rbuf, numRead );
    return numRead;
}
