// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#include <stdio.h>
#include <tchar.h>

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <psapi.h>

#include <pdh.h>
#include <pdhmsg.h>
#pragma comment(lib, "pdh.lib")

#include <vector>
#include <string>
#include <sstream>
#include <Winnls.h>

#define	CNT_NUM	8
#define DEFAULT_TO	100

bool isPIDAlive(int pid);
void usage();

class PerformanceCounter
{
private:
	WCHAR strCntPath[256];
	HCOUNTER hCounter;
	HQUERY hQuery;
	PDH_FMT_COUNTERVALUE itemBuffer;
	PDH_STATUS pdhStatus;
	DWORD dwFormat;

public:
	PerformanceCounter(LPWSTR hostName, std::string pathName, DWORD pdhFormat) : dwFormat(pdhFormat)
	{
		std::wstring wstrPath = std::wstring(pathName.begin(), pathName.end());
		const wchar_t* wcharPath = wstrPath.c_str();
		swprintf(strCntPath, 256, L"\\\\%s%s", hostName, wcharPath);		
		pdhStatus = PdhOpenQuery(NULL, 0, &hQuery);
		pdhStatus = PdhAddCounter(hQuery, (LPCWSTR)strCntPath, 0, &hCounter);
	}

	~PerformanceCounter()
	{
		PdhCloseQuery(hQuery);
	}

	DWORD getFormat() { return dwFormat; }

	PWSTR getPath()
	{
		PWSTR path(strCntPath);
		return path;
	}

	__int64 getLongValue()
	{
		pdhStatus = PdhCollectQueryData(hQuery);
		//PDH_FMT_LARGE - memory
		//PDH_FMT_DOUBLE - CPU

		pdhStatus = PdhGetFormattedCounterValue(hCounter, dwFormat, (LPDWORD)NULL, &itemBuffer);
		if(pdhStatus == ERROR_SUCCESS)
			return itemBuffer.largeValue;
		return 0;
	}

	double getDoubleValue()
	{
		pdhStatus = PdhCollectQueryData(hQuery);
		//PDH_FMT_LARGE - memory
		//PDH_FMT_DOUBLE - CPU

		pdhStatus = PdhGetFormattedCounterValue(hCounter, dwFormat, (LPDWORD)NULL, &itemBuffer);
		if(pdhStatus == ERROR_SUCCESS)
			return itemBuffer.doubleValue;
		return 0;
	}
};

// TODO: reference additional headers your program requires here
