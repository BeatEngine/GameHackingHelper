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

#define pow2(X) (X)*(X)


void aimToNearest(float playerX, float playerY, float playerZ, float* entX, float* entY, float* entZ, int entities, float* yaw, float* pitch)
{
	float dist;
	int n = 0;
	float nd = 99999999999;
	for (int i = 0; i < entities; i++)
	{
		dist = sqrt(pow2(entX[i] - playerX) + pow2(entY[i] - playerY) + pow2(entZ[i] - playerZ));
		if (dist > 0.1)
		{
			if (dist < nd)
			{
				nd = dist;
				n = i;
			}
		}
	}

	float x = entX[n] - playerX;
	float y = entX[n] - playerX;
	float z = entX[n] - playerX;
	float yawc;
	float pitchc;

	yawc = acosf(y / (sqrt(pow2(x) + pow2(y))));
	pitchc = acosf(y / (pow2(z)));

	yawc = yawc / 3.141592 * 180;
	pitchc = pitchc / 3.141592 * 180;

	if (yawc > 180)
	{
		yawc -= 180;
		yawc = -yawc;
	}

	if (pitchc > 90)
	{
		pitchc -= 90;
		pitchc = -pitchc;
	}
	*yaw = yawc;
	*pitch = pitchc;

}

int main()
{
	int pid;
	HANDLE csgo = attach((char*)"csgo.exe", &pid);

		printf("csgo.exe: 0x%" PRIXXX "\n", csgo);

		uintptr_t mainModule = getmoduleaddress(csgo, (char*)"csgo.exe");
		printf("csgo.exe: 0x%" PRIXXX "\n", mainModule);
		uintptr_t client_panorama = getmoduleaddress(csgo, (char*)"client_panorama.dll");
		uintptr_t engine = getmoduleaddress(csgo, (char*)"engine.dll");
		uintptr_t server = getmoduleaddress(csgo, (char*)"server.dll");

		printf("client_panorama.dll: 0x%" PRIXXX "\n", client_panorama);
		printf("engine.dll: 0x%" PRIXXX "\n", engine);
		printf("server.dll: 0x%" PRIXXX "\n", server);
		//ProcessAddress playerXtmp = ProcessAddress(csgo,0x3CF3D4C);
		uintptr_t plxOffsets[5] = { 0xCF7A4C, 0x24, 0x38, 0x568, 0x14C };
		ProcessAddress playerX = ProcessAddress(csgo, client_panorama, plxOffsets, 5);
		uintptr_t plyOffsets[5] = { 0xCF7A4C, 0x24, 0x38, 0x568, 0x150 };
		ProcessAddress playerY = ProcessAddress(csgo, client_panorama, plyOffsets, 5);
		uintptr_t plzOffsets[5] = { 0xCF7A4C, 0x24, 0x38, 0x568, 0x154 };
		ProcessAddress playerZ = ProcessAddress(csgo, client_panorama, plzOffsets, 5);
		
		uintptr_t pitchOffsets[2] = { 0x590D8C, 0x4D88 };
		ProcessAddress playerPitch = ProcessAddress(csgo, engine, pitchOffsets ,2);
		ProcessAddress playerYaw = ProcessAddress(csgo, playerPitch.getAddress()+0x4);

		uintptr_t entitybaseoffset[1] = { 0xA7BF34 };
		ProcessAddress firstEntityX = ProcessAddress(csgo, server + 0xA7BF34);

		float entX[10] = { 0 };
		float entY[10] = { 0 };
		float entZ[10] = { 0 };
		int i = 0;
		float px;
		float py;
		float pz;

		float yaw;
		float pitch;

		ProcessAddress EntityTmp = ProcessAddress(csgo, firstEntityX.getAddress() + i * 0x24);
		while (true)
		{
			for (i = 0; i < 10; i++)
			{
				EntityTmp.load(csgo, firstEntityX.getAddress() + i * 0x24);
				entX[i] = EntityTmp.read<float>();
				EntityTmp.load(csgo, firstEntityX.getAddress() + i * 0x24 + 0x4);
				entY[i] = EntityTmp.read<float>();
				EntityTmp.load(csgo, firstEntityX.getAddress() + i * 0x24 + 0x8);
				entZ[i] = EntityTmp.read<float>();
			}

			px = playerX.read<float>();
			py = playerY.read<float>();
			pz = playerZ.read<float>();
			yaw = playerYaw.read<float>();
			pitch = playerPitch.read<float>();
			printf("\rPlayer: (%f %f) (%f %f %f) ",yaw, pitch, px, py, pz);
			
			/*for (i = 0; i < 10; i++)
			{
				printf("(%f %f %f)", entX[i], entY[i], entZ[i]);
				if (i % 3 == 0)
				{
					printf("\n");
				}
			}
			printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n");*/


			if (GetAsyncKeyState(0x51))
			{
				aimToNearest(px, py, pz, entX, entY, entZ, 10, &yaw, &pitch);
				playerPitch.write<float>(pitch);
				playerYaw.write<float>(yaw);
				while (GetAsyncKeyState(0x51))
				{
					Sleep(1);
				}
			}
			Sleep(50);
		}

	detach(csgo);
	getchar();
	return 0;
}

