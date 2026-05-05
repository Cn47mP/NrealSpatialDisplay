
#pragma once
#include "DisplayCapture.h"
#include <vector>
#include <memory>
#include <string>

struct DisplayStatus {
    int displayIndex = -1;
    std::string name;
    bool exists = false;
    bool resolutionMatch = false;
    uint32_t expectedWidth = 0;
    uint32_t expectedHeight = 0;
    uint32_t actualWidth = 0;
    uint32_t actualHeight = 0;
    std::string errorMessage;
};

class CaptureManager {
public:
    CaptureManager() = default;
    ~CaptureManager() { Shutdown(); }

    bool Init(ID3D11Device* device, const std::vector<int>& displayIndices);
    void Shutdown();
    void CaptureAll(ID3D11DeviceContext* ctx);
    ID3D11Texture2D* GetTexture(size_t index) const;
    size_t GetCount() const { return m_captures.size(); }
    uint32_t GetSourceWidth(size_t i) const { return i < m_captures.size() ? m_captures[i]->GetWidth() : 0; }
    uint32_t GetSourceHeight(size_t i) const { return i < m_captures.size() ? m_captures[i]->GetHeight() : 0; }

    bool ReinitCapture(int index);
    void ReinitAll();
    void Pause();
    void Resume();
    bool IsHealthy() const;

    std::vector<DisplayStatus> CheckDisplayStatus(const std::vector<int>& expectedIndices,
                                                   uint32_t expectedWidth,
                                                   uint32_t expectedHeight);

private:
    std::vector<std::unique_ptr<DisplayCapture>> m_captures;
    std::vector<ComPtr<ID3D11Texture2D>> m_textures;
    ID3D11Device* m_device = nullptr;
    bool m_paused = false;
};
