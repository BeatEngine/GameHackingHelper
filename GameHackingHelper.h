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

char* strwcopy(wchar_t* wstr)
{
	int i = 0;
	while (i < 10000)
	{
		if (wstr[i] == 0)
		{
			break;
		}
		i++;
	}
	char* result = (char*)calloc(i + 1, sizeof(char));
	for (int c = 0; c < i; c++)
	{
		result[c] = (char)(int)(wstr[c]);
	}
	result[i] = 0;
	return result;
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
				char* res = strwcopy(pname);
				if (pnameSz > 0)
				{
					if (strcmp(ProcessName, res)==0)
					{
						free(pids);
						free(res);
						return tmph;
					}
				}
				free(res);
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


		if (EnumProcessModulesEx(processHandle, NULL, 0, &bytesRequired, LIST_MODULES_ALL))
		{
			if (bytesRequired)
			{
				moduleArrayBytes = (uintptr_t)LocalAlloc(LPTR, bytesRequired);
				if (moduleArrayBytes)
				{
					moduleCount = bytesRequired / 8;
					moduleArray = (HMODULE*)moduleArrayBytes;

					if (EnumProcessModulesEx(processHandle, moduleArray, bytesRequired, &bytesRequired, LIST_MODULES_ALL))
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

	void removeSpaces(char* str)
	{
		int l = strlen(str)-1;
		int c = 0;
		for (int i = 0; i < l; i++)
		{
			if (str[i] == 32)
			{
				for (c = i; c < l; c++)
				{
					str[c] = str[c + 1];
				}
				str[l] = 0;
				l--;
			}
		}
		
	}

	bool loadByModulePattern(HANDLE processHandle, uintptr_t moduleBaseAddress,const char* pattern)
	{
		this->address = 0;
		this->processHandle = processHandle;
		char* workpattern = (char*)calloc(strlen(pattern)+1, sizeof(char));
		memcpy(workpattern, pattern, strlen(pattern));
		removeSpaces(workpattern);
		
		unsigned char tmp = 0;
		uintptr_t addr = moduleBaseAddress;
		int i;
		int patlen = strlen(workpattern);
		int numberOfBytes = patlen / 2;
		bool* skip = (bool*)calloc(numberOfBytes+1, sizeof(bool));
		unsigned char* bytes = (unsigned char*)calloc(numberOfBytes+1, sizeof(unsigned char));
		int k = 0;
		char tmpn[4] = { 0 };
		unsigned int tnb = 0;
		
		for (i = 0; i+1 < patlen; i+=2)
		{
			if (workpattern[i] == '?' || workpattern[i + 1] == '?')
			{
				skip[k] = false;
				bytes[k] = 0;
			}
			else
			{
				tmpn[0] = workpattern[i];
				tmpn[1] = workpattern[i+1];
				skip[k] = true;
				sscanf(tmpn,"%2x", &tnb);
				bytes[k] = (unsigned char)tnb;
			}
			k++;
		}
		
		unsigned char* arr = (unsigned char*)calloc(numberOfBytes+1, sizeof(char));
		while (ReadProcessMemory(processHandle, (void*)addr, &tmp, sizeof(char), 0))
		{
			if (tmp == bytes[0] || !skip[0])
			{
				if (ReadProcessMemory(processHandle, (void*)addr, arr, sizeof(char)*numberOfBytes, 0))
				{
					i = 0;
					for (i = 0; i < numberOfBytes; i++)
					{
						if (arr[i]!= bytes[i]&&skip[i])
						{
							i = 0;
							break;
						}
					}
					if (i != 0)
					{
						this->address = addr;
						break;
					}
				}
			}
			addr++;
		}
		
		free(skip);
		free(bytes);
		free(arr);
		free(workpattern);
		if (this->address == 0)
		{
			return false;
		}
		return true;
	}

	void load(HANDLE processHandle, uintptr_t address)
	{
		this->processHandle = processHandle;
		this->address = address;
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

	uintptr_t getAddress()
	{
		return address;
	}

private:

};




