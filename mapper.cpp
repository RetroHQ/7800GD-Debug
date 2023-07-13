#include <stdio.h>
#include <string.h>
#include "mapper.h"
#include "7800cmd.h"

// Version 3.1 header additions

static const u16 A78_TYPE_V3_POKEY_800 = 1 << 15;
static const u16 A78_TYPE_V3_EXRAM_M2 = 1 << 14;
static const u16 A78_TYPE_V3_BANKSET = 1 << 13;

static const u8 A78_TYPE_V3_IRQ_POKEY_800 = 1 << 4;
static const u8 A78_TYPE_V3_IRQ_YM2151_460 = 1 << 3;
static const u8 A78_TYPE_V3_IRQ_POKEY_440 = 1 << 2;
static const u8 A78_TYPE_V3_IRQ_POKEY_450 = 1 << 1;
static const u8 A78_TYPE_V3_IRQ_POKEY_4000 = 1 << 0;

// Version 1 header

static const u16 A78_TYPE_POKEY_4000 = 1 << 0;
static const u16 A78_TYPE_SUPERGAME_BANKING = 1 << 1;
static const u16 A78_TYPE_EXRAM_4000 = 1 << 2;
static const u16 A78_TYPE_ROM_4000 = 1 << 3;
static const u16 A78_TYPE_BANK6_4000 = 1 << 4;
static const u16 A78_TYPE_EXRAM_X2 = 1 << 5;
static const u16 A78_TYPE_POKEY_450 = 1 << 6;
static const u16 A78_TYPE_EXRAM_A8 = 1 << 7;
static const u16 A78_TYPE_ACTIVISION_BANKING = 1 << 8;
static const u16 A78_TYPE_ABSOLUTE_BANKING = 1 << 9;
static const u16 A78_TYPE_POKEY_440 = 1 << 10;
static const u16 A78_TYPE_YM2151_460 = 1 << 11;
static const u16 A78_TYPE_SOUPER = 1 << 12;

static const u16 A78_TV_REGION = 1 << 0; // 0:NTSC, 1:PAL
static const u16 A78_TV_COMPOSITE = 1 << 1; // composite artifacts are intended

static const u16 A78_SAVE_HSC = 1 << 0;
static const u16 A78_SAVE_SAVEKEY = 1 << 1;
static const u16 A78_VIDEO_HINT_COMPOSITE = 1 << 1;

static const u8 A78_CONTROLLER_ATARIVOX_SAVEKEY = 10;
static const u8 A78_CONTROLLER_MEGA7800 = 12;

#pragma pack(push,1)
struct A78Header
{
	// Standard V1 header
	char	szMagic[16];
	char	szTitle[32];
	u32		nSize;					// big endian
	u16		nType;					// big endian
	u8		nController1;
	u8		nController2;
	u8		nVideoHint;
	u8		nSaveDevice;
	u8		reserved[3];
	u8		nV3IRQEnable;			// new V3 field
	u8		nExpansionModule;		// $3f
	
	// V4 header
	u8		nV4Mapper;				// $40
	u8		nV4MapperOptions;
	u16		nV4MapperAudio;			// big endian
	u16		nV4MapperIRQEnable;		// big endian

	// Read first to algin rest of header
	u8		nVersion;
};
#pragma pack(pop)

static void StripTrailingSpaces(char *pData, u32 nSize)
{
	pData += nSize;
	while (nSize--)
	{
		if (*--pData != ' ') break;
		*pData = 0;
	}
}

static u32 EndianSwapDword(u32 nDword)
{
	return	((nDword & 0xff000000) >> 24) |
			((nDword & 0x00ff0000) >> 8) |
			((nDword & 0x0000ff00) << 8) |
			((nDword & 0x000000ff) << 24);
}

static u16 EndianSwapWord(u16 nWord)
{
	return	((nWord & 0x0000ff00) >> 8) |
			((nWord & 0x000000ff) << 8);
}

