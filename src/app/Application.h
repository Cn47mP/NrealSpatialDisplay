#pragma once
#include <windows.h>
#include <memory>
#include <vector>
#include <map>
#include <atomic>
#include <chrono>
#include "../imu/AirIMU.h"
#include "../imu/NrealLightIMU.h"
#include "../render/Camera.h"
#include "../render/D3DRenderer.h"
#include "../capture/CaptureManager.h"
#include "../layout/LayoutPreset.h"
#include "../layout/LayoutEngine.h"
#include "../gui/SettingsGUI.h"
#include "../display/DisplaySwitcher.h"
#include "../network/UnityStream.h"
#include "../utils/Log.h"
#include "AppConfig.h"

struct NrealDisplayInfo {
    bool found = false;
    std::wstring deviceName;
    UINT width = 1920;
    UINT height = 1080;
    int posX = 0;
    int posY = 0;
};

enum class ConnectionState {
    Connected,
    Disconnected,
    Reconnecting
};

class Application {
public:
    static Application* s_instance;

    Application();
    ~Application();

    bool Init(bool noPopup = false);
    void Shutdown();
    void Tick();
    void HandleAction(const std::string& action);
    void SetPaused(bool paused);
    bool TryReconnect();

    static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK PreviewWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    bool CreateMainWindow(bool noPopup = false);
    bool CreatePreviewWindow();
    void TakeScreenshot();

    void CreateTrayIcon();
    void DestroyTrayIcon();
    void ShowTrayMenu();
    void HandleTrayCommand(int id);
    void SwitchLayout(const std::string& name);
    void UpdateTrayTooltip();

    bool m_renderingPaused = false;

    HWND m_hwnd = nullptr;
    HWND m_previewHwnd = nullptr;
    bool m_previewEnabled = false;
    UINT m_previewW = 960;
    UINT m_previewH = 540;

    AppConfig m_config;
    std::unique_ptr<D3DRenderer> m_renderer;
    std::unique_ptr<CaptureManager> m_captureMgr;
    std::unique_ptr<AirIMU> m_airIMU;
    std::unique_ptr<NrealLightIMU> m_lightIMU;
    std::unique_ptr<SettingsGUI> m_gui;
    std::unique_ptr<DisplaySwitcher> m_displaySwitcher;
    std::unique_ptr<UnityStream> m_unityStream;
    Camera m_camera;

    NOTIFYICONDATAW m_trayIcon = {};
    bool m_trayCreated = false;

    std::map<int, std::string> m_vkToAction;
    std::map<std::string, int> m_actionToId;
    std::vector<HotkeyEntry> m_hotkeys;

    void RegisterHotkeys();
    void UnregisterHotkeys();
    int GetHotkeyId(const std::string& action) const;
    bool HasConflict(int vk, bool ctrl, bool alt, bool shift, const std::string& excludeAction) const;
    std::chrono::high_resolution_clock::time_point m_lastTick;
    float m_fps = 60.0f;
    int m_frameCount = 0;

    ConnectionState m_connState = ConnectionState::Connected;
    NrealDisplayInfo m_nrealDisplay;
    std::chrono::steady_clock::time_point m_lastImuTime;
    std::chrono::steady_clock::time_point m_lastReconnectAttempt;
    std::chrono::steady_clock::time_point m_lastTooltipUpdate;
    static constexpr int RECONNECT_INTERVAL_MS = 3000;
    static constexpr int TOOLTIP_UPDATE_INTERVAL_MS = 5000;
};