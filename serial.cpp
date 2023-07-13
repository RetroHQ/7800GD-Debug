#include "serial.h"

// Opens the specified serial port, configures its timeouts, and sets its
// baud rate.  Returns a handle on success, or INVALID_HANDLE_VALUE on failure.
const COMPORT ComOpen(const char *device, u32 baud_rate)
{
	HANDLE port = CreateFileA(device, GENERIC_READ | GENERIC_WRITE, 0, NULL,
	OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (port == INVALID_HANDLE_VALUE)
	{
		return COMPORT_INVALID;
	}
 
	// Flush away any bytes previously read or written.
	BOOL success = FlushFileBuffers(port);
	if (!success)
	{
		CloseHandle(port);
		return COMPORT_INVALID;
	}
 
	// Configure read and write operations
	COMMTIMEOUTS timeouts = {0};
	timeouts.ReadIntervalTimeout = 0;
	timeouts.ReadTotalTimeoutConstant = 500;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 500;
	timeouts.WriteTotalTimeoutMultiplier = 0;
 
	success = SetCommTimeouts(port, &timeouts);
	if (!success)
	{
		CloseHandle(port);
		return COMPORT_INVALID;
	}
 
	// Set the baud rate and other options.
	DCB state = {0};
	state.DCBlength = sizeof(DCB);
	state.BaudRate = baud_rate;
	state.ByteSize = 8;
	state.Parity = NOPARITY;
	state.StopBits = ONESTOPBIT;
	success = SetCommState(port, &state);
	if (!success)
	{
		CloseHandle(port);
		return COMPORT_INVALID;
	}
 
	return port;
}

void ComClose(const COMPORT h)
{
	CloseHandle(h);
}

bool ComBreak(const COMPORT h)
{
	BOOL bOk = TRUE;
	bOk &= SetCommBreak(h);
	Sleep(1);
	bOk &= ClearCommBreak(h);
	return bOk;
}

bool ComWrite(const COMPORT h, const void *pData, const int nSize)
{
	DWORD nWritten;
	return WriteFile(h, pData, nSize, &nWritten, 0) && (nWritten == nSize);
}

bool ComRead(const COMPORT h, void *pData, const int nSize)
{
	DWORD nRead;
	return ReadFile(h, pData, nSize, &nRead, 0) && (nRead == nSize);
}
