#include "DisplaySwitcher.h"
#include "../utils/Log.h"
#include <cstring>

DisplaySwitcher::DisplaySwitcher() {}

DisplaySwitcher::~DisplaySwitcher() {
    if (m_sbsEnabled) {
        Enable2D();
    }
}

std::vector<DisplaySwitcher::DisplayInfo> DisplaySwitcher::EnumerateAllDisplays() {
    std::vector<DisplayInfo> result;

    DISPLAY_DEVICEA dd = {};
    dd.cb = sizeof(dd);

    for (int i = 0; EnumDisplayDevicesA(nullptr, i, &dd, 0); i++) {
        DisplayInfo info;
        info.index = i;
        info.name = dd.DeviceString;
        info.isNreal = (strstr(dd.DeviceString, "nreal") != nullptr) ||
                       (strstr(dd.DeviceString, "NReal") != nullptr) ||
                       (strstr(dd.DeviceString, "NREAL") != nullptr) ||
                       (strstr(dd.DeviceString, "Light") != nullptr);

        if (info.isNreal) {
            DEVMODEA dm = {};
            dm.dmSize = sizeof(dm);
            if (EnumDisplaySettingsA(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm)) {
                info.currentWidth = dm.dmPelsWidth;
                info.currentHeight = dm.dmPelsHeight;
            }

            // Enumerate all modes
            for (int modeIdx = 0; EnumDisplaySettingsA(dd.DeviceName, modeIdx, &dm); modeIdx++) {
                DisplayMode mode;
                mode.width = dm.dmPelsWidth;
                mode.height = dm.dmPelsHeight;
                mode.refresh = dm.dmDisplayFrequency;
                mode.isSBS = (mode.width == 3840 && mode.height == 1080) ||
                             (mode.width == 1920 && mode.height == 1080 && strstr((char*)dm.dmDeviceName, "SBS") != nullptr);
                info.modes.push_back(mode);
            }
        }

        result.push_back(info);
    }

    return result;
}

bool DisplaySwitcher::DetectNrealDisplay() {
    auto displays = EnumerateAllDisplays();

    for (const auto& d : displays) {
        if (d.isNreal) {
            m_nrealInfo = d;
            m_nrealFound = true;
            LOG_INFO("DisplaySwitcher: Found Nreal display '%s' at index %d, %dx%d",
                     d.name.c_str(), d.index, d.currentWidth, d.currentHeight);
            return true;
        }
    }

    m_nrealFound = false;
    LOG_WARN("DisplaySwitcher: Nreal display not found");
    return false;
}

bool DisplaySwitcher::SetDisplayResolution(int width, int height) {
    if (!m_nrealFound) {
        LOG_WARN("DisplaySwitcher: Cannot set resolution, Nreal not found");
        return false;
    }

    DISPLAY_DEVICEA dd = {};
    dd.cb = sizeof(dd);
    if (!EnumDisplayDevicesA(nullptr, m_nrealInfo.index, &dd, 0)) {
        LOG_ERROR("DisplaySwitcher: Failed to get display device");
        return false;
    }

    DEVMODEA dm = {};
    dm.dmSize = sizeof(dm);
    dm.dmPelsWidth = width;
    dm.dmPelsHeight = height;
    dm.dmBitsPerPel = 32;
    dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;

    LONG result = ChangeDisplaySettingsExA(dd.DeviceName, &dm, nullptr, CDS_UPDATEREGISTRY, nullptr);
    if (result != DISP_CHANGE_SUCCESSFUL) {
        // Try without updating registry
        result = ChangeDisplaySettingsExA(dd.DeviceName, &dm, nullptr, 0, nullptr);
    }

    if (result == DISP_CHANGE_SUCCESSFUL) {
        LOG_INFO("DisplaySwitcher: Set resolution to %dx%d", width, height);
        return true;
    } else {
        LOG_ERROR("DisplaySwitcher: Failed to set resolution %dx%d, error=%ld", width, height, result);
        return false;
    }
}

bool DisplaySwitcher::EnableSBS() {
    if (m_sbsEnabled) return true;

    if (!DetectNrealDisplay()) {
        LOG_WARN("DisplaySwitcher: SBS enable failed, no Nreal display");
        return false;
    }

    // Save current mode
    m_originalMode.width = m_nrealInfo.currentWidth;
    m_originalMode.height = m_nrealInfo.currentHeight;
    m_originalMode.refresh = 60;

    // Try 3840x1080 first (native SBS for Nreal Light)
    if (SetDisplayResolution(3840, 1080)) {
        m_sbsEnabled = true;
        LOG_INFO("DisplaySwitcher: SBS mode enabled (3840x1080)");
        return true;
    }

    LOG_ERROR("DisplaySwitcher: Failed to enable SBS mode");
    return false;
}

bool DisplaySwitcher::Enable2D() {
    if (!m_sbsEnabled) return true;

    if (m_originalMode.width > 0 && m_originalMode.height > 0) {
        if (SetDisplayResolution(m_originalMode.width, m_originalMode.height)) {
            m_sbsEnabled = false;
            LOG_INFO("DisplaySwitcher: 2D mode restored (%dx%d)",
                     m_originalMode.width, m_originalMode.height);
            return true;
        }
    }

    // Fallback to 1920x1080
    if (SetDisplayResolution(1920, 1080)) {
        m_sbsEnabled = false;
        LOG_INFO("DisplaySwitcher: 2D mode restored (1920x1080 fallback)");
        return true;
    }

    return false;
}

bool DisplaySwitcher::ToggleMode() {
    if (m_sbsEnabled) {
        return Enable2D();
    } else {
        return EnableSBS();
    }
}

bool DisplaySwitcher::SetBrightness(int level) {
    if (level < 0) level = 0;
    if (level > 100) level = 100;

    if (m_brightnessCb) {
        return m_brightnessCb(level);
    }

    LOG_WARN("DisplaySwitcher: SetBrightness(%d) - no callback wired", level);
    return false;
}
