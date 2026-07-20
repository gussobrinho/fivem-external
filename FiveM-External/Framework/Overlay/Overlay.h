#pragma once
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_win32.h"
#include "../ImGui/imgui_impl_dx11.h"
#include "../../Globals/Globals.h"
#include "../Memory/Memory.h"
#include <dwmapi.h>
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

class Overlay
{
private:
	// OverlayWindow
	WNDCLASSEXA wc;
	HWND m_hWnd;
	char m_CTitle[32] = "FUCK NAZI-RUSSIA";
	char m_CClass[32] = "wnd-class";

	// OverlayManager
	std::string m_CTargetClass;

	LONG MenuFlags = WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
	LONG DefaultFlags = WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST;

	void OverlayManager();
	void OverlayUserFunction();
	HWND GetWindowByProcessName(const std::string processName);
public:
	bool InitOverlay(const char* targetName, InitMode init_mode);
	void OverlayLoop();
	void DestroyOverlay();

	bool LoadFont(const char* fontPath, float fontSize = 16.0f);
};