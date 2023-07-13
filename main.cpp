#include <stdio.h>
#include "7800cmd.h"
#include "mapper.h"

void Usage(const char *cmd)
{
	printf("%s -com {comport:} [-run rom.a78]\n", cmd);
}

bool UploadToCart(const COMPORT com, FILE *f, u32 nOffset, u32 nLoadAddr, u32 nSize)
{
	if (CmdWriteCart(com, nLoadAddr, nSize))
	{
		fseek(f, nOffset, SEEK_SET);

		// write accepted, send the data
		printf("Writing %dK to $%05x: ", nSize / 1024, nLoadAddr);

		// read from file and write to serial
		u32 nLeft = nSize;
		while (nLeft)
		{
			u8 buf[4096];
			u32 nRead = nLeft > 4096 ? 4096 : nLeft;

			fread(buf, 1, nRead, f);
			printf("+");
			if (CmdWriteData(com, buf, nRead))
			{
				printf("\b*");
			}
			else
			{
				break;
			}

			nLeft -= nRead;
		}
		if ((nLeft == 0) && CmdWriteDataComplete(com))
		{
 			printf(" OK\n");
			return true;
		}
		else
		{
			printf(" ERROR\n");
			return false;
		}
	}
	else
	{
		printf("Unable to write data.\n");
		return false;
	}
}

bool IsBankset(const GDMapperInfo *pInfo)
{
	return ((pInfo->nMapper == EA78_V4_MAPPER_LINEAR || pInfo->nMapper == EA78_V4_MAPPER_SUPERGAME) && (pInfo->nMapperOptions & EA78_V4_MAPPER_LINEAR_BANKSET));
}

int main(int argc, const char **argv)
{

	// no parameters, show usage
	
	if (argc == 1)
	{
		Usage(argv[0]);
		return 0;
	}

	// grab comport and rom to run

	const char *pComPort = 0;
	const char *pRunRom = 0;

	for (int n = 1; n < argc; n++)
	{
		// com port
		if ((_stricmp(argv[n], "-com") == 0) && ((n + 1) < argc))
		{
			pComPort = argv[++n];
		}

		// rom to run
		else if ((_stricmp(argv[n], "-run") == 0) && ((n + 1) < argc))
		{
			pRunRom = argv[++n];
		}
	}

	// see if we have enough to go on

	if (!(pComPort && pRunRom))
	{
		Usage(argv[0]);
		return 0;
	}

	FILE *f = 0;
	COMPORT com = COMPORT_INVALID;
	do
	{
		// try and connect to the 7800GD
		com = CmdInit(pComPort);
		if (com == COMPORT_INVALID)
		{
			printf("Unable to open '%s'...\n", pComPort);
			break;
		}

		// now try and open the file
		if (fopen_s(&f, pRunRom, "rb") != 0)
		{
			printf("Unable to open '%s'...\n", pRunRom);
			break;
		}

		// see if the file looks valid
		GDMapperInfo mapper;
		if (!Get7800Mapper(&mapper, f))
		{
			printf("File is not valid '%s'...\n", pRunRom);
			break;
		}

		// all good, lets make sure we're in a state to upload (menu or in break)
		E7800Status status;
		if (CmdStatus(com, &status))
		{
			if (status == EStatus_Running)
			{
				if (!CmdBreak(com))
				{
					printf("Unable to break.\n");
					break;
				}
			}
		}
		else
		{
			printf("Unable to get status.\n");
			break;
		}

		// now upload, bankset has two sections
		if (UploadToCart(com, f, 0x80, mapper.nLoadAddr, mapper.nSize)
			&& (!IsBankset(&mapper) || UploadToCart(com, f, 0x80 + mapper.nSize, mapper.nLoadAddr | 0x80000, mapper.nSize)))
		{
			// setup and execute with given mapper details
			if (CmdExecute(	com,
							mapper.nMapper,
							mapper.nMapperOptions,
							mapper.nMapperAudio,
							mapper.nMapperIRQEnable,
							mapper.nSize,
							mapper.nExtraFlags))
			{
				printf("Executing...\n");
			}
			else
			{
				printf("Unable to execute.\n");
			}
		}
	}
	while(0);
	
	if (com != COMPORT_INVALID) CmdTerm(com);
	if (f) fclose(f);

	return 0;
}