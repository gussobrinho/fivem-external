#include "Overlay.h"
#include "fontawesome.h"

ID3D11Device* g_pd3dDevice = NULL;
ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
IDXGISwapChain* g_pSwapChain = NULL;
ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
bool CreateDeviceD3D(HWND hWnd);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool Overlay::InitOverlay(const char* targetName, InitMode init_mode)
{
    if (init_mode != WINDOW_CLASS)
    {
        switch (init_mode)
        {
        case WINDOW_TITLE: {
            char class_name[128]{};
            GetClassName(FindWindow(nullptr, targetName), class_name, sizeof(class_name));
            m_CTargetClass = class_name;
        }    break;
        case PROCESS_NAME: {
            char class_name[128]{};
            GetClassName(GetWindowByProcessName(targetName), class_name, sizeof(class_name));
            m_CTargetClass = class_name;
        }    break;
        default:
            return false;
            break;
        }
    }
    else if (init_mode == WINDOW_CLASS)
    {
        m_CTargetClass = targetName;
    }

    // Create Overlay
    wc = { sizeof(WNDCLASSEXA), 0, WndProc, 0, 0, NULL, NULL, NULL, NULL, m_CTitle, m_CClass, NULL };
    RegisterClassExA(&wc);
    m_hWnd = CreateWindowExA(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        wc.lpszClassName, wc.lpszMenuName, WS_POPUP | WS_VISIBLE,
        100, 100, 1280, 720, NULL, NULL, wc.hInstance, NULL);
    SetLayeredWindowAttributes(m_hWnd, RGB(0, 0, 0), 255, LWA_ALPHA);
    MARGINS margin = { -1 };
    DwmExtendFrameIntoClientArea(m_hWnd, &margin);

    if (!CreateDeviceD3D(m_hWnd))
    {
        CleanupDeviceD3D();
        UnregisterClassA(wc.lpszClassName, wc.hInstance);
        return false;
    }

    ShowWindow(m_hWnd, SW_SHOWDEFAULT);
    UpdateWindow(m_hWnd);

    // -------- IMGUI INITIALIZATION --------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    // -------- LOAD FONTS --------
    LoadFont("C:\\Windows\\Fonts\\tahomabd.ttf", 15.0f);

    static const ImWchar fa_ranges[] = { 0xf000, 0xf8ff, 0 };
    ImFontConfig cfg;
    cfg.MergeMode = true;
    cfg.GlyphOffset = ImVec2(0, 2);
    cfg.GlyphMinAdvanceX = 14.f;
    cfg.FontDataOwnedByAtlas = false; // FA_DATA is a static array, don't let ImGui free it
    io.Fonts->AddFontFromMemoryTTF((void*)FA_DATA, (int)FA_SIZE, 14.f, &cfg, fa_ranges);

    // -------- BACKEND INIT --------
    ImGui_ImplWin32_Init(m_hWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    std::cout << "Initialized Overlay. \n";

    return true;
}

// -------- FONT LOADING (simple version) --------
bool Overlay::LoadFont(const char* fontPath, float fontSize)
{
    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath, fontSize);
    if (!font)
        return false;
    return true;
}

void Overlay::OverlayManager()
{
    HWND CheckHwnd = FindWindowA(m_CTargetClass.c_str(), nullptr);
    if (!CheckHwnd) {
        g.process_active = false;
        return;
    }

    DWORD Flag = NULL;
    GetWindowDisplayAffinity(m_hWnd, &Flag);
    if (g.StreamProof && Flag == WDA_NONE)
        SetWindowDisplayAffinity(m_hWnd, WDA_EXCLUDEFROMCAPTURE);
    else if (!g.StreamProof && Flag == WDA_EXCLUDEFROMCAPTURE)
        SetWindowDisplayAffinity(m_hWnd, WDA_NONE);

    LONG TmpLong = GetWindowLong(m_hWnd, GWL_EXSTYLE);
    if (g.ShowMenu && MenuFlags != TmpLong)
        SetWindowLong(m_hWnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST);
    else if (!g.ShowMenu && DefaultFlags != TmpLong)
        SetWindowLong(m_hWnd, GWL_EXSTYLE, WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST);

    static bool menu_key = false;
    if (IsKeyDown(g.MenuKey) && !menu_key)
    {
        g.ShowMenu = !g.ShowMenu;
        menu_key = true;
    }
    else if (!IsKeyDown(g.MenuKey) && menu_key)
    {
        menu_key = false;
    }

    RECT TmpRect{};
    POINT TmpPoint{};
    GetClientRect(CheckHwnd, &TmpRect);
    ClientToScreen(CheckHwnd, &TmpPoint);

    if (TmpRect.left != g.GameRect.left || TmpRect.bottom != g.GameRect.bottom ||
        TmpRect.top != g.GameRect.top || TmpRect.right != g.GameRect.right ||
        TmpPoint.x != g.GamePoint.x || TmpPoint.y != g.GamePoint.y)
    {
        g.GameRect = TmpRect;
        g.GamePoint = TmpPoint;
        SetWindowPos(m_hWnd, nullptr, TmpPoint.x, TmpPoint.y,
            g.GameRect.right, g.GameRect.bottom, SWP_NOREDRAW);
    }
}

void Overlay::OverlayLoop()
{
    while (g.process_active)
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        OverlayManager();
        OverlayUserFunction();

        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.f, 0.f, 0.f, 0.f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
    }
}

void Overlay::DestroyOverlay()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(m_hWnd);
    UnregisterClassA(wc.lpszClassName, wc.hInstance);
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
        &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_CLOSE:
        PostQuitMessage(0);
        g.process_active = false;
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        g.process_active = false;
        return 0;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

HWND Overlay::GetWindowByProcessName(const std::string processName)
{
    PROCESSENTRY32 entry{};
    entry.dwSize = sizeof(PROCESSENTRY32);
    const auto snapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    while (Process32Next(snapShot, &entry))
    {
        if (!processName.compare(entry.szExeFile))
            break;
    }

    CloseHandle(snapShot);

    HWND hwnd = GetTopWindow(NULL);
    do {
        if (GetWindowLong(hwnd, 8) != 0 || !IsWindowVisible(hwnd))
            continue;
        DWORD ProcessID;
        GetWindowThreadProcessId(hwnd, &ProcessID);
        if (entry.th32ProcessID == ProcessID)
            return hwnd;
    } while ((hwnd = GetNextWindow(hwnd, GW_HWNDNEXT)) != NULL);

    return NULL;
}