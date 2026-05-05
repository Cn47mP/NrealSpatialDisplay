// NrealSpatialDisplay - AR 多虚拟屏空间锚定系统
// WinMain 入口点

#include "app/Application.h"
#include "utils/Log.h"

#include <Windows.h>
#include <cstdlib>
#include <cstdio>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // 打开调试控制台，用于输出日志
    AllocConsole();
    FILE* fp = nullptr;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);

    Application app;
    bool noPopup = (strstr(lpCmdLine, "--no-popup") != nullptr);

    // 初始化应用：D3D、捕获、IMU、GUI 等全部子系统
    if (!app.Init(noPopup))
    {
        MessageBoxW(nullptr, L"应用初始化失败，请检查日志文件。", L"NrealSpatialDisplay", MB_OK | MB_ICONERROR);
        return 1;
    }

    // 主消息循环：空闲时执行渲染 Tick
    MSG msg = {};
    while (true)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                goto quit;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        app.Tick();
    }

quit:
    app.Shutdown();

    if (fp) fclose(fp);
    FreeConsole();

    return static_cast<int>(msg.wParam);
}
