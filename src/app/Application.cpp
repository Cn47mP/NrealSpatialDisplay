#include "Application.h"

Application* Application::s_instance = nullptr;

Application::Application() {
    s_instance = this;
    m_lastTooltipUpdate = std::chrono::steady_clock::now();
}

Application::~Application() {
    Shutdown();
}

LRESULT Application::PreviewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_USER + 1:
        if (LOWORD(lParam) == WM_RBUTTONUP) {
            if (s_instance) {
                s_instance->ShowTrayMenu();
            }
        }
        return 0;
    case WM_COMMAND:
        if (s_instance && HIWORD(wParam) == 0) {
            s_instance->HandleTrayCommand(LOWORD(wParam));
        }
        return 0;
    case WM_HOTKEY:
        if (s_instance) {
            int id = (int)wParam;
            auto it = s_instance->m_vkToAction.find(id);
            if (it != s_instance->m_vkToAction.end()) {
                s_instance->HandleAction(it->second);
            }
        }
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool Application::Init() {
    char exePath[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    char* slash = strrchr(exePath, '\\');
    if (slash) *slash = '\0';
    std::string configPath = std::string(exePath) + "\\..\\..\\..\\config\\default.json";
    LOG_INFO("Config search path: %s", configPath.c_str());
    m_config = AppConfig::Load(configPath);

    m_renderer = std::make_unique<D3DRenderer>();
    if (!m_renderer->Init(nullptr, 1920, 1080)) {
        LOG_ERROR("Application: Renderer init failed");
        return false;
    }

    m_renderer->InitScreenResources((int)m_config.layout.screens.size());

    for (size_t i = 0; i < m_config.layout.screens.size(); ++i) {
        auto& sc = m_config.layout.screens[i];
        m_renderer->InitCurvedScreen((int)i, sc.sizeMeters.x, sc.sizeMeters.y, sc.curvatureRad, sc.curveSegments);
    }

    // Display switcher
    m_displaySwitcher = std::make_unique<DisplaySwitcher>();
    m_displaySwitcher->DetectNrealDisplay();

    if (m_config.imu.enabled) {
        // Try Nreal Air first
        m_airIMU = std::make_unique<AirIMU>();
        bool airStarted = m_airIMU->Start();
        bool airConnected = airStarted && m_airIMU->IsConnected();

        if (airConnected) {
            LOG_INFO("Application: Nreal Air device connected");
            m_airIMU->SetCallback([this](const ImuData& data) {
                m_camera.UpdateFromIMU(data);
                if (m_unityStream && m_unityStream->IsRunning()) {
                    m_unityStream->SendImuData(data.quatX, data.quatY, data.quatZ, data.quatW,
                                               data.pitch, data.yaw, data.roll, data.timestampUs);
                }
            });
        } else {
            if (airStarted) {
                m_airIMU->Stop();
            }
            m_airIMU.reset();

            // Try Nreal Light
            m_lightIMU = std::make_unique<NrealLightIMU>();
            if (m_lightIMU->Start()) {
                if (m_lightIMU->IsConnected()) {
                    LOG_INFO("Application: Nreal Light connected");
                } else {
                    LOG_INFO("Application: Nreal Light not found, simulation mode");
                }
                m_lightIMU->SetCallback([this](const ImuData& data) {
                    m_camera.UpdateFromIMU(data);
                    if (m_unityStream && m_unityStream->IsRunning()) {
                        m_unityStream->SendImuData(data.quatX, data.quatY, data.quatZ, data.quatW,
                                                   data.pitch, data.yaw, data.roll, data.timestampUs);
                    }
                });
            }
        }
        LOG_INFO("Application: IMU initialized");
    }

    // Unity UDP stream
    m_unityStream = std::make_unique<UnityStream>();
    if (m_unityStream->Start("127.0.0.1", 4242)) {
        LOG_INFO("Application: Unity stream started on 127.0.0.1:4242");
    }

    m_captureMgr = std::make_unique<CaptureManager>();
    std::vector<int> indices;
    for (const auto& sc : m_config.layout.screens) {
        indices.push_back(sc.captureIndex);
    }
    m_captureMgr->Init(m_renderer->GetDevice(), indices);

    m_gui = std::make_unique<SettingsGUI>();

    CreatePreviewWindow();

    if (m_previewHwnd) {
        m_gui->Init(m_previewHwnd, m_renderer->GetDevice(), m_renderer->GetContext());
    }

    CreateTrayIcon();

    RegisterHotkeys();

    LOG_INFO("Application: Initialized");
    return true;
}

void Application::Shutdown() {
    LOG_INFO("Application: Shutting down...");

    if (m_airIMU) {
        m_airIMU->Stop();
        m_airIMU.reset();
    }
    if (m_lightIMU) {
        m_lightIMU->Stop();
        m_lightIMU.reset();
    }
    if (m_unityStream) {
        m_unityStream->Stop();
        m_unityStream.reset();
    }
    if (m_displaySwitcher) {
        m_displaySwitcher.reset();
    }

    if (m_renderer) {
        m_renderer->Shutdown();
        m_renderer.reset();
    }

    if (m_previewHwnd) {
        DestroyWindow(m_previewHwnd);
        m_previewHwnd = nullptr;
    }

    DestroyTrayIcon();
    UnregisterHotkeys();

    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }

    LOG_INFO("Application: Shutdown complete");
}

