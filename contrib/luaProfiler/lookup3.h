#ifndef _LOOKUP_3_H
#define _LOOKUP_3_H

#include "stdint.h"     /* defines uint32_t etc */
#include <stddef.h>

uint32_t hashlittle(const void* key, size_t length, uint32_t initval);

uint32_t hashword(
	const uint32_t* k,                   /* the key, an array of uint32_t values */
	size_t          length,               /* the length of the key, in uint32_ts */
	uint32_t        initval);         /* the previous hash, or an arbitrary value */

#endif // _LOOKUP_3_H