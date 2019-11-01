#pragma

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

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

#ifdef __linux__ 

#define uintptr_t unsigned long long

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/fcntl.h>
#include <sys/sysctl.h>
#include <unistd.h>
#define HANDLE long

void ProcessnameByStatus(char* status_path, char* name_out)
{
	char data[200] = {0};
	long f = open(status_path,O_RDONLY);
	read(f,data,200);
	close(f);
	int o = 0;
	for(int i = 0; i < 200; i++)
	{
		if(data[i]=='\n')
		{
			o = i;
			while (data[i]!=':'&&i>0)
			{
				i--;
			}
			i++;
			while ((data[i]==' '||data[i]=='\t')&&i<200)
			{
				i++;
			}
			strncpy(name_out,(data+i),o-i);
			break;
		}
	}
}

int GetProcessFileName(long processHandle, char* name)
{
    DIR* proc = opendir("/proc/");
    if(proc==0)
    {
        closedir(proc);
        printf("Process lookup failed!\n");
        return 0;
    }
	dirent* tproc;
    while ((tproc = readdir(proc)) != 0)
    {
        if(atoi(tproc->d_name) != 0)
        {
			char process_dir_out[512] = {0};
            sprintf(process_dir_out, "/proc/%s/mem", tproc->d_name);
            char statdir[512] = {0};
            sprintf(statdir, "/proc/%s/status", tproc->d_name);
            long handle = open(process_dir_out, O_RDONLY);
            if(handle == processHandle)
            {
                char* fdata = (char*)calloc(sizeof(char),201);
                if(read(handle, fdata, 200)) {
                    close(handle);
					ProcessnameByStatus(statdir , name);
					closedir(proc);
                    free(fdata);
                    return 1;
                }
                free(fdata);
            }
        }
    }
    closedir(proc);
    return 0;
}

long attach(char* ProcessName, int* pid)
{
    DIR* proc = opendir("/proc/");
    if(proc==0)
    {
        closedir(proc);
        printf("Process lookup failed!\n");
        return 0;
    }
	struct dirent* tproc;
    while ((tproc = readdir(proc)) != 0)
    {
        if(atoi(tproc->d_name) != 0)
        {
			//printf("debug2: '%s'\n", tproc->d_name);
            char process_dir_out[512] = {0};
            sprintf(process_dir_out, "/proc/%s/mem", tproc->d_name);
            char statdir[512] = {0};
            sprintf(statdir, "/proc/%s/status", tproc->d_name);
			char prcnam[200] = {0};
			ProcessnameByStatus(statdir , prcnam);
            if(strwcmp(prcnam, ProcessName))
            {
				printf("found: '%s' (%d)\n", prcnam, *pid);
				long handle = 0;//open(process_dir_out, O_RDWR);
                char* fdata = (char*)calloc(sizeof(char),201);
				if(true)
				{
		
				char buff[128] = {0};
				//close(handle);
				
				printf("Attached to %s!\n", prcnam);
				*pid = atol(tproc->d_name);
				char BaseRegion[1024] = {0};
				sprintf(BaseRegion, "/proc/%s/mem",tproc->d_name);
				handle = open(BaseRegion, O_RDWR);
				if(handle>0)
				{
					free(fdata);
					closedir(proc);
					return handle;
				}
				else
				{
					printf("Error open process failed!\n");
				}
						
				}
                free(fdata);
            }
        }
    }
    closedir(proc);
    return 0;
}

void detach(long processHandle)
{
    close(processHandle);
}

uintptr_t getmoduleaddress(long processHandle, char* processModuleName)
{
    char FileLocation[1024] = {0};
    char BaseAddress[1024] = {0};
    char* ptr = 0;
    if(processHandle > 0)
    {
        char* FileBuffer = (char*)calloc(sizeof(char), 100000);
        if(FileBuffer != 0)
        {
            memset(FileBuffer, 0, 100000);

            for(int i = 0; read(processHandle, FileBuffer + i, 1) > 0; i++);
            if(processModuleName!=0)
            {
                if((ptr = strstr(FileBuffer, processModuleName)) != 0)
                {
                    while(*ptr != '\n' && ptr >= FileBuffer)
                    {
                        ptr--;
                    }
                    ptr++;

                    for(int i = 0; *ptr != '-'; i++)
                    {
                        BaseAddress[i] = *ptr;
                        ptr++;
                    }
                    uintptr_t ret = 0;
                    sscanf(BaseAddress, "%" PRIxPTR, &ret);
                    return ret;
                }
                else
                {
                    printf("Error couldn't find module \"%s\"!\n",processModuleName);
                }
            }
        }
    }
	return 0;
}

bool ReadProcessMemory(long processHandle , void* address, void* buffer, size_t size, int unused)
{
    lseek(processHandle, (uintptr_t)address, SEEK_SET);
    if(write(processHandle, buffer, size))
    {
        lseek(processHandle, 0, SEEK_SET);
        return true;
    }
    lseek(processHandle, 0, SEEK_SET);
    return false;
}

bool WriteProcessMemory(long processHandle , void* address, void* buffer, size_t size, int unused)
{
    lseek(processHandle, (uintptr_t)address, SEEK_SET);
    if(read(processHandle, buffer, size))
    {
        lseek(processHandle, 0, SEEK_SET);
        return true;
    }
    lseek(processHandle, 0, SEEK_SET);
    return false;
}




/** ####################################################################################################### */

#else

#pragma comment( lib, "Kernel32" )
#pragma comment( lib, "User32" )
#include <Windows.h>
#include <Psapi.h>


#ifdef _WIN64
#define PRIXXX PRIx64
#else
#define PRIXXX PRIx32
#endif // WIN64



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

#endif


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






