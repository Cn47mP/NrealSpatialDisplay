#include "AppConfig.h"
#include "../utils/Log.h"
#include <fstream>
#include <windows.h>

void AppConfig::CreateDefaults(AppConfig& cfg) {
    cfg.version = 1;
    cfg.imu.enabled = true;
    cfg.imu.smoothing = 0.15f;
    cfg.imu.deadzoneDeg = 0.5f;
    cfg.imu.autoDriftCompensation = false;

    cfg.display.nrealName = "light";
    cfg.display.virtualCount = 3;
    cfg.display.virtualWidth = 1920;
    cfg.display.virtualHeight = 1080;

    cfg.hotkeys.entries = {
        {"preset_single", "Single", 0x31, true, true, false},
        {"preset_dual", "Dual", 0x32, true, true, false},
        {"preset_triple", "Triple", 0x33, true, true, false},
        {"toggle_imu", "ToggleIMU", 0x49, true, true, false},
        {"reset_view", "ResetView", 0x52, true, true, false},
        {"quit", "Quit", 0x51, true, true, false},
    };

    cfg.render.vsync = true;
    cfg.render.showBorders = true;
    cfg.render.borderColor[0] = 0.3f; cfg.render.borderColor[1] = 0.3f; cfg.render.borderColor[2] = 0.3f;
    cfg.render.borderWidth = 0.005f;
    cfg.render.bgColor[0] = 0.02f; cfg.render.bgColor[1] = 0.02f; cfg.render.bgColor[2] = 0.04f;

    cfg.hud.enabled = true;
    cfg.hud.hotkeyVk = VK_F1;
    cfg.hud.showFps = true;
    cfg.hud.showImu = true;

    cfg.layout = LayoutEngine::CreatePreset("triple", {0, 1, 2});
}

AppConfig AppConfig::Load(const std::string& path) {
    AppConfig cfg;
    CreateDefaults(cfg);

    if (path.empty()) return cfg;

    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_WARN("Failed to open config: %s", path.c_str());
        return cfg;
    }

    try {
        json j;
        file >> j;
        if (j.contains("version")) cfg.version = j["version"];
        if (j.contains("imu")) {
            auto& imu = j["imu"];
            cfg.imu.enabled = imu.value("enabled", cfg.imu.enabled);
            cfg.imu.smoothing = imu.value("smoothing", cfg.imu.smoothing);
            cfg.imu.autoDriftCompensation = imu.value("autoDriftCompensation", cfg.imu.autoDriftCompensation);
        }
        if (j.contains("display")) {
            auto& d = j["display"];
            cfg.display.nrealName = d.value("nrealName", cfg.display.nrealName);
            cfg.display.virtualCount = d.value("virtualCount", cfg.display.virtualCount);
        }
        if (j.contains("hotkeys")) {
            const json& hotkeysNode = j["hotkeys"];
            // 支持两种格式: 直接数组 或 {"entries": [...]}
            const json* arr = nullptr;
            if (hotkeysNode.is_array()) {
                arr = &hotkeysNode;
            } else if (hotkeysNode.is_object() && hotkeysNode.contains("entries") && hotkeysNode["entries"].is_array()) {
                arr = &hotkeysNode["entries"];
            }
            if (arr) {
                cfg.hotkeys.entries.clear();
                for (const auto& hk : *arr) {
                    HotkeyEntry entry;
                    entry.id = hk.value("id", "");
                    entry.name = hk.value("name", "");
                    entry.key = hk.value("key", hk.value("vk", 0));
                    entry.ctrl = hk.value("ctrl", true);
                    entry.alt = hk.value("alt", true);
                    entry.shift = hk.value("shift", false);
                    if (!entry.id.empty()) {
                        cfg.hotkeys.entries.push_back(entry);
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to parse config: %s", e.what());
    }

    return cfg;
}

void AppConfig::Save(const std::string& path) const {
    if (path.empty()) return;

    std::ofstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Failed to save config: %s", path.c_str());
        return;
    }

    try {
        json j;
        j["version"] = version;
        j["imu"] = {
            {"enabled", imu.enabled},
            {"smoothing", imu.smoothing},
            {"autoDriftCompensation", imu.autoDriftCompensation}
        };
        j["display"] = {
            {"nrealName", display.nrealName},
            {"virtualCount", display.virtualCount}
        };
        j["render"] = {
            {"vsync", render.vsync},
            {"showBorders", render.showBorders}
        };
        j["hud"] = {
            {"enabled", hud.enabled}
        };
        {
            json hkArr = json::array();
            for (const auto& entry : hotkeys.entries) {
                hkArr.push_back({
                    {"id", entry.id},
                    {"name", entry.name},
                    {"key", entry.key},
                    {"ctrl", entry.ctrl},
                    {"alt", entry.alt},
                    {"shift", entry.shift}
                });
            }
            j["hotkeys"] = {{"entries", hkArr}};
        }
        file << j.dump(2);
        LOG_INFO("Config saved: %s", path.c_str());
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to save config: %s", e.what());
    }
}