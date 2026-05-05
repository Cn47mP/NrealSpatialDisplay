#pragma once
#include <windows.h>
#include <d3d11.h>
#include <string>
#include <functional>
#include "../app/AppConfig.h"
#include "imgui.h"

using ToggleCallback = std::function<void(bool)>;
using ConfigChangedCallback = std::function<void(const AppConfig&)>;
using LayoutSwitchCallback = std::function<void(const std::string&)>;
using ResetViewCallback = std::function<void()>;
using QuitCallback = std::function<void()>;
using ActionCallback = std::function<void(const std::string&)>;

class SettingsGUI {
public:
    SettingsGUI() = default;
    ~SettingsGUI() { Shutdown(); }

    bool Init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* ctx);
    bool InitPreviewHud(ID3D11Device* dev, ID3D11DeviceContext* ctx, HWND hwnd);
    void Shutdown();
    void NewFrame();
    void Render();
    void RenderPreviewHud();

    void SetConfig(const AppConfig& cfg) { m_config = cfg; }
    AppConfig& GetConfig() { return m_config; }
    void SetEnabled(bool v) { m_enabled = v; }
    bool IsEnabled() const { return m_enabled; }
    void ToggleVisible() { m_windowVisible = !m_windowVisible; }
    bool IsVisible() const { return m_windowVisible; }
    void ToggleHud() { m_hudVisible = !m_hudVisible; }

    void SetImuConnected(bool c) { m_imuConnected = c; }
    void SetImuData(float p, float y, float r) { m_pitch = p; m_yaw = y; m_roll = r; }
    void SetFps(float f) { m_fps = f; }
    void SetFrameLatency(float ms) { m_latency = ms; }
    void SetCaptureCount(int n) { m_captureCount = n; }
    void SetScreenCount(int n) { m_screenCount = n; }
    void SetLayoutName(const std::string& n) { m_layoutName = n; }
    void SetCameraMode(const std::string& mode) { m_cameraMode = mode; }
    void SetBrightness(int b) { m_brightness = b; }
    void SetRenderingPaused(bool p) { m_renderingPaused = p; }

    void OnToggle(ToggleCallback cb) { m_onToggle = std::move(cb); }
    void OnConfigChanged(ConfigChangedCallback cb) { m_onConfigChanged = std::move(cb); }
    void OnLayoutSwitch(LayoutSwitchCallback cb) { m_onLayoutSwitch = std::move(cb); }
    void OnResetView(ResetViewCallback cb) { m_onResetView = std::move(cb); }
    void OnQuit(QuitCallback cb) { m_onQuit = std::move(cb); }
    void OnAction(ActionCallback cb) { m_onAction = std::move(cb); }

    static std::string FormatHotkey(const HotkeyEntry& e);

private:
    void RenderSettingsWindow();
    void RenderHUD();

    bool m_initialized = false;
    bool m_previewInitialized = false;
    bool m_enabled = true;
    bool m_windowVisible = false;
    bool m_hudVisible = true;
    bool m_renderingPaused = false;
    AppConfig m_config;

    HWND m_hwnd = nullptr;
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_deviceContext = nullptr;

    // 预览窗口独立的 ImGui 上下文
    ImGuiContext* m_mainCtx = nullptr;
    ImGuiContext* m_previewCtx = nullptr;
    HWND m_previewHwnd = nullptr;
    ID3D11Device* m_previewDevice = nullptr;
    ID3D11DeviceContext* m_previewDeviceCtx = nullptr;

    bool m_imuConnected = false;
    float m_pitch = 0.0f, m_yaw = 0.0f, m_roll = 0.0f;
    float m_fps = 60.0f;
    float m_latency = 16.0f;
    int m_captureCount = 0;
    int m_screenCount = 0;
    int m_brightness = 50;
    std::string m_layoutName = "triple";
    std::string m_cameraMode = "World Lock";

    ToggleCallback m_onToggle;
    ConfigChangedCallback m_onConfigChanged;
    LayoutSwitchCallback m_onLayoutSwitch;
    ResetViewCallback m_onResetView;
    QuitCallback m_onQuit;
    ActionCallback m_onAction;
};