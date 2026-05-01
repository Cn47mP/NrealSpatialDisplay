#!/bin/bash
# 热修复：SettingsGUI.cpp 被清空 + AirIMU 未 patch + 残留文件
# 在仓库根目录运行：bash fix_hotfix.sh
set -e
cd "$(dirname "$0")"

echo "[1/4] 恢复 SettingsGUI.cpp 并实现 RenderPreviewHud()"

git show 72b49768:src/gui/SettingsGUI.cpp > src/gui/SettingsGUI.cpp

python3 << 'PYEOF'
with open("src/gui/SettingsGUI.cpp", "r") as f:
    content = f.read()

old = "void SettingsGUI::RenderPreviewHud() {\n}"
new = """void SettingsGUI::RenderPreviewHud() {
    if (!m_hudVisible) return;

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.6f);
    ImGui::Begin("##PreviewHUD", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing);

    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "FPS: %.1f", m_fps);
    ImGui::Text("Latency: %.1f ms", m_latency);
    ImGui::Text("Layout: %s", m_layoutName.c_str());
    ImGui::Text("Mode: %s", m_cameraMode.c_str());

    ImVec4 imuColor = m_imuConnected
        ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f)
        : ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
    ImGui::TextColored(imuColor, "IMU: %s", m_imuConnected ? "OK" : "DISCONNECTED");

    ImGui::Separator();
    ImGui::Text("P:%.1f Y:%.1f R:%.1f", m_pitch, m_yaw, m_roll);
    ImGui::Text("Screens: %d | Brightness: %d%%", m_screenCount, m_brightness);

    if (m_renderingPaused) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "PAUSED");
    }

    ImGui::End();
}"""

if old in content:
    content = content.replace(old, new)
    with open("src/gui/SettingsGUI.cpp", "w") as f:
        f.write(content)
    print("  ✓ RenderPreviewHud() 已实现")
else:
    print("  ⚠ 未找到空函数")
PYEOF

echo "[2/4] 实现 AirIMU::SetBrightness()"

python3 << 'PYEOF'
with open("src/imu/AirIMU.cpp", "r") as f:
    content = f.read()

old = '''bool AirIMU::SetBrightness(int level) {
    LOG_INFO("AirIMU: Brightness control not implemented");
    return false;
}'''

new = '''bool AirIMU::SetBrightness(int level) {
    if (level < 0) level = 0;
    if (level > 100) level = 100;

    typedef int(*SetBrightnessAPI_t)(int);
    static SetBrightnessAPI_t pSetBrightness = nullptr;

    if (!pSetBrightness && m_airAPIDll) {
        pSetBrightness = (SetBrightnessAPI_t)GetProcAddress(m_airAPIDll, "SetBrightness");
    }

    if (pSetBrightness) {
        int result = pSetBrightness(level);
        if (result >= 0) {
            LOG_INFO("AirIMU: Brightness set to %d%%", level);
            return true;
        }
        LOG_WARN("AirIMU: SetBrightness(%d) returned %d", level, result);
        return false;
    }

    LOG_WARN("AirIMU: SetBrightness not available in AirAPI DLL");
    return false;
}'''

if old in content:
    content = content.replace(old, new)
    with open("src/imu/AirIMU.cpp", "w") as f:
        f.write(content)
    print("  ✓ SetBrightness() 已实现")
else:
    print("  ⚠ 未找到原函数")
PYEOF

echo "[3/4] 删除残留文件"

git rm --cached airapi/hidapi-win/x64/hidapi.pdb 2>/dev/null || true
rm -f airapi/hidapi-win/x64/hidapi.pdb
git rm --cached airapi/hidapi-win/x86/hidapi.pdb 2>/dev/null || true
rm -f airapi/hidapi-win/x86/hidapi.pdb

git rm -r --cached airapi/AirAPI_Windows-master/ 2>/dev/null || true
rm -rf airapi/AirAPI_Windows-master/

git rm -r --cached src/gui/imgui/backends/sdlgpu3 2>/dev/null || true
rm -rf src/gui/imgui/backends/sdlgpu3
git rm -r --cached src/gui/imgui/backends/vulkan 2>/dev/null || true
rm -rf src/gui/imgui/backends/vulkan

echo "[4/4] 验证"
echo "  SettingsGUI.cpp: $(wc -c < src/gui/SettingsGUI.cpp) bytes"
echo "  AirIMU SetBrightness: $(grep -c 'GetProcAddress.*SetBrightness' src/imu/AirIMU.cpp) match(es)"
echo "  .pdb: $(find . -name '*.pdb' 2>/dev/null | wc -l) remaining"
echo "  AirAPI src: $([ -d airapi/AirAPI_Windows-master ] && echo '还在' || echo '已删除')"

echo ""
echo "完成！提交："
echo "  git add -A && git commit -m 'hotfix: restore SettingsGUI, patch SetBrightness, cleanup' && git push"
