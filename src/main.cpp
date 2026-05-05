// NrealSpatialDisplay - AR 多虚拟屏空间锚定系统
// WinMain 入口点

#include "app/Application.h"
#include "utils/Log.h"

#include <Windows.h>
#include <cstdlib>
#include <cstdio>
#include <DbgHelp.h>

#pragma comment(lib, "dbghelp.lib")

static LONG WINAPI CrashHandler(EXCEPTION_POINTERS* exInfo) {
    LOG_ERROR("=== UNHANDLED EXCEPTION 0x%08X ===", exInfo->ExceptionRecord->ExceptionCode);
    LOG_ERROR("Fault address: 0x%p", exInfo->ExceptionRecord->ExceptionAddress);

    // 尝试输出调用栈
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();
    SymInitialize(process, NULL, TRUE);

    CONTEXT* ctx = exInfo->ContextRecord;
    STACKFRAME64 stack = {};
    stack.AddrPC.Offset = ctx->Rip;
    stack.AddrPC.Mode = AddrModeFlat;
    stack.AddrFrame.Offset = ctx->Rbp;
    stack.AddrFrame.Mode = AddrModeFlat;
    stack.AddrStack.Offset = ctx->Rsp;
    stack.AddrStack.Mode = AddrModeFlat;

    for (int i = 0; i < 20; ++i) {
        if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, thread, &stack,
                         ctx, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
            break;

        char symbolBuffer[sizeof(SYMBOL_INFO) + 256] = {};
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)symbolBuffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = 255;

        DWORD64 disp = 0;
        if (SymFromAddr(process, stack.AddrPC.Offset, &disp, symbol)) {
            LOG_ERROR("  [%d] %s + 0x%llX (0x%llX)", i, symbol->Name, disp, stack.AddrPC.Offset);
        } else {
            LOG_ERROR("  [%d] ??? (0x%llX)", i, stack.AddrPC.Offset);
        }
    }

    LOG_ERROR("=== END STACK TRACE ===");
    Log::Shutdown();
    return EXCEPTION_EXECUTE_HANDLER;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    SetUnhandledExceptionFilter(CrashHandler);

    // 打开调试控制台，用于输出日志
    AllocConsole();
    FILE* fp = nullptr;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);

    Application app;
    bool noPopup = (strstr(lpCmdLine, "--no-popup") != nullptr);
    int displayIndex = -1;
    const char* dispArg = strstr(lpCmdLine, "--display");
    if (dispArg) {
        dispArg += 9; // skip "--display"
        while (*dispArg == ' ') dispArg++;
        displayIndex = atoi(dispArg);
    }

    // 初始化应用：D3D、捕获、IMU、GUI 等全部子系统
    if (!app.Init(noPopup, displayIndex))
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
