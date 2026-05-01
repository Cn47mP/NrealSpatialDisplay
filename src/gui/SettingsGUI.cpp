#include "SettingsGUI.h"
#include "../utils/Log.h"
#include "imgui.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include <sstream>

static bool s_imguiInitialized = false;

bool SettingsGUI::Init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* ctx) {
    m_hwnd = hwnd;
    m_device = device;
    m_deviceContext = ctx;

    if (!hwnd || !device || !ctx) {
        LOG_WARN("SettingsGUI: Invalid params, headless mode");
        m_initialized = false;
        return false;
    }

    if (!s_imguiInitialized) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        if (!ImGui_ImplWin32_Init(hwnd)) {
            LOG_ERROR("SettingsGUI: Win32 init failed");
        }
        if (!ImGui_ImplDX11_Init(device, ctx)) {
            LOG_ERROR("SettingsGUI: DX11 init failed");
        }
        s_imguiInitialized = true;
    }

    m_initialized = true;
    LOG_INFO("SettingsGUI: ImGui initialized");
    return true;
}

bool SettingsGUI::InitPreviewHud(ID3D11Device* dev, ID3D11DeviceContext* ctx, HWND hwnd) {
    return Init(hwnd, dev, ctx);
}

void SettingsGUI::Shutdown() {
    if (m_initialized) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        s_imguiInitialized = false;
    }
    m_initialized = false;
    LOG_INFO("SettingsGUI: Shutdown");
}

void SettingsGUI::NewFrame() {
    if (!m_initialized || !m_enabled) return;
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void SettingsGUI::Render() {
    if (!m_initialized || !m_enabled) return;

    if (m_hudVisible) {
        RenderHUD();
    }

    if (m_windowVisible) {
        RenderSettingsWindow();
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void SettingsGUI::RenderHUD() {
    if (!ImGui::Begin("Status", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus)) {
        ImGui::End();
        return;
    }

    ImGui::Text("FPS: %.1f", m_fps);
    ImGui::Text("IMU: %s", m_imuConnected ? "OK" : "NO");
    if (m_imuConnected) {
        ImGui::Text("P:%.1f Y:%.1f R:%.1f", m_pitch, m_yaw, m_roll);
    }
    ImGui::Text("Screens: %d", m_screenCount);
    ImGui::Text("Layout: %s", m_layoutName.c_str());

    ImGui::End();
}

void SettingsGUI::RenderSettingsWindow() {
    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Settings", &m_windowVisible)) {
        ImGui::End();
        return;
    }

    ImGui::Text("NrealSpatialDisplay");
    ImGui::Separator();

    if (ImGui::TreeNode("Layout Presets")) {
        const char* presets[] = {"single", "dual", "triple", "five-arc", "curved-ultrawide"};
        const char* labels[] = {"Single", "Dual", "Triple", "Five-ArC", "Curved"};

        for (int i = 0; i < 5; i++) {
            if (ImGui::Selectable(labels[i], m_config.layout.name == presets[i])) {
                m_config.layout.name = presets[i];
                if (m_onLayoutSwitch) {
                    m_onLayoutSwitch(presets[i]);
                }
            }
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("IMU")) {
        ImGui::Checkbox("Enable IMU", &m_config.imu.enabled);
        ImGui::Text("Status: %s", m_imuConnected ? "Connected" : "Disconnected");
        ImGui::SliderFloat("Smoothing", &m_config.imu.smoothing, 0.0f, 0.5f);
        ImGui::SliderFloat("Deadzone", &m_config.imu.deadzoneDeg, 0.0f, 5.0f);
        ImGui::Checkbox("Auto Drift", &m_config.imu.autoDriftCompensation);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Render")) {
        ImGui::Checkbox("VSync", &m_config.render.vsync);
        ImGui::Checkbox("Show Border", &m_config.render.showBorders);
        ImGui::SliderFloat("Border Width", &m_config.render.borderWidth, 0.0f, 0.05f);
        ImGui::ColorEdit3("Border", m_config.render.borderColor);
        ImGui::ColorEdit3("Background", m_config.render.bgColor);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Display")) {
        ImGui::InputInt("Virtual Count", &m_config.display.virtualCount);
        ImGui::InputInt("Width", &m_config.display.virtualWidth);
        ImGui::InputInt("Height", &m_config.display.virtualHeight);
        ImGui::TreePop();
    }

    ImGui::Separator();

    if (m_renderingPaused) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "PAUSED");
    } else {
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "RUNNING");
    }

    if (ImGui::Button("Save Config")) {
        if (m_onConfigChanged) {
            m_onConfigChanged(m_config);
        }
    }

    ImGui::End();
}

void SettingsGUI::RenderPreviewHud() {
    if (!m_hudVisible) return;

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.6f);
    ImGui::Begin("##PreviewHUD", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing);

    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "FPS: %.1f", m_fps);
    ImGui::Text("Latency: %.1f ms", m_latency);
    ImGui::Text("Layout: %s", m_layoutName.c_str());
    ImGui::Text("Mode: %s", m_cameraMode.c_str());

    ImVec4 imuColor = m_imuConnected
        ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f)
        : ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
    ImGui::TextColored(imuColor, "IMU: %s", m_imuConnected ? "OK" : "DISCONNECTED");

    ImGui::Separator();
    ImGui::Text("P:%.1f Y:%.1f R:%.1f", m_pitch, m_yaw, m_roll);
    ImGui::Text("Screens: %d | Brightness: %d%%", m_screenCount, m_brightness);

    if (m_renderingPaused) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "PAUSED");
    }

    ImGui::End();
}

std::string SettingsGUI::FormatHotkey(const HotkeyEntry& e) {
    std::ostringstream oss;
    if (e.ctrl) oss << "Ctrl+";
    if (e.alt) oss << "Alt+";
    if (e.shift) oss << "Shift+";
    if (e.key >= 0x20 && e.key < 0x7F) {
        oss << char(e.key);
    } else {
        oss << "0x" << std::hex << e.key;
    }
    return oss.str();
}