bool Application::CreatePreviewWindow() {
    WNDCLASSA wc = {};
    wc.lpfnWndProc = PreviewWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "NrealPreview";
    RegisterClassA(&wc);

    m_previewHwnd = CreateWindowExA(
        0, "NrealPreview", "Preview",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, m_previewW, m_previewH,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr
    );

    m_previewEnabled = (m_previewHwnd != nullptr);

    if (m_previewHwnd) {
        ShowWindow(m_previewHwnd, SW_SHOW);
        SetWindowPos(m_previewHwnd, HWND_TOP, 100, 100, m_previewW, m_previewH, SWP_SHOWWINDOW);
        BringWindowToTop(m_previewHwnd);
        SetForegroundWindow(m_previewHwnd);
        LOG_INFO("Preview window shown, hwnd=%p", m_previewHwnd);
    }

    if (m_renderer && m_previewEnabled) {
        m_renderer->InitPreview(m_previewHwnd, m_previewW, m_previewH);
    }

    return m_previewEnabled;
}

void Application::Tick() {
    if (!m_renderer) return;

    if (m_renderingPaused) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        return;
    }

    m_frameCount++;

    // Update GUI status
    if (m_lightIMU && m_lightIMU->IsConnected()) {
        ImuData data = m_lightIMU->GetLatest();
        m_camera.UpdateFromIMU(data);
        if (m_gui) {
            m_gui->SetImuConnected(true);
            m_gui->SetImuData(data.pitch, data.yaw, data.roll);
        }
        if (m_unityStream && m_unityStream->IsRunning()) {
            m_unityStream->SendImuData(data.quatX, data.quatY, data.quatZ, data.quatW,
                                       data.pitch, data.yaw, data.roll, data.timestampUs);
        }
    } else if (m_airIMU && m_airIMU->IsConnected()) {
        ImuData data = m_airIMU->GetLatest();
        m_camera.UpdateFromIMU(data);
        if (m_gui) {
            m_gui->SetImuConnected(true);
            m_gui->SetImuData(data.pitch, data.yaw, data.roll);
        }
        if (m_unityStream && m_unityStream->IsRunning()) {
            m_unityStream->SendImuData(data.quatX, data.quatY, data.quatZ, data.quatW,
                                       data.pitch, data.yaw, data.roll, data.timestampUs);
        }
    } else {
        if (m_gui) {
            m_gui->SetImuConnected(false);
        }

        if (m_connState == ConnectionState::Connected) {
            m_connState = ConnectionState::Disconnected;
            m_lastReconnectAttempt = std::chrono::steady_clock::now();
            LOG_WARN("IMU disconnected, attempting to reconnect...");
        } else if (m_connState == ConnectionState::Disconnected) {
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastReconnectAttempt).count() > RECONNECT_INTERVAL_MS) {
                m_connState = ConnectionState::Reconnecting;
            }
        } else if (m_connState == ConnectionState::Reconnecting) {
            if (TryReconnect()) {
                m_connState = ConnectionState::Connected;
                LOG_INFO("Reconnection successful");
            } else {
                m_connState = ConnectionState::Disconnected;
                m_lastReconnectAttempt = std::chrono::steady_clock::now();
            }
        }
    }

    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastTooltipUpdate).count() > TOOLTIP_UPDATE_INTERVAL_MS) {
        UpdateTrayTooltip();
        m_lastTooltipUpdate = now;
    }

    if (m_captureMgr) {
        m_captureMgr->CaptureAll(m_renderer->GetContext());
    }

    if (m_previewEnabled) {
        static int frameCount = 0;
        frameCount++;
        if (frameCount % 60 == 0) {
            LOG_DEBUG("Preview tick %d, screens=%zu, guiEnabled=%d, guiVisible=%d",
                frameCount, m_config.layout.screens.size(),
                m_gui ? m_gui->IsEnabled() : -1,
                m_gui ? m_gui->IsVisible() : -1);
        }

        m_renderer->BeginFrame();

        XMMATRIX view = m_camera.GetViewMatrix();
        for (size_t i = 0; i < m_config.layout.screens.size(); ++i) {
            auto& sc = m_config.layout.screens[i];
            ID3D11Texture2D* tex = m_captureMgr->GetTexture(i);
            if (tex) {
                m_renderer->UpdateTexture(i, tex);
                m_renderer->DrawScreen(sc.worldMatrix, view, m_renderer->GetTextureSRV(i), (int)i);
            }
        }

        if (m_gui && m_gui->IsEnabled()) {
            LOG_DEBUG("GUI NewFrame");
            m_gui->NewFrame();
            LOG_DEBUG("GUI Render");
            m_gui->Render();
            LOG_DEBUG("GUI done");
        }

        m_renderer->EndFrame();
        LOG_DEBUG("PresentPreview");
        m_renderer->PresentPreview();
        LOG_DEBUG("Frame done");
    }
}

