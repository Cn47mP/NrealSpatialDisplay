#include "LayoutEngine.h"
#include "../utils/Log.h"
#include <fstream>
#include <cmath>

LayoutConfig LayoutEngine::CreatePreset(const std::string& name, const std::vector<int>& captureIndices) {
    LayoutConfig config;
    config.name = name;

    if (name == "single") {
        config.viewerDistance = 2.0f;
        config.screens.resize(1);
        config.screens[0].captureIndex = captureIndices.empty() ? 0 : captureIndices[0];
        config.screens[0].name = "Main";
        config.screens[0].position = XMFLOAT3(0.0f, 0.0f, -2.0f);
        config.screens[0].sizeMeters = XMFLOAT2(1.6f, 0.9f);
    } else if (name == "dual") {
        config.viewerDistance = 2.0f;
        config.screens.resize(2);
        config.screens[0].captureIndex = captureIndices.size() > 0 ? captureIndices[0] : 0;
        config.screens[0].name = "Left";
        config.screens[0].position = XMFLOAT3(-0.9f, 0.0f, -2.0f);
        config.screens[0].sizeMeters = XMFLOAT2(0.8f, 0.45f);
        config.screens[1].captureIndex = captureIndices.size() > 1 ? captureIndices[1] : 1;
        config.screens[1].name = "Right";
        config.screens[1].position = XMFLOAT3(0.9f, 0.0f, -2.0f);
        config.screens[1].sizeMeters = XMFLOAT2(0.8f, 0.45f);
    } else if (name == "triple") {
        config.viewerDistance = 2.0f;
        config.screens.resize(3);
        config.screens[0].captureIndex = captureIndices.size() > 0 ? captureIndices[0] : 0;
        config.screens[0].name = "Left";
        config.screens[0].position = XMFLOAT3(-0.9f, 0.0f, -2.0f);
        config.screens[0].rotationDeg = XMFLOAT3(0.0f, 30.0f, 0.0f);
        config.screens[0].sizeMeters = XMFLOAT2(0.6f, 0.3375f);
        config.screens[1].captureIndex = captureIndices.size() > 1 ? captureIndices[1] : 1;
        config.screens[1].name = "Center";
        config.screens[1].position = XMFLOAT3(0.0f, 0.0f, -2.0f);
        config.screens[1].sizeMeters = XMFLOAT2(0.8f, 0.45f);
        config.screens[2].captureIndex = captureIndices.size() > 2 ? captureIndices[2] : 2;
        config.screens[2].name = "Right";
        config.screens[2].position = XMFLOAT3(0.9f, 0.0f, -2.0f);
        config.screens[2].rotationDeg = XMFLOAT3(0.0f, -30.0f, 0.0f);
        config.screens[2].sizeMeters = XMFLOAT2(0.6f, 0.3375f);
    } else if (name == "five-arc") {
        config.viewerDistance = 1.5f;
        config.screens.resize(5);
        float angles[] = { 50.0f, 25.0f, 0.0f, -25.0f, -50.0f };
        float dist = 1.5f;
        for (int i = 0; i < 5; ++i) {
            float angleRad = angles[i] * XM_PI / 180.0f;
            config.screens[i].captureIndex = captureIndices.size() > i ? captureIndices[i] : i;
            config.screens[i].name = "Screen " + std::to_string(i);
            config.screens[i].position = XMFLOAT3(sin(angleRad) * dist, 0.0f, -cos(angleRad) * dist);
            config.screens[i].rotationDeg = XMFLOAT3(0.0f, angles[i], 0.0f);
            config.screens[i].sizeMeters = XMFLOAT2(0.5f, 0.28f);
        }
    } else if (name == "curved-ultrawide") {
        config.viewerDistance = 1.8f;
        config.screens.resize(1);
        config.screens[0].captureIndex = captureIndices.empty() ? 0 : captureIndices[0];
        config.screens[0].name = "Curved";
        config.screens[0].position = XMFLOAT3(0.0f, 0.0f, -1.8f);
        config.screens[0].sizeMeters = XMFLOAT2(2.0f, 0.5f);
        config.screens[0].curvatureRad = 0.4f;
    } else {
        config.viewerDistance = 2.0f;
        config.screens.resize(1);
        config.screens[0].captureIndex = captureIndices.empty() ? 0 : captureIndices[0];
        config.screens[0].name = "Main";
        config.screens[0].position = XMFLOAT3(0.0f, 0.0f, -2.0f);
        config.screens[0].sizeMeters = XMFLOAT2(0.6f, 0.3375f);
    }

    UpdateWorldMatrices(config);
    return config;
}

void LayoutEngine::UpdateWorldMatrices(LayoutConfig& layout) {
    for (auto& screen : layout.screens) {
        XMVECTOR pos = XMLoadFloat3(&screen.position);
        XMMATRIX scale = XMMatrixScaling(screen.sizeMeters.x, screen.sizeMeters.y, 1.0f);
        XMMATRIX rotZ = XMMatrixRotationZ(XMConvertToRadians(screen.rotationDeg.z));
        XMMATRIX rotX = XMMatrixRotationX(XMConvertToRadians(screen.rotationDeg.x));
        XMMATRIX rotY = XMMatrixRotationY(XMConvertToRadians(screen.rotationDeg.y));
        XMMATRIX trans = XMMatrixTranslationFromVector(pos);
        screen.worldMatrix = scale * rotZ * rotX * rotY * trans;
    }
}

