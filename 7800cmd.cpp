#include "serial.h"
#include "7800cmd.h"

enum E7800Cmd
{
	ECmd_Status = 0,
	ECmd_Break,
	ECmd_Return,
	ECmd_WriteCart,
	ECmd_Execute
};

struct SCmd7800WriteParam
{
	u32 addr;
	u32 size;
};

// Start command and send command byte

static inline bool CmdSend(const COMPORT h, E7800Cmd cmd)
{
	return ComBreak(h) && ComWrite(h, &cmd, 1);
}

// Send command packet including parameters if required

static inline bool CmdSimple(const COMPORT h, E7800Cmd cmd, const void *data = 0, const u32 size = 0)
{
	unsigned char ret;
	return	CmdSend(h, cmd) && 
			(size == 0 || ComWrite(h, data, size)) &&
			ComRead(h, &ret, 1) &&
			(ret == 0);
}

// Initialise 7800 command

const COMPORT CmdInit(const char *pCommPort)
{
	return ComOpen(pCommPort, 500000);
}

// Terminate connection

void CmdTerm(const COMPORT h)
{
	ComClose(h);
}

// Request status

bool CmdStatus(const COMPORT h, E7800Status *status)
{
	return CmdSend(h, ECmd_Status) && ComRead(h, status, 1);
}

// Break currently executing program

bool CmdBreak(const COMPORT h)
{
	return CmdSimple(h, ECmd_Break);
}

// Return from break

bool CmdReturn(const COMPORT h)
{
	return CmdSimple(h, ECmd_Return);
}

// Write to cartridge memory space

bool CmdWriteCart(const COMPORT h, const u32 nAddr, const u32 nSize)
{
	SCmd7800WriteParam param = { nAddr, nSize };
	return CmdSimple(h, ECmd_WriteCart, &param, sizeof(SCmd7800WriteParam));
}

// Write raw data to cart, used for data upload

bool CmdWriteData(const COMPORT h, const void *pData, const u32 nSize)
{
	return ComWrite(h, pData, nSize);
}

// Check for write data completion

bool CmdWriteDataComplete(const COMPORT h)
{
	u8 c;
	return (ComRead(h, &c, 1) && (c == 0));
}

// Execute ROM (reboot) with given mapper

#pragma pack(push,1)
struct SCmdExecute
{
	u8		nMapper;
	u8		nMapperOptions;
	u16		nMapperAudio;
	u16		nMapperIRQEnable;
	u32		nSize;
	u16		nExtraFlags;
};
#pragma pack(pop)

bool CmdExecute(const COMPORT h, const u8 nMapper, const u8 nMapperOptions, const u16 nMapperAudio, const u16 nMapperIRQEnable, const u32 nSize, const u16 nExtraFlags)
{
	// send as little endian
	SCmdExecute mapper =
	{
		nMapper, 
		nMapperOptions,
		nMapperAudio,
		nMapperIRQEnable,
		nSize,
		nExtraFlags
	};

	return CmdSimple(h, ECmd_Execute, &mapper, sizeof(SCmdExecute));
}
