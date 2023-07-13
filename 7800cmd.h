#ifndef __7800_CMD_H__
#define __7800_CMD_H__

#include "serial.h"
#include "types.h"

enum E7800Status : unsigned char
{
	EStatus_Running = 0,
	EStatus_Stopped = 1,
	EStatus_Menu = 2
};

const COMPORT CmdInit(const char *pCommPort);
void CmdTerm(const COMPORT h);
bool CmdStatus(const COMPORT h, E7800Status *status);
bool CmdBreak(const COMPORT h);
bool CmdReturn(const COMPORT h);
bool CmdWriteCart(const COMPORT h, const u32 nAddr, const u32 nSize);
bool CmdWriteData(const COMPORT h, const void *pData, const u32 nSize);
bool CmdWriteDataComplete(const COMPORT h);
bool CmdExecute(const COMPORT h, const u8 nMapper, const u8 nMapperOptions, const u16 nMapperAudio, const u16 nMapperIRQEnable, const u32 nSize);

#endif // __7800_CMD_H__