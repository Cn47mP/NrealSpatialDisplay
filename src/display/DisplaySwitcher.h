#pragma once
#include <windows.h>
#include <string>
#include <vector>

// Manages Nreal display connection and Side-by-Side (SBS) mode
class DisplaySwitcher {
public:
    struct DisplayMode {
        int width = 0;
        int height = 0;
        int refresh = 60;
        bool isSBS = false;
    };

    struct DisplayInfo {
        std::string name;
        int index = -1;
        bool isNreal = false;
        int currentWidth = 0;
        int currentHeight = 0;
        std::vector<DisplayMode> modes;
    };

    DisplaySwitcher();
    ~DisplaySwitcher();

    // Enumerate displays and identify Nreal
    bool DetectNrealDisplay();

    // Switch to SBS 3D mode (3840x1080 or 1920x1080@SBS)
    bool EnableSBS();

    // Switch back to 2D mode
    bool Enable2D();

    // Toggle between 2D and SBS
    bool ToggleMode();

    bool IsSBSEnabled() const { return m_sbsEnabled; }
    bool IsNrealFound() const { return m_nrealFound; }
    const DisplayInfo& GetNrealInfo() const { return m_nrealInfo; }

    // Set brightness (0-100)
    bool SetBrightness(int level);

    static std::vector<DisplayInfo> EnumerateAllDisplays();

private:
    bool SetDisplayResolution(int width, int height);
    bool ChangeDisplaySettings(int index, int width, int height, int refresh);

    bool m_nrealFound = false;
    bool m_sbsEnabled = false;
    DisplayInfo m_nrealInfo;
    DisplayMode m_originalMode;
};