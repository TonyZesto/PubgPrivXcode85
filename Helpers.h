#pragma once
#include "stdafx.h"

namespace Helpers
{
	void LogAddress(char* szName, int64_t iAddress);
	void LogError(char* szMessage);
	void Log(char* szMessage);
	void HookFunction(PVOID *oFunction, PVOID pDetour);
	void UnhookFunction(PVOID *oFunction, PVOID pDetour);
}