void Application::HandleAction(const std::string& action) {
    if (action == "quit") {
        PostMessage(m_hwnd, WM_QUIT, 0, 0);
    } else if (action == "reset_view") {
        m_camera.ResetYawZero();
    } else if (action == "toggle_imu") {
        if (m_airIMU && m_airIMU->IsConnected()) {
            m_airIMU->Stop();
        } else if (m_lightIMU && m_lightIMU->IsConnected()) {
            m_lightIMU->Stop();
        } else if (m_airIMU) {
            m_airIMU->Start();
        } else if (m_lightIMU) {
            m_lightIMU->Start();
        }
    } else if (action == "screenshot") {
        TakeScreenshot();
    } else if (action == "toggle_gui") {
        if (m_gui) {
            m_gui->ToggleVisible();
        }
    } else if (action == "toggle_preview") {
        if (m_previewEnabled) {
            ShowWindow(m_previewHwnd, SW_HIDE);
            m_previewEnabled = false;
        } else {
            ShowWindow(m_previewHwnd, SW_SHOW);
            m_previewEnabled = true;
        }
    } else if (action == "toggle_rendering") {
        SetPaused(!m_renderingPaused);
    } else if (action == "toggle_headlock") {
        m_camera.ToggleMode();
        LOG_INFO("Camera mode: %s", m_camera.GetMode() == CameraMode::HeadLock ? "Head Lock" : "World Lock");
    } else if (action == "preset_single") {
        SwitchLayout("single");
    } else if (action == "preset_dual") {
        SwitchLayout("dual");
    } else if (action == "preset_triple") {
        SwitchLayout("triple");
    } else if (action == "preset_five") {
        SwitchLayout("five-arc");
    } else if (action == "preset_curved") {
        SwitchLayout("curved-ultrawide");
    }
}

void Application::SetPaused(bool paused) {
    m_renderingPaused = paused;
    if (paused) {
        if (m_captureMgr) m_captureMgr->Pause();
        if (m_airIMU) m_airIMU->SetPollRate(10);
        if (m_lightIMU) m_lightIMU->SetPollRate(10);
        LOG_INFO("Rendering paused - capture/IMU power saving enabled");
    } else {
        if (m_captureMgr) m_captureMgr->Resume();
        if (m_airIMU) m_airIMU->SetPollRate(250);
        if (m_lightIMU) m_lightIMU->SetPollRate(250);
        LOG_INFO("Rendering resumed - full speed");
    }
}

