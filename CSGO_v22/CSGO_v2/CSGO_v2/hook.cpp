#include "hook.h"

bool hooks::Setup()
{
	if (MH_Initialize())
		return 0;//throw std::runtime_error("Unable to initialize Hooks");

	// D3D9 Hook
	if (MH_CreateHook(
		VirtualFunction(gui::device, 42),
		&hkEndScene,
		reinterpret_cast<void**>(&oEndScene)
	)) return 0;//throw std::runtime_error("Unable to hook Reset");

	// Reset Hook
	if (MH_CreateHook(
		VirtualFunction(gui::device, 16),
		&hkReset,
		reinterpret_cast<void**>(&oReset)
	)) return 0;// throw std::runtime_error("Unable to hook Reset");

	// CreateMove Hook
	g_ClientMode = **reinterpret_cast<void***>((*reinterpret_cast<unsigned int**>(globals::g_interfaces.BaseClient))[10] + 5);
	
	if (MH_CreateHook(
		(*static_cast<void***>(g_ClientMode))[24],
		&hkCreateMove,
		reinterpret_cast<void**>(&oCreateMove)
	)) return 0;//throw std::runtime_error("Unable to hook CreateMove");

	// Actually hook
	if (MH_EnableHook(MH_ALL_HOOKS))
		return 0;//throw std::runtime_error("Unable to enable hooks");

	gui::DestroyDirectX();
	return 1;
}

void hooks::Destroy() noexcept
{
	// restore hooks
	MH_DisableHook(MH_ALL_HOOKS);
	MH_RemoveHook(MH_ALL_HOOKS);

	// uninit minhook
	MH_Uninitialize();
}

long __stdcall hooks::hkEndScene(LPDIRECT3DDEVICE9 pDevice) noexcept
{
	if (cfg.settings.StreamProof)
	{
		// Stream proof using steam's Game Overlay
		static uintptr_t GameOverlayRetAdr = 0;

		if (!GameOverlayRetAdr)
		{
			MEMORY_BASIC_INFORMATION info;
			VirtualQuery(_ReturnAddress(), &info, sizeof(MEMORY_BASIC_INFORMATION));

			char mod[MAX_PATH];
			GetModuleFileNameA((HMODULE)info.AllocationBase, mod, MAX_PATH);

			if (strstr(mod, "gameoverlay"))
				GameOverlayRetAdr = (uintptr_t)(_ReturnAddress());
		}

		if (GameOverlayRetAdr != (uintptr_t)(_ReturnAddress()))
			return oEndScene(pDevice);
	}


	if (!gui::init)
		gui::SetupMenu(pDevice);

	gui::Render();

	return oEndScene(pDevice);
}

HRESULT __stdcall hooks::hkReset(IDirect3DDevice9* Device, D3DPRESENT_PARAMETERS* params) noexcept
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	const auto result = oReset(Device, Device, params);
	ImGui_ImplDX9_CreateDeviceObjects();
	return result;
}

bool __stdcall hooks::hkCreateMove(float frametime, CUserCmd* cmd) noexcept
{
	const bool result = hooks::oCreateMove(hooks::g_ClientMode, frametime, cmd);

	if (!cmd || !cmd->command_number)
		return result;

	if (GetAsyncKeyState(VK_F9))
		cmd->viewangles.x = 0;

	aimbot::Run(cmd);
	misc::BunnyHop(cmd);

	return result;
}