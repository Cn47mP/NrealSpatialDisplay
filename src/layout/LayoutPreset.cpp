#include "LayoutPreset.h"
#include "../utils/Log.h"

void LayoutPreset::RegisterPresets() {
    LOG_INFO("LayoutPreset: Registering presets");
}

LayoutConfig LayoutPreset::GetPreset(const std::string& name, const std::vector<int>& captureIndices) {
    return LayoutEngine::CreatePreset(name, captureIndices);
}