bool Application::TryReconnect() {
    LOG_INFO("Attempting to reconnect IMU...");

    bool reconnected = false;

    if (m_lightIMU) {
        if (!m_lightIMU->IsConnected()) {
            m_lightIMU->Stop();
            if (m_lightIMU->Start()) {
                LOG_INFO("NrealLight IMU reconnected successfully");
                reconnected = true;
            }
        } else {
            reconnected = true;
        }
    }

    if (m_airIMU) {
        if (!m_airIMU->IsConnected()) {
            if (m_airIMU->Reconnect()) {
                LOG_INFO("AirIMU reconnected successfully");
                reconnected = true;
            }
        } else {
            reconnected = true;
        }
    }

    return reconnected;
}

void Application::RegisterHotkeys() {
    UnregisterHotkeys();

    for (const auto& hk : m_config.hotkeys.entries) {
        int mods = 0;
        if (hk.ctrl) mods |= MOD_CONTROL;
        if (hk.alt) mods |= MOD_ALT;
        if (hk.shift) mods |= MOD_SHIFT;

        int id = GetHotkeyId(hk.id);
        if (id == -1) {
            id = 1000 + (int)m_vkToAction.size();
            m_actionToId[hk.id] = id;
        }

        HWND hwnd = m_previewHwnd ? m_previewHwnd : m_hwnd;
        if (RegisterHotKey(hwnd, id, mods, hk.key)) {
            m_vkToAction[id] = hk.id;
            m_hotkeys.push_back(hk);
            LOG_INFO("Registered hotkey: %s (vk=%d, ctrl=%d, alt=%d, shift=%d)",
                     hk.id.c_str(), hk.key, hk.ctrl, hk.alt, hk.shift);
        } else {
            LOG_WARN("Failed to register hotkey: %s (vk=%d)", hk.id.c_str(), hk.key);
        }
    }
}

void Application::UnregisterHotkeys() {
    HWND hwnd = m_previewHwnd ? m_previewHwnd : m_hwnd;
    for (const auto& [id, action] : m_vkToAction) {
        UnregisterHotKey(hwnd, id);
    }
    m_vkToAction.clear();
    m_hotkeys.clear();
}

int Application::GetHotkeyId(const std::string& action) {
    auto it = m_actionToId.find(action);
    return (it != m_actionToId.end()) ? it->second : -1;
}

bool Application::HasConflict(int vk, bool ctrl, bool alt, bool shift, const std::string& excludeAction) {
    for (const auto& hk : m_config.hotkeys.entries) {
        if (hk.id == excludeAction) continue;
        if (hk.key != vk) continue;
        if (hk.ctrl != ctrl || hk.alt != alt || hk.shift != shift) continue;
        return true;
    }
    return false;
}

LRESULT Application::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        PostMessage(m_hwnd, WM_QUIT, 0, 0);
        return 0;
    case WM_HOTKEY:
        {
            int id = (int)wParam;
            auto it = m_vkToAction.find(id);
            if (it != m_vkToAction.end()) {
                HandleAction(it->second);
            }
        }
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            PostMessage(m_hwnd, WM_CLOSE, 0, 0);
        }
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void Application::CreateTrayIcon() {
    if (m_trayCreated) return;

    m_trayIcon.cbSize = sizeof(NOTIFYICONDATAW);
    m_trayIcon.hWnd = m_previewHwnd ? m_previewHwnd : m_hwnd;
    m_trayIcon.uID = 1;
    m_trayIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_trayIcon.uCallbackMessage = WM_USER + 1;
    m_trayIcon.hIcon = LoadIcon(nullptr, IDI_APPLICATION);

    MultiByteToWideChar(CP_UTF8, 0, "NrealSpatialDisplay - 多屏空间锚定", -1, m_trayIcon.szTip, 128);

    if (Shell_NotifyIconW(NIM_ADD, &m_trayIcon)) {
        m_trayCreated = true;
        LOG_INFO("System tray icon created");
    } else {
        LOG_ERROR("Failed to create system tray icon");
    }
}

void Application::DestroyTrayIcon() {
    if (!m_trayCreated) return;
    Shell_NotifyIconW(NIM_DELETE, &m_trayIcon);
    m_trayCreated = false;
    LOG_INFO("System tray icon destroyed");
}

