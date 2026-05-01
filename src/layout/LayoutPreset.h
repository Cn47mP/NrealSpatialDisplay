#pragma once
#include <string>
#include <vector>
#include "LayoutEngine.h"

class LayoutPreset {
public:
    static void RegisterPresets();
    static LayoutConfig GetPreset(const std::string& name, const std::vector<int>& captureIndices);
};