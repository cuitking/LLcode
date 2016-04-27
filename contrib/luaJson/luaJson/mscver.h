#ifndef MSCVER_H
#define MSCVER_H

#if _MSC_VER
#define snprintf _snprintf
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

#if defined(_MSC_VER)
/* MSVC is stuck in the last century and doesn't have C99's stdint.h. */
typedef __int8 int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#ifdef _WIN64
typedef __int64 intptr_t;
typedef unsigned __int64 uintptr_t;
#else
typedef __int32 intptr_t;
typedef unsigned __int32 uintptr_t;
#endif
#endif

#endif //MSCVER_H