#ifndef _RINGBUFFER_H
#define _RINGBUFFER_H
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

typedef struct
{
    long   bufferSize; /* Number of bytes in FIFO. Power of 2. Set by RingBuffer_Init. */
/* These are declared volatile because they are written by a different thread than the reader. */
    volatile long   writeIndex; /* Index of next writable byte. Set by RingBuffer_AdvanceWriteIndex. */
    volatile long   readIndex;  /* Index of next readable byte. Set by RingBuffer_AdvanceReadIndex. */
    long   bigMask;    /* Used for wrapping indices with extra bit to distinguish full/empty. */
    long   smallMask;  /* Used for fitting indices to buffer. */
    char *buffer;
}
RingBuffer;
/*
 * Initialize Ring Buffer.
 * numBytes must be power of 2, returns -1 if not.
 */
long RingBuffer_Init( RingBuffer *rbuf, long numBytes, void *dataPtr );

/* Clear buffer. Should only be called when buffer is NOT being read. */
void RingBuffer_Flush( RingBuffer *rbuf );

/* Return number of bytes available for writing. */
long RingBuffer_GetWriteAvailable( RingBuffer *rbuf );
/* Return number of bytes available for read. */
long RingBuffer_GetReadAvailable( RingBuffer *rbuf );
/* Return bytes written. */
long RingBuffer_Write( RingBuffer *rbuf, void *data, long numBytes );
/* Return bytes read. */
long RingBuffer_Read( RingBuffer *rbuf, void *data, long numBytes );

/* Get address of region(s) to which we can write data.
** If the region is contiguous, size2 will be zero.
** If non-contiguous, size2 will be the size of second region.
** Returns room available to be written or numBytes, whichever is smaller.
*/
long RingBuffer_GetWriteRegions( RingBuffer *rbuf, long numBytes,
                                 void **dataPtr1, long *sizePtr1,
                                 void **dataPtr2, long *sizePtr2 );
long RingBuffer_AdvanceWriteIndex( RingBuffer *rbuf, long numBytes );

/* Get address of region(s) from which we can read data.
** If the region is contiguous, size2 will be zero.
** If non-contiguous, size2 will be the size of second region.
** Returns room available to be read or numBytes, whichever is smaller.
*/
long RingBuffer_GetReadRegions( RingBuffer *rbuf, long numBytes,
                                void **dataPtr1, long *sizePtr1,
                                void **dataPtr2, long *sizePtr2 );

long RingBuffer_AdvanceReadIndex( RingBuffer *rbuf, long numBytes );

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _RINGBUFFER_H */
