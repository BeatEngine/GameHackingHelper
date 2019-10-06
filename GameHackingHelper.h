#pragma once
#pragma comment( lib, "Kernel32" )
#pragma comment( lib, "User32" )
#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN64
#define PRIXXX PRIx64
#else
#define PRIXXX PRIx32
#endif // WIN64

bool strwcmp(char* str, wchar_t* wstr)
{
	int sz = strlen(str);

	char* cmpcp = (char*)calloc(sz * 2 + 1, sizeof(char));
	int o = 0;
	for (int i = 0; i < sz * 2; i++)
	{
		if (wstr[i] > 31 && wstr[i] < 127)
		{
			cmpcp[o] = wstr[i];
			o++;
		}
		else
		{
			cmpcp[o] = 0;
			break;
		}
	}

	for (int i = 0; i < sz; i++)
	{
		if (str[i] != cmpcp[i])
		{
			//printf("proc:%s\n",cmpcp);
			free(cmpcp);
			return false;
		}
	}
	//printf("found:%s\n",cmpcp);
	free(cmpcp);
	return true;
}

int GetProcessFileName(HANDLE processHandle, wchar_t* name)
{
	wchar_t path[400] = { 0 };
	int i = GetProcessImageFileNameW(processHandle, path, 400);
	if (i > 0)
	{
		int sz = i;
		while (i>0)
		{
			if (path[i] == '\\' || path[i] == '/')
			{
				i++;
				break;
			}
			i--;
		}
		int u = 0;
		while (i<sz)
		{
			name[u] = path[i];
			i++;
			u++;
		}
		name[u] = '\0';
		return u;
	}
	return 0;
}

HANDLE attach(char* ProcessName, int* outPID)
{
	HANDLE processhandle;
	DWORD* pids = (DWORD*)calloc(2000, sizeof(DWORD));
	DWORD procs = 0;
	wchar_t pname[200] = { 0 };
	int pnameSz = 0;

	if (EnumProcesses(pids, 2000, &procs))
	{
		int results = procs / sizeof(DWORD);
		HANDLE tmph = 0;
		for (int i = 0; i < results; i++)
		{
			tmph = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pids[i]);
			if(tmph)
			{
				memset(pname, 0, sizeof(wchar_t)*199);
				pnameSz = GetProcessFileName(tmph, pname);
				if (pnameSz > 0)
				{
					if (strwcmp(ProcessName, pname))
					{
						free(pids);
						return tmph;
					}
				}
				CloseHandle(tmph);
			}
		}
	}
	free(pids);
	return NULL;
}

void detach(HANDLE processHandle)
{
	CloseHandle(processHandle);
}

uintptr_t getmoduleaddress(HANDLE processHandle, char* processModuleName)
{
	uintptr_t tmodule = 0;
	HMODULE* moduleArray;
	uintptr_t moduleArrayBytes;
	DWORD bytesRequired;
	unsigned int moduleCount;

	if (processHandle)
	{
		/*HMODULE hMods[1024];
		DWORD cbNeeded;
		unsigned int i;
		if (EnumProcessModules(processHandle, hMods, sizeof(hMods), &cbNeeded))
		{
			for (i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
			{
				wchar_t szModName[MAX_PATH];

				// Get the full path to the module's file.

				if (GetModuleFileNameEx(processHandle, hMods[i], szModName,
					sizeof(szModName) / sizeof(wchar_t)))
				{
					// Print the module name and handle value.

					printf("%s (0x%08X)\n", szModName, hMods[i]);
				}
			}
		}*/


		if (EnumProcessModules(processHandle, NULL, 0, &bytesRequired))
		{
			if (bytesRequired)
			{
				moduleArrayBytes = (uintptr_t)LocalAlloc(LPTR, bytesRequired);
				if (moduleArrayBytes)
				{
					moduleCount = bytesRequired / sizeof(HMODULE);
					moduleArray = (HMODULE*)moduleArrayBytes;

					if (EnumProcessModules(processHandle, moduleArray, bytesRequired, &bytesRequired))
					{
						wchar_t tModuleName[300];
						for (int i = 0; i < moduleCount; i++)
						{
							memset(tModuleName, 0, 300);
							GetModuleBaseNameW(processHandle, moduleArray[i], tModuleName, 299);
							if (strwcmp(processModuleName, tModuleName))
							{
								tmodule = (uintptr_t)moduleArray[i];
								break;
							}
						}

					}
					LocalFree((HLOCAL)moduleArrayBytes);
				}
			}
		}
	}
	return tmodule;
}


class ProcessAddress
{
	HANDLE processHandle;
	uintptr_t address;

public:
	ProcessAddress(HANDLE processHandle)
	{
		this->processHandle = processHandle;
		address = 0;
	}
	ProcessAddress(HANDLE processHandle, uintptr_t address)
	{
		this->processHandle = processHandle;
		this->address = address;
	}
																/** First offset is for base.   */
	ProcessAddress(HANDLE processHandle, uintptr_t moduleBaseAddress, uintptr_t* offsetList, int offsets)
	{
		this->processHandle = processHandle;
		this->loadByMultiLevelPointer(moduleBaseAddress, offsetList, offsets);
	}

	void loadByMultiLevelPointer(uintptr_t moduleBaseAddress,uintptr_t* offsetList, int offsets)
	{
		address = moduleBaseAddress;
		int i = 0;
		while (ReadProcessMemory(processHandle, (void*)(address + offsetList[i]), &address, sizeof(uintptr_t), 0))
		{
			//printf("[%d]: 0x%" PRIXXX "\n", i, address);
			if (i >= offsets-2)
			{
				address += offsetList[offsets - 1];
				return;
			}
			i++;
		}
		address = 0;
	}

	template<class T> T read()
	{
		if (address == 0)
		{
			return 0;
		}
		T tmp;
		if (ReadProcessMemory(processHandle, (void*)(address), &tmp, sizeof(T), 0)!=0)
		{
			return tmp;
		}
		return 0;
	}

	template<class T> bool write(T value)
	{
		if (address == 0)
		{
			return false;
		}
		return WriteProcessMemory(processHandle, (void*)(address), &value, sizeof(T), 0);
	}

private:

};




