#ifndef __7800_CMD_H__
#define __7800_CMD_H__

#include "serial.h"
#include "types.h"

enum E7800Status : u8
{
	EStatus_Running = 0,
	EStatus_Stopped = 1,
	EStatus_Menu = 2
};

enum E7800ExecuteExtraFlags : u16
{
	EExtra_HSC = 1,						// enable high score cart support
	EExtra_MEGA7800 = 2,				// native game Mega7800 support
	EExtra_SAVEKEY = 4,					// savekey or atarivox expected in port 2
	EExtra_COMPOSITE = 8				// enable strong blending
};

const COMPORT CmdInit(const char *pCommPort);
void CmdTerm(const COMPORT h);
bool CmdStatus(const COMPORT h, E7800Status *status);
bool CmdBreak(const COMPORT h);
bool CmdReturn(const COMPORT h);
bool CmdWriteCart(const COMPORT h, const u32 nAddr, const u32 nSize);
bool CmdWriteData(const COMPORT h, const void *pData, const u32 nSize);
bool CmdWriteDataComplete(const COMPORT h);
bool CmdExecute(const COMPORT h, const u8 nMapper, const u8 nMapperOptions, const u16 nMapperAudio, const u16 nMapperIRQEnable, const u32 nSize, const u16 nExtraFlags);

#endif // __7800_CMD_H__