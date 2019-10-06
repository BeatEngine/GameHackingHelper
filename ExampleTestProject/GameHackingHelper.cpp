// GameHackingHelper.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//

#include "pch.h"
#include <inttypes.h>
#include <iostream>
#include "GameHackingHelper.h"

/*int main()
{
    std::cout << "Hello World!\n"; 

	
	#ifdef _WIN64
		char pname[] = "Test Application64.exe";
	#else
		char pname[] = "Test Application68.exe";
	#endif // WIN64
	int pid = 0;
	HANDLE proc = attach(pname, &pid);

	uintptr_t basemodule = getmoduleaddress(proc, pname);
	uintptr_t NTDLLmodule = getmoduleaddress(proc, (char*)"ntdll.dll");

	#ifdef _WIN64
		printf("process:0x%" PRIx64 "\nbase:0x%" PRIx64 "\nntdll:0x%" PRIx64 "\n", proc, basemodule, NTDLLmodule);
	#else
		printf("process:0x%" PRIx32 "\nbase:0x%" PRIx32 "\nntdll:0x%" PRIx32 "\n", proc, basemodule, NTDLLmodule);
	#endif // WIN64

	ProcessAddress memPtr = ProcessAddress(proc);
	


	detach(proc);
	getchar();
	return 0;
}*/


int main()
{
	int pid;
	HANDLE csgo = attach((char*)"csgo.exe", &pid);

		printf("csgo.exe: 0x%" PRIXXX "\n", csgo);

		uintptr_t mainModule = getmoduleaddress(csgo, (char*)"csgo.exe");
		printf("csgo.exe: 0x%" PRIXXX "\n", mainModule);
		uintptr_t client_panorama = getmoduleaddress(csgo, (char*)"client_panorama.dll");

		printf("client_panorama.dll: 0x%" PRIXXX "\n", client_panorama);
		//ProcessAddress playerXtmp = ProcessAddress(csgo,0x3CF3D4C);
		uintptr_t plxOffsets[5] = { 0xCF7A4C, 0x24, 0x38, 0x568, 0x14C };
		ProcessAddress playerX = ProcessAddress(csgo, client_panorama, plxOffsets, 5);
		uintptr_t plyOffsets[5] = { 0xCF7A4C, 0x24, 0x38, 0x568, 0x150 };
		ProcessAddress playerY = ProcessAddress(csgo, client_panorama, plyOffsets, 5);
		uintptr_t plzOffsets[5] = { 0xCF7A4C, 0x24, 0x38, 0x568, 0x154 };
		ProcessAddress playerZ = ProcessAddress(csgo, client_panorama, plzOffsets, 5);
		


		while (true)
		{
			printf("\rPlayer: %f %f %f ", playerX.read<float>(), playerY.read<float>(), playerZ.read<float>());
			Sleep(500);
		}

	detach(csgo);
	getchar();
	return 0;
}

