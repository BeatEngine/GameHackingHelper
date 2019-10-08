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

#define posf(X) sqrt((X)*(X))


void aimToNearest(float playerX, float playerY, float playerZ, float* entX, float* entY, float* entZ, int entities, float* yaw, float* pitch, int mode)
{
	float dist;
	int n = 0;
	float nd = 2000;

	float yawc = 0;
	float pitchc = 0;

	float angbefy = 9999;
	float angbefp = 9999;

	float preyaw = *yaw;
	float prepitch = *pitch;




	for (int i = 0; i < entities; i++)
	{
		dist = sqrt(pow2(entX[i] - playerX) + pow2(entY[i] - playerY) + pow2(entZ[i] - playerZ));



	
		n = i;

		float x = entX[n] - playerX;
		float y = entY[n] - playerY -7;
		float z = entZ[n] - playerZ;
		yawc = 0;
		pitchc = 0;


		yawc = acosf(x / sqrtf(x*x + y * y));
		pitchc = asinf(z / dist);

		yawc = yawc / 3.141592 * 180;
		pitchc = pitchc / 3.141592 * 180;


		if (y < 0)
		{
			yawc = -yawc;
		}

		pitchc = -pitchc;

		if (dist<1999&&dist>2)
		{
			if (posf(preyaw - yawc) < angbefy && posf(preyaw - yawc) < 60)
			{
				if (posf(prepitch - pitchc) < angbefp)
				{
					angbefy = posf(preyaw - yawc);
					angbefp = posf(prepitch - pitchc);
					nd = dist;
					*yaw = yawc;
					*pitch = pitchc;
				}
			}
		}

	}
	

}

int main()
{
	int pid;
	HANDLE csgo = attach((char*)"csgo.exe", &pid);

		printf("csgo.exe: 0x%" PRIXXX "\n", csgo);

		uintptr_t mainModule = getmoduleaddress(csgo, (char*)"csgo.exe");
		printf("csgo.exe: 0x%" PRIXXX "\n", mainModule);
		//uintptr_t client_panorama = getmoduleaddress(csgo, (char*)"client_panorama.dll");
		uintptr_t engine = getmoduleaddress(csgo, (char*)"engine.dll");
		uintptr_t server = getmoduleaddress(csgo, (char*)"server.dll");
		printf("Loading Baseaddresses by patterns wait a minute!\n");
		ProcessAddress PlayerServerBase = ProcessAddress(csgo);
		unsigned char pservpatt[] = {0xD0, 0x8E, 0x2D, 0x4E, 0xFA, 0x76};
		if (!PlayerServerBase.loadByModulePattern(csgo, server, pservpatt, 6))
		{
			printf("Could not find PlayerServerBase!");
			return 0;
		}
		ProcessAddress PlayerEngineBase = ProcessAddress(csgo);
		unsigned char pengpatt[] = { 0xD0, 0x53, 0x74, 0x11, 0x90, 0xE8, 0x85, 0x11, 0x01};
		if (!PlayerEngineBase.loadByModulePattern(csgo, engine, pengpatt, 9))
		{
			printf("Could not find PlayerEngineBase!");
			return 0;
		}
		ProcessAddress EntityListBase = ProcessAddress(csgo);
		unsigned char elstpatt[] = { 0x74, 0x72, 0x7D, 0x68, 0x00, 0x00};
		if (!EntityListBase.loadByModulePattern(csgo, server, elstpatt, 6))
		{
			printf("Could not find EntityListBase!");
			return 0;
		}



		//printf("client_panorama.dll: 0x%" PRIXXX "\n", client_panorama);
		printf("engine.dll: 0x%" PRIXXX "\n", engine);
		printf("server.dll: 0x%" PRIXXX "\n", server);

		printf("PlayerServerBase: 0x%" PRIXXX "\n", PlayerServerBase.getAddress());
		printf("PlayerEngineBase: 0x%" PRIXXX "\n", PlayerEngineBase.getAddress());
		printf("EntityListBase: 0x%" PRIXXX "\n", EntityListBase.getAddress());

		//ProcessAddress playerXtmp = ProcessAddress(csgo,0x3CF3D4C);
		
		while (true)
		{
			ProcessAddress playerX = ProcessAddress(csgo, PlayerServerBase.read<uintptr_t>() + 0x1DC);
			ProcessAddress playerY = ProcessAddress(csgo, PlayerServerBase.read<uintptr_t>() + 0x1E0);
			ProcessAddress playerZ = ProcessAddress(csgo, PlayerServerBase.read<uintptr_t>() + 0x1E4);
			ProcessAddress playerPitch = ProcessAddress(csgo, PlayerEngineBase.read<uintptr_t>() + 0x4D88);
			ProcessAddress playerYaw = ProcessAddress(csgo, PlayerEngineBase.read<uintptr_t>() + 0x4D8C);

			ProcessAddress firstEntityX = ProcessAddress(csgo, EntityListBase.getAddress() + 0x33C);

			float entX[10] = { 0 };
			float entY[10] = { 0 };
			float entZ[10] = { 0 };
			int i = 0;
			float px;
			float py;
			float pz;

			float yaw;
			float pitch;
			
			ProcessAddress EntityTmp = ProcessAddress(csgo);
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
			//printf("First entity: 0x%" PRIXXX "", firstEntityX.getAddress());
			
			/*for (i = 0; i < 10; i++)
			{
				printf("(%f %f %f)", entX[i], entY[i], entZ[i]);
				if (i % 3 == 0)
				{
					printf("\n");
				}
			}
			printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n");*/


			if (GetAsyncKeyState(VK_MBUTTON))
			{
				aimToNearest(px, py, pz, entX, entY, entZ, 10, &yaw, &pitch,1);
				printf(" calc: (%f %f)      ", yaw, pitch);
				playerPitch.write<float>(pitch);
				playerYaw.write<float>(yaw);
				while (GetAsyncKeyState(VK_MBUTTON))
				{
					Sleep(1);
				}
			}
			if (GetAsyncKeyState(VK_LBUTTON))
			{
				aimToNearest(px, py, pz, entX, entY, entZ, 10, &yaw, &pitch,1);
				printf(" calc: (%f %f)      ", yaw, pitch);
				playerPitch.write<float>(pitch);
				playerYaw.write<float>(yaw);
				Sleep(1);
			}
			Sleep(10);
		}

	detach(csgo);
	getchar();
	return 0;
}

