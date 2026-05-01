#include "app/Application.h"
#include "utils/Log.h"
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    char exePath[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, exePath, sizeof(exePath));
    char* slash = strrchr(exePath, '\\');
    if (slash) *slash = '\0';

    std::string logDir = std::string(exePath) + "\\logs";
    Log::Init(logDir);

    LOG_INFO("=== NrealSpatialDisplay starting ===");
    LOG_INFO("Executable: %s", exePath);
    LOG_INFO("Log dir: %s", logDir.c_str());

    Application app;

    if (!app.Init()) {
        LOG_ERROR("Application init failed");
        Log::Shutdown();
        MessageBoxA(nullptr, "Init failed - check logs folder", "Error", MB_OK);
        return 1;
    }

    LOG_INFO("Application initialized, entering main loop");

    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            app.Tick();
        }
    }

    app.Shutdown();
    LOG_INFO("=== NrealSpatialDisplay exiting ===");
    Log::Shutdown();
    return 0;
}