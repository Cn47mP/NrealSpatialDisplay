
#pragma once
#include <DirectXMath.h>
#include <string>
#include <vector>
#include "json.hpp"

using namespace DirectX;
using json = nlohmann::json;

struct ScreenConfig {
    int captureIndex = 0;
    std::string name = "Screen";
    XMFLOAT3 position = {0, 0, -2.0f};
    XMFLOAT3 rotationDeg = {0, 0, 0};
    XMFLOAT2 sizeMeters = {0.6f, 0.3375f};
    float curvatureRad = 0.0f;
    int curveSegments = 16;
    int customWidth = 0;
    int customHeight = 0;
    XMMATRIX worldMatrix = XMMatrixIdentity();
};

struct LayoutConfig {
    std::string name = "default";
    std::vector<ScreenConfig> screens;
    float viewerDistance = 2.0f;
    XMFLOAT3 viewerOffset = {0, 0, 0};
};

class LayoutEngine {
public:
    static LayoutConfig LoadFromJson(const std::string& path);
    static void SaveToJson(const std::string& path, const LayoutConfig& config);
    static LayoutConfig CreatePreset(const std::string& name, const std::vector<int>& captureIndices);

    static void UpdateWorldMatrices(LayoutConfig& layout);

    static void MoveScreen(LayoutConfig& layout, int index, XMFLOAT3 delta);
    static void RotateScreen(LayoutConfig& layout, int index, XMFLOAT3 deltaDeg);
    static void ScaleScreen(LayoutConfig& layout, int index, float factor);

    static void SnapScreen(LayoutConfig& layout, int index, float threshold = 0.05f);

    static std::vector<XMFLOAT3> GenerateCurvedMesh(float width, float height, float curvature, int segments);
};
