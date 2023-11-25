#include <windows.h>
#include <TlHelp32.h>
#include "proxyproc.h"
#include "offsets.h"


uintptr_t get_module_base(DWORD pid, const char* module)
{
	uintptr_t modBaseAddr = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	if (hSnap != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 modEntry;
		modEntry.dwSize = sizeof(modEntry);
		if (Module32First(hSnap, &modEntry))
		{
			do
			{
				if (!_stricmp(modEntry.szModule, module))
				{
					modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
					break;
				}
			} while (Module32Next(hSnap, &modEntry));
		}
	}
	CloseHandle(hSnap);
	return modBaseAddr;
}

void main()
{
	{	// wait for cstrike.
		uintptr_t cs2pid, cs2client{ 0 };

		cs2pid = proxyproc::get_process_id("cs2.exe");
		cs2client = get_module_base(cs2pid, "client.dll");


		std::cout << "pid: " << cs2pid << std::endl;
		std::cout << "client: " << cs2client << std::endl;

		proxyproc::create_handle_in_proxy("cs2.exe");

		while (1)
		{
			if (GetAsyncKeyState(VK_DELETE) & 0x01)
			{
				proxyproc::cleanup();
				break;
			}

			if (!GetAsyncKeyState(VK_XBUTTON2) & 0x01)
				continue;

			uintptr_t localplayer = proxyproc::read_virtual_memory<uintptr_t>(cs2client + client_dll::dwLocalPlayerPawn);
			int localteam = proxyproc::read_virtual_memory<int>(localplayer + player::m_iTeamNum);

			int entity_id = proxyproc::read_virtual_memory<int>(localplayer + player::m_iIDEntIndex);

			if (entity_id > 0)
			{
				uintptr_t entity_list = proxyproc::read_virtual_memory<uintptr_t>(cs2client + client_dll::dwEntityList);

				uintptr_t ent_entry = proxyproc::read_virtual_memory<uintptr_t>(entity_list + 0x8 * (entity_id >> 9) + 0x10);
				uintptr_t entity = proxyproc::read_virtual_memory<uintptr_t>(ent_entry + 120 * (entity_id & 0x1FF));

				int team = proxyproc::read_virtual_memory<int>(entity + player::m_iTeamNum);

				if (team != localteam)
				{
					mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
					Sleep(10);
					mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
				}
			}

			Sleep(1);
		}

		ExitProcess(0);
	}
}