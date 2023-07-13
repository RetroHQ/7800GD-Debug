#ifndef __7800CMD_SERIAL__
#define __7800CMD_SERIAL__

#include <windows.h>
#include "types.h"

// windows specific types, redefine for different platforms
#define COMPORT				HANDLE
#define COMPORT_INVALID		INVALID_HANDLE_VALUE

const COMPORT ComOpen(const char *device, u32 baud_rate);
void ComClose(const COMPORT h);
bool ComWrite(const COMPORT h, const void *pData, const int nSize);
bool ComRead(const COMPORT h, void *pData, const int nSize);
bool ComBreak(const COMPORT h);

#endif // __7800CMD_SERIAL__