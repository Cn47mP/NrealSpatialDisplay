#pragma once
// src/gui/ImGuiSetup.h
// ImGui 全局配置

// 不使用 ImGui 自带的 assert
#define IM_ASSERT(_EXPR) ((void)(_EXPR))

// 启用 Unicode（中文支持）
#define IMGUI_USE_WCHAR32

// 禁用不需要的后端
#define IMGUI_DISABLE_DEMO_WINDOWS
#define IMGUI_DISABLE_METRICS_WINDOW

// 版本校验
#include "imgui/imgui.h"