bool Get7800Mapper(GDMapperInfo *pMapper, FILE *pFile)
{
	// Read in the header from the open file
	A78Header sHeader;
	fseek(pFile, 0, SEEK_SET);
	fread(&sHeader.nVersion, 1, 1, pFile);				// version first such that the rest is aligned
	fread(&sHeader, 1, sizeof(sHeader) - 1, pFile);

	// Check the header magic is there
	StripTrailingSpaces(sHeader.szMagic, sizeof(sHeader.szMagic));
	if (memcmp(sHeader.szMagic, "ATARI7800", 10) != 0 || sHeader.nVersion > 4)
	{
		return false;
	}

	// Endian swap non-byte fields for little endian
	sHeader.nSize = EndianSwapDword(sHeader.nSize);
	sHeader.nType = EndianSwapWord(sHeader.nType);
	sHeader.nV4MapperAudio = EndianSwapWord(sHeader.nV4MapperAudio);
	sHeader.nV4MapperIRQEnable = EndianSwapWord(sHeader.nV4MapperIRQEnable);

	// If this is <V3 then just make sure there are no bits set there shouldn't be
	if (sHeader.nVersion < 3)
	{
		sHeader.nType &= ~(A78_TYPE_V3_POKEY_800|A78_TYPE_V3_EXRAM_M2|A78_TYPE_V3_BANKSET);
		sHeader.nV3IRQEnable = 0;
	}

	// if this is <V4 then create a valid V4 header
	if (sHeader.nVersion < 4)
	{
		sHeader.nV4Mapper = 0;
		sHeader.nV4MapperAudio = 0;
		sHeader.nV4MapperOptions = 0;
		sHeader.nV4MapperIRQEnable = 0;

		// SuperGame bit set
		if (sHeader.nType & A78_TYPE_SUPERGAME_BANKING)
		{
			// SuperGame mapper
			sHeader.nV4Mapper = EA78_V4_MAPPER_SUPERGAME;

			// add @4000 options
			if (sHeader.nType & A78_TYPE_EXRAM_4000)
			{
				sHeader.nV4MapperOptions = EA78_V4_MAPPER_SUPERGAME_4KOPT_RAM;
			}
			else if (sHeader.nType & A78_TYPE_ROM_4000)
			{
				sHeader.nV4MapperOptions = EA78_V4_MAPPER_SUPERGAME_4KOPT_EXROM;
			}
			else if (sHeader.nType & A78_TYPE_BANK6_4000)
			{
				sHeader.nV4MapperOptions = EA78_V4_MAPPER_SUPERGAME_4KOPT_EXFIX;
			}
			else if (sHeader.nType & A78_TYPE_V3_EXRAM_M2)
			{
				sHeader.nV4MapperOptions = EA78_V4_MAPPER_SUPERGAME_4KOPT_EXRAM_M2;
			}
			else if (sHeader.nType & A78_TYPE_EXRAM_X2)
			{
				sHeader.nV4MapperOptions = EA78_V4_MAPPER_SUPERGAME_4KOPT_EXRAM_X2;
			}
			else if (sHeader.nType & A78_TYPE_EXRAM_A8)
			{
				sHeader.nV4MapperOptions = EA78_V4_MAPPER_SUPERGAME_4KOPT_EXRAM_A8;
			}

			// bankset extension
			if (sHeader.nType & A78_TYPE_V3_BANKSET)
			{
				sHeader.nV4MapperOptions |= EA78_V4_MAPPER_SUPERGAME_BANKSET;
			}
		}

		// Activision bit set
		else if (sHeader.nType & A78_TYPE_ACTIVISION_BANKING)
		{
			sHeader.nV4Mapper = EA78_V4_MAPPER_ACTIVISION;
		}

		// Absolute bit set
		else if (sHeader.nType & A78_TYPE_ABSOLUTE_BANKING)
		{
			sHeader.nV4Mapper = EA78_V4_MAPPER_ABSOLUTE;
		}

		// Souper bit set
		else if (sHeader.nType & A78_TYPE_SOUPER)
		{
			sHeader.nV4Mapper = EA78_V4_MAPPER_SOUPER;
		}

		// No mapper bits set, it's linear
		else
		{
			// Linear mapper
			sHeader.nV4Mapper = EA78_V4_MAPPER_LINEAR;
			
			// add @4000 options
			if (sHeader.nType & A78_TYPE_EXRAM_4000)
			{
				sHeader.nV4MapperOptions = EA78_V4_MAPPER_LINEAR_4KOPT_RAM;
			}
			else if (sHeader.nType & A78_TYPE_V3_EXRAM_M2)
			{
				sHeader.nV4MapperOptions = EA78_V4_MAPPER_LINEAR_4KOPT_EXRAM_M2;
			}
			else if (sHeader.nType & A78_TYPE_EXRAM_A8)
			{
				sHeader.nV4MapperOptions = EA78_V4_MAPPER_LINEAR_4KOPT_EXRAM_A8;
			}

			// bankset extension
			if (sHeader.nType & A78_TYPE_V3_BANKSET)
			{
				sHeader.nV4MapperOptions |= EA78_V4_MAPPER_SUPERGAME_BANKSET;
			}
		}

		// DUAL POKEY@440/450
		if ((sHeader.nType & (A78_TYPE_POKEY_440 | A78_TYPE_POKEY_450)) == (A78_TYPE_POKEY_440 | A78_TYPE_POKEY_450))
		{
			sHeader.nV4MapperAudio |= EA78_V4_AUDIO_POKEY_440_450;
			
			// IRQ's for POKEY
			if (sHeader.nV3IRQEnable & A78_TYPE_V3_IRQ_POKEY_440)
			{
				sHeader.nV4MapperIRQEnable |= EA78_V4_IRQ_ENABLE_POKEY_1;
			}
			if (sHeader.nV3IRQEnable & A78_TYPE_V3_IRQ_POKEY_450)
			{
				sHeader.nV4MapperIRQEnable |= EA78_V4_IRQ_ENABLE_POKEY_2;
			}
		}
		// POKEY@440
		else if (sHeader.nType & A78_TYPE_POKEY_440)
		{
			sHeader.nV4MapperAudio |= EA78_V4_AUDIO_POKEY_440;

			// IRQ's for POKEY
			if (sHeader.nV3IRQEnable & A78_TYPE_V3_IRQ_POKEY_440)
			{
				sHeader.nV4MapperIRQEnable |= EA78_V4_IRQ_ENABLE_POKEY_1;
			}
		}
		// POKEY@450
		else if (sHeader.nType & A78_TYPE_POKEY_450)
		{
			sHeader.nV4MapperAudio |= EA78_V4_AUDIO_POKEY_450;

			// IRQ's for POKEY
			if (sHeader.nV3IRQEnable & A78_TYPE_V3_IRQ_POKEY_450)
			{
				sHeader.nV4MapperIRQEnable |= EA78_V4_IRQ_ENABLE_POKEY_1;
			}
		}
		// POKEY@4000
		else if (sHeader.nType & A78_TYPE_POKEY_4000)
		{
			sHeader.nV4MapperAudio |= EA78_V4_AUDIO_POKEY_4000;

			// IRQ's for POKEY
			if (sHeader.nV3IRQEnable & A78_TYPE_V3_IRQ_POKEY_4000)
			{
				sHeader.nV4MapperIRQEnable |= EA78_V4_IRQ_ENABLE_POKEY_1;
			}
		}
		// POKEY@800
		else if (sHeader.nType & A78_TYPE_V3_POKEY_800)
		{
			sHeader.nV4MapperAudio |= EA78_V4_AUDIO_POKEY_800;

			// IRQ's for POKEY
			if (sHeader.nV3IRQEnable & A78_TYPE_V3_IRQ_POKEY_800)
			{
				sHeader.nV4MapperIRQEnable |= EA78_V4_IRQ_ENABLE_POKEY_1;
			}
		}

		// YM2151
		if (sHeader.nType & A78_TYPE_YM2151_460)
		{
			sHeader.nV4MapperAudio |= EA78_V4_AUDIO_YM2151;

			// IRQ's for YM
			if (sHeader.nV3IRQEnable & A78_TYPE_V3_IRQ_YM2151_460)
			{
				sHeader.nV4MapperIRQEnable |= EA78_V4_IRQ_ENABLE_YM2151;
			}
		}
	}

	// Mapper settings
	pMapper->nMapper = sHeader.nV4Mapper;
	pMapper->nMapperOptions = sHeader.nV4MapperOptions;
	pMapper->nMapperAudio = sHeader.nV4MapperAudio;
	pMapper->nMapperIRQEnable = sHeader.nV4MapperIRQEnable;
	pMapper->nExtraFlags = 0;
	
	// Figure out load address required for the given mapper
	u32 nAddr = 0;
	
	// Linear mapper is literally linear address space mapped
	if (sHeader.nV4Mapper == EA78_V4_MAPPER_LINEAR)
	{
		// divide by 2 for bankset ROM, we'll deal with loading both sections separely
		if (sHeader.nV4MapperOptions & EA78_V4_MAPPER_LINEAR_BANKSET)
		{
			sHeader.nSize /= 2;
		}

		// see if the image is larger than possible just fail
		if (sHeader.nSize > 0xd000)
		{
			return false;
		}

		// load address is from top of address space down, any ROM size is possible
		// any RAM located at 4000 will be populated if the payload is large enough
		nAddr = 0x10000 - sHeader.nSize;
	}

	// SuperGame has the fist bank mapped as EXROM or EXRAM
	else if (sHeader.nV4Mapper == EA78_V4_MAPPER_SUPERGAME)
	{
		// if EXRAM, then load from second bank up, first bank will be left
		// uninitialised as RAM
		// 1MB supergame uses one of the rom banks for ram, so just load from 0
		if (sHeader.nSize <= (512*1024))
		{
			switch (sHeader.nV4MapperOptions & EA78_V4_MAPPER_SUPERGAME_4KOPT_MASK)
			{
				case EA78_V4_MAPPER_SUPERGAME_4KOPT_RAM:
				case EA78_V4_MAPPER_SUPERGAME_4KOPT_EXRAM_M2:
				case EA78_V4_MAPPER_SUPERGAME_4KOPT_EXRAM_X2:
				case EA78_V4_MAPPER_SUPERGAME_4KOPT_EXRAM_A8:
					nAddr = 0x4000;
					break;
			}
		}

		// see if we have enough RAM for this, max 1MB
		if ((sHeader.nSize + nAddr) > 0x100000)
		{
			return false;
		}

		// divide by 2 for bankset ROM, we'll deal with loading both sections separely
		if (sHeader.nV4MapperOptions & EA78_V4_MAPPER_SUPERGAME_BANKSET)
		{
			sHeader.nSize /= 2;
		}
	}

	// check for HSC and enable support
	if (sHeader.nSaveDevice & A78_SAVE_HSC)
	{
		pMapper->nExtraFlags |= EExtra_HSC;
	}

	// enable composite blending if required
	if (sHeader.nVideoHint & A78_VIDEO_HINT_COMPOSITE)
	{
		pMapper->nExtraFlags |= EExtra_COMPOSITE;
	}

	// enable savekey in port 2 if required
	if ((sHeader.nSaveDevice & A78_SAVE_SAVEKEY) || (sHeader.nController2 == A78_CONTROLLER_ATARIVOX_SAVEKEY))
	{
		pMapper->nExtraFlags |= EExtra_SAVEKEY;
	}

	// enable native mega7800 support if required
	if (sHeader.nController1 == A78_CONTROLLER_MEGA7800 || sHeader.nController2 == A78_CONTROLLER_MEGA7800)
	{
		pMapper->nExtraFlags |= EExtra_MEGA7800;
	}

	// Load address and size
	pMapper->nLoadAddr = nAddr;
	pMapper->nSize = sHeader.nSize;

	return true;
}