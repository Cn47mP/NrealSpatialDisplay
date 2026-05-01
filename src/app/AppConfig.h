
#pragma once
#include <string>
#include <vector>
#include "json.hpp"
#include "../layout/LayoutEngine.h"

struct HotkeyEntry {
    std::string id;
    std::string name;
    int key = 0;
    bool ctrl = true;
    bool alt = true;
    bool shift = false;
};

struct HotkeyConfig {
    std::vector<HotkeyEntry> entries;
};

struct ImuConfig {
    bool enabled = true;
    float smoothing = 0.15f;
    float deadzoneDeg = 0.5f;
    bool autoDriftCompensation = false;
};

struct DisplayConfig {
    std::string nrealName = "light";
    int virtualCount = 3;
    int virtualWidth = 1920;
    int virtualHeight = 1080;
};

struct RenderConfig {
    bool vsync = true;
    bool showBorders = true;
    float borderColor[3] = {0.3f, 0.3f, 0.3f};
    float borderWidth = 0.005f;
    float bgColor[3] = {0.02f, 0.02f, 0.04f};
    bool sharpenEnabled = false;
    float sharpenStrength = 0.5f;
};

struct HudConfig {
    bool enabled = true;
    int hotkeyVk = 116;
    bool showFps = true;
    bool showLatency = true;
    bool showImu = true;
    bool showCapture = true;
    bool showLayout = true;
};

struct AppConfig {
    int version = 1;
    ImuConfig imu;
    DisplayConfig display;
    LayoutConfig layout;
    RenderConfig render;
    HudConfig hud;
    HotkeyConfig hotkeys;

    static AppConfig Load(const std::string& path);
    void Save(const std::string& path) const;
    static void CreateDefaults(AppConfig& cfg);
};
