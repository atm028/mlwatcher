// MLWatcher.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <Winternl.h>

#include <vector>
#include <algorithm>
#include <functional>

/*

- Memory\Available Bytes reports available bytes; its value tends to fall during a memory leak.
- Memory\Committed Bytes reports the private bytes committed to processes; its value tends to rise during a memory leak.
- Process( process_name )\Private Bytes reports bytes allocated exclusively for a specific process; its value tends to rise for a leaking process.
- Process( process_name )\Working Set reports the shared and private bytes allocated to a process; its value tends to rise for a leaking process.
- Process( process_name )\Page Faults/sec reports the total number of faults (hard and soft faults) caused by a process; its value tends to rise for a leaking process.
- Process( process_name )\Page File Bytes reports the size of the paging file; its value tends to rise during a memory leak.
- Process( process_name )\Handle Count reports the number of handles that an application opened for objects it creates. 
Handles are used by programs to identify resources that they must access. 
The value of this counter tends to rise during a memory leak; however, you cannot rule out a leak simply because this counters value is stable.
*/
DWORD DW_FORMAT[CNT_NUM] = {
	PDH_FMT_LARGE, //"\\Memory\\Available Bytes"
	PDH_FMT_LARGE, //"\\Memory\\Committed Bytes"
	PDH_FMT_LARGE, //"\\Process(*)\\Private Bytes"
	PDH_FMT_LARGE, //"\\Process(*)\\Working Set"
	PDH_FMT_LARGE, //"\\Process(*)\\Page Faults/sec"
	PDH_FMT_LARGE, //"\\Process(*)\\Page File Bytes"
	PDH_FMT_LARGE, //"\\Process(*)\\Handle Count"
	PDH_FMT_DOUBLE //"\\Processor(_Total)\\% Processor Time"
};

class CounterPath
{
private:
	std::string pathName[CNT_NUM];
public:
	CounterPath(std::string processName)
	{
		pathName[0] = "\\Memory\\Available Bytes";
		pathName[1] = "\\Memory\\Committed Bytes";
		pathName[2] = "\\Process("+processName+"*)\\Private Bytes";
		pathName[3] = "\\Process("+processName+"*)\\Working Set";
		pathName[4] = "\\Process("+processName+"*)\\Page Faults/sec";
		pathName[5] = "\\Process("+processName+"*)\\Page File Bytes";
		pathName[6] = "\\Process("+processName+"*)\\Handle Count";
		pathName[7] = "\\Processor(_Total)\\% Processor Time";
	}

	std::string operator[](int index)
	{
		return pathName[index];
	}
};

bool isPIDAlive(int pid) 
{
	DWORD aProcesses[1024], cbNeeded;
	EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded);
	for(int i = 0; i < cbNeeded/sizeof(DWORD); i++)
		if(aProcesses[i] == pid) 
		{
			return true;
		}
	return false;
}

void usage() 
{
	printf("Usage:\n MLWatcher.exe <PID> <Host name> <Log file name> <counter read timeout ms(optional)> \n");
}



int main(int argc, char* argv[])
{
	if(argc < 4)
	{
		printf("Wrong number of arguments\n");
		usage();
		return -1;
	}

    int iPID = 0, cnt = 0;
	char hostName[256];
	wchar_t wcharHostName[256];
		
	DWORD dwFormat = PDH_FMT_LONG; 

   
	sscanf(argv[1], "%d", &iPID);
	if(iPID == 0)
	{
		printf("Wrong format of PID \n");
		usage();
		return -1;
	}

	HANDLE Handle = OpenProcess(
        PROCESS_QUERY_INFORMATION,
        FALSE,
        iPID
    );

	TCHAR buffer[MAX_PATH];
	GetProcessImageFileName(Handle, buffer, MAX_PATH);
	CloseHandle(Handle);
	std::wstring tmp(buffer);
	std::string str(tmp.begin(), tmp.end());
	size_t found;

	found = str.find_first_of("\\");
	while(found != std::string::npos)
	{
		str = str.substr(found+1);
		found = str.find_first_of("\\");
	}
	found = str.find_first_of(".");
	str = str.substr(0, found);
	CounterPath COUNTER_PATH(str);
	

	if(!isPIDAlive(iPID))
	{
		printf("Process with PID %d does not exist\n", iPID);
		return -1;
	}

    sscanf(argv[2], "%s", hostName);	
	mbstowcs(wcharHostName, hostName, strlen(hostName)+1);
	LPWSTR pstrHostName = wcharHostName;

	int cntReadTo = DEFAULT_TO;
	if(argv[4])
		sscanf(argv[4], "%d", &cntReadTo);
	else
		printf("Counter read timeout is not passed, use default %d ms value\n", DEFAULT_TO);	
	

	FILE *fp = fopen((char*)argv[3], "w");	
	
	std::vector<__int64> privateBytes;

	std::vector<PerformanceCounter*> pCntV;
	for(cnt = 0; cnt < CNT_NUM; cnt++)
		pCntV.push_back(new PerformanceCounter(pstrHostName, COUNTER_PATH[cnt], DW_FORMAT[cnt]));	
	
	std::vector<PerformanceCounter*>::iterator it;

	while(isPIDAlive(iPID))
	{			
		for(it = pCntV.begin(); it != pCntV.end(); it++)
		{
			PerformanceCounter* pc = *it;
			if(pc->getFormat() == PDH_FMT_LARGE)
			{		
				size_t hlen = wcslen(pstrHostName) + COUNTER_PATH[2].length()+2;
				size_t rlen = wcslen(pc->getPath());
				if(hlen == rlen)
				{
					std::nth_element(privateBytes.begin(), privateBytes.begin() + privateBytes.size()/2, privateBytes.end());
					if(privateBytes.size())
						fprintf(fp, "%d \n", privateBytes[privateBytes.size()/2]);

					privateBytes.push_back(pc->getLongValue());					
				}
			}
		}

		Sleep(cntReadTo);
	}

	fclose(fp);

	for(it = pCntV.begin(); it != pCntV.end(); it++)
	{
		PerformanceCounter* pc = *it;
		delete pc;
	}

    return 0;
}


 
