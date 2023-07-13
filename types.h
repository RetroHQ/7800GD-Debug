#ifndef __7800CMD_TYPES__
#define __7800CMD_TYPES__

#include <stdint.h>

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

#define COUNTOF(x)		(sizeof(x) / sizeof(x[0]))
#define PACKED			__attribute__((packed))
#define ALIGNED(x)		__declspec(align(x))

#endif // __7800CMD_TYPES__