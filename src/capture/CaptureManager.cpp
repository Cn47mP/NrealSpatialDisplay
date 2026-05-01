#include "CaptureManager.h"
#include "../utils/Log.h"
#include <windows.h>
#include <wingdi.h>

std::vector<DisplayStatus> CaptureManager::CheckDisplayStatus(
    const std::vector<int>& expectedIndices,
    uint32_t expectedWidth,
    uint32_t expectedHeight) {

    std::vector<DisplayStatus> results;

    for (int idx : expectedIndices) {
        DisplayStatus status;
        status.displayIndex = idx;
        status.expectedWidth = expectedWidth;
        status.expectedHeight = expectedHeight;
        status.exists = false;
        status.resolutionMatch = false;

        DISPLAY_DEVICEA dd = {};
        dd.cb = sizeof(dd);

        DWORD deviceNum = 0;
        while (EnumDisplayDevicesA(nullptr, deviceNum, &dd, 0)) {
            if (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) {
                DISPLAY_DEVICEA ddMon = {};
                ddMon.cb = sizeof(ddMon);

                if (EnumDisplayDevicesA(dd.DeviceName, 0, &ddMon, 0)) {
                    std::string name(ddMon.DeviceName);

                    if (name.find("virtual") != std::string::npos ||
                        name.find("Virtual") != std::string::npos ||
                        name.find("VRD") != std::string::npos ||
                        name.find("\\Device\\Video") != std::string::npos) {

                        DEVMODEA dm = {};
                        dm.dmSize = sizeof(dm);
                        if (EnumDisplaySettingsA(ddMon.DeviceName, ENUM_CURRENT_SETTINGS, &dm)) {
                            status.exists = true;
                            status.name = ddMon.DeviceName;
                            status.actualWidth = dm.dmPelsWidth;
                            status.actualHeight = dm.dmPelsHeight;

                            if (dm.dmPelsWidth == expectedWidth && dm.dmPelsHeight == expectedHeight) {
                                status.resolutionMatch = true;
                            } else {
                                status.errorMessage = "分辨率不匹配: " +
                                    std::to_string(dm.dmPelsWidth) + "x" + std::to_string(dm.dmPelsHeight) +
                                    " (期望 " + std::to_string(expectedWidth) + "x" + std::to_string(expectedHeight) + ")";
                            }
                            break;
                        }
                    }
                }
            }
            deviceNum++;
        }

        if (!status.exists) {
            status.errorMessage = "虚拟显示器不存在或未连接";
        }

        results.push_back(status);
    }

    return results;
}

bool CaptureManager::Init(ID3D11Device* device, const std::vector<int>& displayIndices) {
    m_device = device;

    for (int idx : displayIndices) {
        auto capture = std::make_unique<DisplayCapture>();
        if (capture->Init(device, idx)) {
            m_captures.push_back(std::move(capture));
        } else {
            LOG_ERROR("CaptureManager: Display source #%d init failed", idx);
            return false;
        }
    }

    m_textures.resize(m_captures.size());
    LOG_INFO("CaptureManager: Initialized %zu sources", m_captures.size());
    return true;
}

void CaptureManager::Shutdown() {
    for (auto& cap : m_captures) cap->Shutdown();
    m_captures.clear();
    for (auto& tex : m_textures) tex.Reset();
    m_textures.clear();
    m_device = nullptr;
}

void CaptureManager::CaptureAll(ID3D11DeviceContext* ctx) {
    if (m_paused) return;

    for (size_t i = 0; i < m_captures.size(); ++i) {
        ID3D11Texture2D* tex = nullptr;
        if (m_captures[i]->CaptureFrame(ctx, &tex)) {
            m_textures[i] = tex;
            tex->Release();
        }
    }
}

ID3D11Texture2D* CaptureManager::GetTexture(size_t index) const {
    if (index >= m_textures.size()) return nullptr;
    return m_textures[index].Get();
}

bool CaptureManager::ReinitCapture(int index) {
    if (index < 0 || index >= (int)m_captures.size()) return false;

    LOG_INFO("CaptureManager: Reinitializing source #%d", index);
    m_captures[index]->Shutdown();
    return m_captures[index]->Init(m_device, index);
}

void CaptureManager::ReinitAll() {
    LOG_INFO("CaptureManager: Reinitializing all sources");
    for (size_t i = 0; i < m_captures.size(); ++i) {
        ReinitCapture((int)i);
    }
}

void CaptureManager::Pause() {
    m_paused = true;
    LOG_INFO("CaptureManager: Paused");
}

void CaptureManager::Resume() {
    m_paused = false;
    LOG_INFO("CaptureManager: Resumed");
}

bool CaptureManager::IsHealthy() const {
    for (const auto& cap : m_captures) {
        if (!cap->IsInitialized()) return false;
    }
    return true;
}