LayoutConfig LayoutEngine::LoadFromJson(const std::string& path) {
    try {
        std::ifstream f(path);
        json data;
        f >> data;

        LayoutConfig config;
        config.name = data.value("name", "default");
        config.viewerDistance = data.value("viewerDistance", 2.0f);

        if (data.contains("screens")) {
            for (const auto& sj : data["screens"]) {
                ScreenConfig sc;
                sc.name = sj.value("name", "Screen");
                sc.captureIndex = sj.value("captureIndex", 0);
                if (sj.contains("position")) {
                    auto p = sj["position"];
                    sc.position = XMFLOAT3(p[0], p[1], p[2]);
                }
                if (sj.contains("rotation")) {
                    auto r = sj["rotation"];
                    sc.rotationDeg = XMFLOAT3(r[0], r[1], r[2]);
                }
                if (sj.contains("size")) {
                    auto s = sj["size"];
                    sc.sizeMeters = XMFLOAT2(s[0], s[1]);
                }
                sc.curvatureRad = sj.value("curvature", 0.0f);
                config.screens.push_back(sc);
            }
        }

        UpdateWorldMatrices(config);
        LOG_INFO("LayoutEngine: Loaded %s from %s", config.name.c_str(), path.c_str());
        return config;
    } catch (const std::exception& e) {
        LOG_ERROR("LayoutEngine: Failed to load %s: %s", path.c_str(), e.what());
        return CreatePreset("triple", {0, 1, 2});
    }
}

void LayoutEngine::SaveToJson(const std::string& path, const LayoutConfig& config) {
    try {
        json j;
        j["name"] = config.name;
        j["viewerDistance"] = config.viewerDistance;

        for (const auto& s : config.screens) {
            json sj;
            sj["name"] = s.name;
            sj["captureIndex"] = s.captureIndex;
            sj["position"] = {s.position.x, s.position.y, s.position.z};
            sj["rotation"] = {s.rotationDeg.x, s.rotationDeg.y, s.rotationDeg.z};
            sj["size"] = {s.sizeMeters.x, s.sizeMeters.y};
            sj["curvature"] = s.curvatureRad;
            j["screens"].push_back(sj);
        }

        std::ofstream f(path);
        f << j.dump(2);
        LOG_INFO("LayoutEngine: Saved %s to %s", config.name.c_str(), path.c_str());
    } catch (const std::exception& e) {
        LOG_ERROR("LayoutEngine: Failed to save %s: %s", path.c_str(), e.what());
    }
}

void LayoutEngine::MoveScreen(LayoutConfig& layout, int index, XMFLOAT3 delta) {
    if (index < 0 || index >= (int)layout.screens.size()) return;
    auto& sc = layout.screens[index];
    sc.position.x += delta.x;
    sc.position.y += delta.y;
    sc.position.z += delta.z;
    UpdateWorldMatrices(layout);
}

void LayoutEngine::RotateScreen(LayoutConfig& layout, int index, XMFLOAT3 deltaDeg) {
    if (index < 0 || index >= (int)layout.screens.size()) return;
    auto& sc = layout.screens[index];
    sc.rotationDeg.x += deltaDeg.x;
    sc.rotationDeg.y += deltaDeg.y;
    sc.rotationDeg.z += deltaDeg.z;
    UpdateWorldMatrices(layout);
}

void LayoutEngine::ScaleScreen(LayoutConfig& layout, int index, float factor) {
    if (index < 0 || index >= (int)layout.screens.size()) return;
    auto& sc = layout.screens[index];
    sc.sizeMeters.x *= factor;
    sc.sizeMeters.y *= factor;
    UpdateWorldMatrices(layout);
}

void LayoutEngine::SnapScreen(LayoutConfig& layout, int index, float threshold) {
    if (index < 0 || index >= (int)layout.screens.size()) return;
    auto& sc = layout.screens[index];

    auto snap = [threshold](float v, float t) {
        if (fabs(v - t) < threshold) return t;
        return v;
    };

    sc.position.x = snap(sc.position.x, -0.9f);
    sc.position.x = snap(sc.position.x, 0.9f);
    sc.position.z = snap(sc.position.z, -2.0f);
    sc.rotationDeg.y = snap(sc.rotationDeg.y, -30.0f);
    sc.rotationDeg.y = snap(sc.rotationDeg.y, 30.0f);

    UpdateWorldMatrices(layout);
}

std::vector<XMFLOAT3> LayoutEngine::GenerateCurvedMesh(float width, float height, float curvature, int segments) {
    std::vector<XMFLOAT3> points;
    float radius = curvature > 0.0f ? (1.0f / curvature) : 0.0f;

    for (int row = 0; row <= segments; ++row) {
        float v = (float)row / segments;
        float y = (v - 0.5f) * height;

        for (int col = 0; col <= segments; ++col) {
            float u = (float)col / segments;
            float x = (u - 0.5f) * width;

            float z = 0.0f;
            if (radius > 0.0f) {
                float angle = u * XM_PI;
                z = (float)(radius - radius * cos(angle));
            }

            points.push_back(XMFLOAT3(x, y, z));
        }
    }

    return points;
}