void Application::UpdateTrayTooltip() {
    if (!m_trayCreated) return;

    std::vector<int> indices;
    for (int i = 0; i < (int)m_config.layout.screens.size(); ++i) {
        indices.push_back(m_config.layout.screens[i].captureIndex);
    }

    auto statuses = m_captureMgr->CheckDisplayStatus(
        indices,
        m_config.display.virtualWidth,
        m_config.display.virtualHeight);

    wchar_t tip[128] = {};
    bool hasWarning = false;
    std::wstring details;

    for (const auto& st : statuses) {
        if (!st.exists || !st.resolutionMatch) {
            hasWarning = true;
            if (!st.errorMessage.empty()) {
                details += L"⚠ " + std::wstring(st.errorMessage.begin(), st.errorMessage.end()) + L"\n";
            }
        }
    }

    if (hasWarning) {
        swprintf_s(tip, L"NrealSpatialDisplay ⚠ 显示器异常\n%ls", details.c_str());
    } else {
        swprintf_s(tip, L"NrealSpatialDisplay - %S\n布局: %S | %d屏",
                   m_config.layout.name.c_str(),
                   m_config.layout.name.c_str(),
                   (int)m_config.layout.screens.size());
    }

    wcsncpy_s(m_trayIcon.szTip, tip, 128);
    Shell_NotifyIconW(NIM_MODIFY, &m_trayIcon);
}

void Application::ShowTrayMenu() {
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();

    HMENU hLayoutSub = CreatePopupMenu();
    AppendMenuW(hLayoutSub, MF_STRING, 1001, L"单屏 (&1)");
    AppendMenuW(hLayoutSub, MF_STRING, 1002, L"双屏 (&2)");
    AppendMenuW(hLayoutSub, MF_STRING, 1003, L"三屏环绕 (&3)");
    AppendMenuW(hLayoutSub, MF_STRING, 1005, L"五屏弧形 (&5)");
    AppendMenuW(hLayoutSub, MF_STRING, 1006, L"曲面超宽屏 (&C)");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hLayoutSub, L"切换布局");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    AppendMenuW(hMenu, MF_STRING, 2001, L"设置面板  F1");
    AppendMenuW(hMenu, MF_STRING, 2002, L"预览窗口  F3");
    AppendMenuW(hMenu, MF_STRING, 2003, L"截图  Ctrl+Alt+S");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    if (m_renderer) {
        AppendMenuW(hMenu, MF_STRING, 2004, L"暂停渲染");
    } else {
        AppendMenuW(hMenu, MF_STRING, 2004, L"启动渲染");
    }

    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, 2005, L"退出 (&Q)");

    SetForegroundWindow(m_previewHwnd ? m_previewHwnd : m_hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_previewHwnd ? m_previewHwnd : m_hwnd, nullptr);
    DestroyMenu(hMenu);
}

void Application::HandleTrayCommand(int id) {
    switch (id) {
    case 1001: HandleAction("preset_single"); break;
    case 1002: HandleAction("preset_dual"); break;
    case 1003: HandleAction("preset_triple"); break;
    case 1005: HandleAction("preset_five"); break;
    case 1006: HandleAction("preset_curved"); break;
    case 2001: HandleAction("toggle_gui"); break;
    case 2002: HandleAction("toggle_preview"); break;
    case 2003: HandleAction("screenshot"); break;
    case 2004: HandleAction("toggle_rendering"); break;
    case 2005: HandleAction("quit"); break;
    }
}

void Application::TakeScreenshot() {
    if (m_renderer) {
        m_renderer->SaveScreenshot("screenshot.bmp");
    }
}

void Application::SwitchLayout(const std::string& name) {
    LOG_INFO("Switching layout to: %s", name.c_str());
    std::vector<int> indices;
    for (const auto& sc : m_config.layout.screens) {
        indices.push_back(sc.captureIndex);
    }
    m_config.layout = LayoutEngine::CreatePreset(name, indices);
    LayoutEngine::UpdateWorldMatrices(m_config.layout);

    if (m_renderer) {
        m_renderer->InitScreenResources((int)m_config.layout.screens.size());
        for (size_t i = 0; i < m_config.layout.screens.size(); ++i) {
            auto& sc = m_config.layout.screens[i];
            m_renderer->InitCurvedScreen((int)i, sc.sizeMeters.x, sc.sizeMeters.y, sc.curvatureRad, sc.curveSegments);
        }
    }

    if (m_gui) {
        m_gui->SetLayoutName(name);
    }
}