#pragma once
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>

struct GdbMsg
{
	uint32_t    m_MsgSz;
	const char* m_Msg;
};

//-----------------------------------------------------------------------------

uint64_t GetHighResTime(void);

long int SecToNano(double seconds);

double NanoToSec(uint64_t nanosecs);

void TimedWait(double secs);

//-----------------------------------------------------------------------------

int* GetFtoGPipes(void);

int* GetGtoFPipes(void);

bool SendCommand(const char* fmt, ...);

GdbMsg GdbOutput(void);