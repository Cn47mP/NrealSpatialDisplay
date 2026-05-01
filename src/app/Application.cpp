// Application.cpp - 核心应用实现
// NrealSpatialDisplay: AR 多虚拟屏空间锚定系统

#include "Application.h"
#include "core/Log.h"
#include "core/AppConfig.h"
#include "layout/LayoutEngine.h"
#include "layout/LayoutPreset.h"
#include "render/D3DRenderer.h"
#include "capture/CaptureManager.h"
#include "imu/AirIMU.h"
#include "imu/NrealLightIMU.h"
#include "gui/SettingsGUI.h"
#include "display/DisplaySwitcher.h"
#include "stream/UnityStream.h"
#include "render/Camera.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <shellapi.h>

// 系统托盘消息
#define WM_TRAYICON (WM_APP + 1)

// 全局热键 ID 起始值
static constexpr int HOTKEY_BASE = 0x1000;

// 静态实例指针（用于窗口过程回调）
Application* Application::s_instance = nullptr;

// ============================================================
// 构造 / 析构
// ============================================================

Application::Application()
    : m_renderingPaused(false)
    , m_hwnd(nullptr)
    , m_previewHwnd(nullptr)
    , m_previewEnabled(false)
    , m_trayCreated(false)
    , m_fps(0.0f)
    , m_frameCount(0)
    , m_connState(ConnectionState::Disconnected)
{
    s_instance = this;
    m_lastTick = std::chrono::high_resolution_clock::now();
    m_lastImuTime = std::chrono::steady_clock::now();
    m_lastReconnectAttempt = std::chrono::steady_clock::now();
    m_lastTooltipUpdate = std::chrono::steady_clock::now();
}

Application::~Application()
{
    Shutdown();
}

// ============================================================
// Init - 初始化所有子系统
// ============================================================

bool Application::Init()
{
    // 1. COM 初始化
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        LOG_ERROR("CoInitializeEx 失败: 0x%08X", hr);
        return false;
    }

    // 2. 日志系统
    Log::Init("logs");
    LOG_INFO("NrealSpatialDisplay 启动中...");

    // 3. 注册布局预设
    LayoutPreset::RegisterPresets();

    // 4. 加载配置
    m_config = AppConfig::Load("config/default.json");
    LOG_INFO("配置加载完成，布局: %s", m_config.layout.name.c_str());

    // 5. 缓存热键列表
    m_hotkeys = m_config.hotkeys.entries;

    // 6. 创建主窗口（自动寻找 Nreal 显示器）
    if (!CreateMainWindow())
    {
        LOG_ERROR("创建主窗口失败");
        return false;
    }

    // 7. 初始化 D3D 渲染器
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    m_renderer = std::make_unique<D3DRenderer>();
    if (!m_renderer->Init(m_hwnd, width, height))
    {
        LOG_ERROR("D3D 渲染器初始化失败");
        return false;
    }

    // 8. 创建屏幕渲染资源
    if (!m_renderer->InitScreenResources(m_config.display.virtualCount))
    {
        LOG_ERROR("屏幕资源初始化失败");
        return false;
    }

    // 9. 初始化曲面屏资源
    for (int i = 0; i < static_cast<int>(m_config.layout.screens.size()); ++i)
    {
        const auto& screen = m_config.layout.screens[i];
        if (screen.curvatureRad > 0.0f)
        {
            m_renderer->InitCurvedScreen(i, screen.curvatureRad, screen.curveSegments);
        }
    }

    // 10. 创建预览窗口
    CreatePreviewWindow();
    m_renderer->InitPreview(m_previewHwnd, m_previewW, m_previewH);

    // 11. 初始化 ImGui 设置面板
    m_gui = std::make_unique<SettingsGUI>();
    auto* device = m_renderer->GetDevice();
    auto* ctx = m_renderer->GetContext();
    m_gui->Init(m_hwnd, device, ctx);

    // 12. 初始化预览 HUD
    m_gui->InitPreviewHud(m_renderer->GetPreviewDevice(), m_renderer->GetPreviewContext(),
                          m_previewW, m_previewH);

    // 13. 初始化屏幕捕获管理器
    m_captureMgr = std::make_unique<CaptureManager>();
    {
        std::vector<int> captureIndices;
        for (const auto& screen : m_config.layout.screens)
        {
            captureIndices.push_back(screen.captureIndex);
        }
        m_captureMgr->Init(device, captureIndices);
    }

    // 14. 初始化 IMU（如果启用）
    if (m_config.imu.enabled)
    {
        m_airIMU = std::make_unique<AirIMU>();
        m_airIMU->Start();
        m_airIMU->SetDisconnectCallback([this]() {
            LOG_WARN("AirIMU 断开连接");
            m_connState = ConnectionState::Disconnected;
        });
        m_connState = ConnectionState::Connected;
    }

    // 15. 计算世界矩阵
    LayoutEngine::UpdateWorldMatrices(m_config.layout);

    // 16. 设置相机初始位置
    m_camera.SetPosition(m_config.layout.viewerOffset);

    // 17. 注册全局热键
    RegisterHotkeys();

    // 18. 创建系统托盘图标
    CreateTrayIcon();

    // 19. 绑定 GUI 回调
    m_gui->SetOnToggle([this](const std::string& id, bool enabled) {
        HandleAction(id);
    });
    m_gui->SetOnConfigChanged([this]() {
        // 配置变更时可触发保存或重载
    });
    m_gui->SetOnLayoutSwitch([this](const std::string& name) {
        SwitchLayout(name);
    });
    m_gui->SetOnResetView([this]() {
        HandleAction("reset_view");
    });
    m_gui->SetOnQuit([this]() {
        PostQuitMessage(0);
    });
    m_gui->SetOnAction([this](const std::string& action) {
        HandleAction(action);
    });

    // 20. 完成
    m_lastTick = std::chrono::high_resolution_clock::now();
    LOG_INFO("Init 完成，共 %d 个虚拟屏", static_cast<int>(m_config.layout.screens.size()));
    return true;
}

// ============================================================
// Tick - 每帧更新与渲染
// ============================================================

void Application::Tick()
{
    // 1. 性能统计
    m_frameCount++;
    auto now = std::chrono::high_resolution_clock::now();
    float elapsed = std::chrono::duration<float>(now - m_lastTick).count();
    if (elapsed >= 1.0f)
    {
        m_fps = static_cast<float>(m_frameCount) / elapsed;
        m_frameCount = 0;
        m_lastTick = now;
    }

    // 2. 连接状态检查与自动重连
    if (m_connState == ConnectionState::Disconnected)
    {
        auto reconnectNow = std::chrono::steady_clock::now();
        auto reconnectElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            reconnectNow - m_lastReconnectAttempt).count();
        if (reconnectElapsed >= RECONNECT_INTERVAL_MS)
        {
            m_lastReconnectAttempt = reconnectNow;
            TryReconnect();
        }
    }

    // 3. 渲染暂停时降低 CPU 占用
    if (m_renderingPaused)
    {
        Sleep(100);
        return;
    }

    // 4. IMU 数据 → 相机更新
    if (m_airIMU && m_airIMU->IsConnected())
    {
        ImuData imu = m_airIMU->GetLatest();
        m_camera.UpdateFromIMU(imu);
    }

    // 5. 屏幕捕获
    m_captureMgr->CaptureAll();

    // 6. 更新纹理
    for (int i = 0; i < static_cast<int>(m_config.layout.screens.size()); ++i)
    {
        ID3D11ShaderResourceView* tex = m_captureMgr->GetTexture(i);
        if (tex)
        {
            m_renderer->UpdateTexture(i, tex);
        }
    }

    // 7. 开始渲染帧
    m_renderer->BeginFrame();

    // 8. 计算 View-Projection 矩阵（52° FOV）
    float fovY = 52.0f * 3.14159265f / 180.0f;
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    float aspect = static_cast<float>(rc.right) / static_cast<float>(rc.bottom);
    XMMATRIX proj = XMMatrixPerspectiveFovLH(fovY, aspect, 0.1f, 100.0f);
    XMMATRIX viewProj = m_camera.GetViewMatrix() * proj;

    // 9. 绘制所有屏幕
    for (int i = 0; i < static_cast<int>(m_config.layout.screens.size()); ++i)
    {
        ID3D11ShaderResourceView* srv = m_captureMgr->GetTexture(i);
        m_renderer->DrawScreen(m_config.layout.screens[i].worldMatrix, viewProj, srv, i);
    }

    // 10. 渲染 ImGui 设置面板
    if (m_gui->IsVisible())
    {
        m_gui->NewFrame();
        m_gui->Render();
    }

    // 11. 结束主窗口渲染
    m_renderer->EndFrame();

    // 12. 预览窗口
    if (m_renderer->IsPreviewReady())
    {
        m_gui->RenderPreviewHud();
        m_renderer->PresentPreview();
    }

    // 13. 更新 GUI 显示数据
    m_gui->SetFps(m_fps);
    if (m_airIMU)
    {
        m_gui->SetImuData(m_airIMU->GetLatest());
    }
    m_gui->SetCameraMode(m_camera.GetMode() == CameraMode::HeadLock ? "HeadLock" : "Free");
    m_gui->SetCaptureCount(m_captureMgr->GetActiveCount());
    m_gui->SetScreenCount(static_cast<int>(m_config.layout.screens.size()));
    m_gui->SetLayoutName(m_config.layout.name);
    m_gui->SetRenderingPaused(m_renderingPaused);

    // 定期更新托盘提示
    auto tooltipNow = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(
            tooltipNow - m_lastTooltipUpdate).count() >= TOOLTIP_UPDATE_INTERVAL_MS)
    {
        UpdateTrayTooltip();
        m_lastTooltipUpdate = tooltipNow;
    }
}

// ============================================================
// Shutdown - 清理所有资源
// ============================================================

void Application::Shutdown()
{
    UnregisterHotkeys();
    DestroyTrayIcon();

    if (m_airIMU)
    {
        m_airIMU->Stop();
        m_airIMU.reset();
    }

    if (m_lightIMU)
    {
        m_lightIMU.reset();
    }

    if (m_captureMgr)
    {
        m_captureMgr->Shutdown();
        m_captureMgr.reset();
    }

    if (m_gui)
    {
        m_gui->Shutdown();
        m_gui.reset();
    }

    if (m_renderer)
    {
        m_renderer->Shutdown();
        m_renderer.reset();
    }

    Log::Shutdown();
    CoUninitialize();

    LOG_INFO("Shutdown 完成");
}

// ============================================================
// HandleAction - 处理热键/菜单动作
// ============================================================

void Application::HandleAction(const std::string& action)
{
    if (action == "preset_single")
    {
        SwitchLayout("single");
    }
    else if (action == "preset_dual")
    {
        SwitchLayout("dual");
    }
    else if (action == "preset_triple")
    {
        SwitchLayout("triple");
    }
    else if (action == "preset_five")
    {
        SwitchLayout("five-arc");
    }
    else if (action == "toggle_imu")
    {
        if (!m_airIMU)
        {
            m_airIMU = std::make_unique<AirIMU>();
            m_airIMU->Start();
            m_airIMU->SetDisconnectCallback([this]() {
                m_connState = ConnectionState::Disconnected;
            });
            m_connState = ConnectionState::Connected;
            LOG_INFO("IMU 已启动");
        }
        else
        {
            m_airIMU->Stop();
            m_airIMU.reset();
            LOG_INFO("IMU 已停止");
        }
    }
    else if (action == "reset_view")
    {
        m_camera.ResetYawZero();
        m_camera.SetRotation(0.0f, 0.0f, 0.0f);
        LOG_INFO("视角已重置");
    }
    else if (action == "toggle_headlock")
    {
        m_camera.ToggleMode();
        LOG_INFO("头部锁定模式: %s",
                 m_camera.GetMode() == CameraMode::HeadLock ? "开" : "关");
    }
    else if (action == "screenshot")
    {
        TakeScreenshot();
    }
    else if (action == "toggle_gui")
    {
        m_gui->ToggleVisible();
    }
    else if (action == "toggle_preview")
    {
        if (m_previewHwnd)
        {
            BOOL visible = IsWindowVisible(m_previewHwnd);
            ShowWindow(m_previewHwnd, visible ? SW_HIDE : SW_SHOW);
            m_previewEnabled = !visible;
        }
    }
    else if (action == "toggle_hud")
    {
        m_gui->ToggleHud();
    }
    else if (action == "brightness_up")
    {
        if (m_airIMU)
        {
            int current = m_airIMU->GetBrightness();
            m_airIMU->SetBrightness(current + 10);
        }
    }
    else if (action == "brightness_down")
    {
        if (m_airIMU)
        {
            int current = m_airIMU->GetBrightness();
            m_airIMU->SetBrightness(current - 10);
        }
    }
    else if (action == "quit")
    {
        PostQuitMessage(0);
    }
}

// ============================================================
// SetPaused - 暂停/恢复渲染
// ============================================================

void Application::SetPaused(bool paused)
{
    m_renderingPaused = paused;
    if (paused)
    {
        if (m_captureMgr) m_captureMgr->Pause();
        if (m_airIMU) m_airIMU->SetPollRate(10);
    }
    else
    {
        if (m_captureMgr) m_captureMgr->Resume();
        if (m_airIMU) m_airIMU->SetPollRate(250);
    }
    if (m_gui) m_gui->SetRenderingPaused(paused);
}

// ============================================================
// TryReconnect - 断线重连
// ============================================================

void Application::TryReconnect()
{
    m_connState = ConnectionState::Reconnecting;
    LOG_INFO("尝试重连...");

    bool imuOk = false;
    bool captureOk = false;

    if (m_airIMU)
    {
        imuOk = m_airIMU->Reconnect();
    }

    if (m_captureMgr)
    {
        captureOk = m_captureMgr->ReinitAll();
    }

    // 如果没有 IMU，只看捕获状态
    if (!m_airIMU) imuOk = true;

    if (imuOk && captureOk)
    {
        m_connState = ConnectionState::Connected;
        LOG_INFO("重连成功");
    }
    else
    {
        m_connState = ConnectionState::Disconnected;
        LOG_WARN("重连失败 (IMU: %s, Capture: %s)",
                 imuOk ? "OK" : "FAIL", captureOk ? "OK" : "FAIL");
    }
}

// ============================================================
// 窗口过程（静态回调）
// ============================================================

LRESULT CALLBACK Application::StaticWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Application* app = reinterpret_cast<Application*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (app)
        return app->WndProc(hWnd, msg, wParam, lParam);
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK Application::PreviewWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
        // 隐藏而不是销毁
        ShowWindow(hWnd, SW_HIDE);
        return 0;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

LRESULT Application::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_HOTKEY:
    {
        int id = static_cast<int>(wParam);
        auto it = m_vkToAction.find(id);
        if (it != m_vkToAction.end())
        {
            HandleAction(it->second);
        }
        return 0;
    }
    case WM_TRAYICON:
    {
        switch (lParam)
        {
        case WM_RBUTTONUP:
            ShowTrayMenu();
            return 0;
        case WM_LBUTTONDBLCLK:
            HandleAction("toggle_gui");
            return 0;
        default:
            break;
        }
        return 0;
    }
    case WM_CLOSE:
    {
        // 最小化到托盘，不退出
        ShowWindow(hWnd, SW_MINIMIZE);
        ShowWindow(hWnd, SW_HIDE);
        return 0;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

// ============================================================
// CreateMainWindow - 创建主渲染窗口
// ============================================================

bool Application::CreateMainWindow()
{
    // 注册窗口类
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = StaticWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszClassName = L"NrealSpatialDisplay";
    RegisterClassExW(&wc);

    // 枚举显示器，寻找 Nreal Light（名称包含 "light"）
    bool foundNreal = false;
    DISPLAY_DEVICEW dd = {};
    dd.cb = sizeof(dd);
    DEVMODEW dm = {};

    for (DWORD i = 0; EnumDisplayDevicesW(nullptr, i, &dd, 0); ++i)
    {
        // 检查设备名称是否包含 "light"（不区分大小写）
        std::wstring devName(dd.DeviceName);
        std::wstring lower = devName;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

        if (lower.find(L"light") != std::wstring::npos)
        {
            // 找到 Nreal 显示器
            EnumDisplaySettingsW(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm);

            m_nrealDisplay.deviceName = devName;
            m_nrealDisplay.width = dm.dmPelsWidth;
            m_nrealDisplay.height = dm.dmPelsHeight;
            m_nrealDisplay.posX = dm.dmPosition.x;
            m_nrealDisplay.posY = dm.dmPosition.y;

            // 全屏窗口创建在 Nreal 显示器上
            m_hwnd = CreateWindowExW(
                WS_EX_TOPMOST,
                L"NrealSpatialDisplay",
                L"NrealSpatialDisplay",
                WS_POPUP,
                dm.dmPosition.x, dm.dmPosition.y,
                dm.dmPelsWidth, dm.dmPelsHeight,
                nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

            foundNreal = true;
            LOG_INFO("找到 Nreal 显示器: %s (%ux%u)",
                     dd.DeviceName, dm.dmPelsWidth, dm.dmPelsHeight);
            break;
        }
    }

    // 未找到 Nreal 显示器，使用窗口模式 1920x1080
    if (!foundNreal)
    {
        LOG_WARN("未找到 Nreal 显示器，使用窗口模式 1920x1080");

        m_hwnd = CreateWindowExW(
            0,
            L"NrealSpatialDisplay",
            L"NrealSpatialDisplay [窗口模式]",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            1920, 1080,
            nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

        // 居中显示
        RECT rc;
        GetWindowRect(m_hwnd, &rc);
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        SetWindowPos(m_hwnd, nullptr,
                     (screenW - (rc.right - rc.left)) / 2,
                     (screenH - (rc.bottom - rc.top)) / 2,
                     0, 0, SWP_NOSIZE | SWP_NOZORDER);
        ShowWindow(m_hwnd, SW_SHOW);
    }

    if (!m_hwnd)
    {
        LOG_ERROR("CreateWindowEx 失败");
        return false;
    }

    // 保存 this 指针到窗口用户数据
    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    // 获取实际窗口大小
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    LOG_INFO("主窗口创建完成: %dx%d", rc.right - rc.left, rc.bottom - rc.top);

    return true;
}

// ============================================================
// CreatePreviewWindow - 创建预览窗口
// ============================================================

bool Application::CreatePreviewWindow()
{
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = PreviewWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszClassName = L"NrealPreview";
    RegisterClassExW(&wc);

    m_previewHwnd = CreateWindowExW(
        0,
        L"NrealPreview",
        L"NrealSpatialDisplay - Preview",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        m_previewW, m_previewH,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    if (!m_previewHwnd)
    {
        LOG_ERROR("创建预览窗口失败");
        return false;
    }

    // 默认隐藏
    ShowWindow(m_previewHwnd, SW_HIDE);
    m_previewEnabled = false;

    LOG_INFO("预览窗口创建完成: %ux%u", m_previewW, m_previewH);
    return true;
}

// ============================================================
// TakeScreenshot - 截图保存
// ============================================================

void Application::TakeScreenshot()
{
    // 生成时间戳文件名
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
    localtime_s(&tm_buf, &time_t_now);

    std::ostringstream oss;
    oss << "screenshot_"
        << std::put_time(&tm_buf, "%Y%m%d_%H%M%S")
        << ".bmp";
    std::string filename = oss.str();

    m_renderer->SaveScreenshot(filename);
    LOG_INFO("截图: %s", filename.c_str());
}

// ============================================================
// 系统托盘
// ============================================================

void Application::CreateTrayIcon()
{
    ZeroMemory(&m_trayIcon, sizeof(m_trayIcon));
    m_trayIcon.cbSize = sizeof(NOTIFYICONDATAW);
    m_trayIcon.hWnd = m_hwnd;
    m_trayIcon.uID = 1;
    m_trayIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_trayIcon.uCallbackMessage = WM_TRAYICON;
    m_trayIcon.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(1));
    if (!m_trayIcon.hIcon)
    {
        m_trayIcon.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    }
    wcscpy_s(m_trayIcon.szTip, L"NrealSpatialDisplay");

    Shell_NotifyIconW(NIM_ADD, &m_trayIcon);
    m_trayCreated = true;
}

void Application::DestroyTrayIcon()
{
    if (m_trayCreated)
    {
        Shell_NotifyIconW(NIM_DELETE, &m_trayIcon);
        m_trayCreated = false;
    }
}

void Application::ShowTrayMenu()
{
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    HMENU hLayoutMenu = CreatePopupMenu();

    // 布局子菜单
    const char* layouts[] = { "single", "dual", "triple", "quad", "five-arc" };
    const wchar_t* layoutNames[] = { L"单屏", L"双屏", L"三屏环绕", L"四屏", L"五屏弧形" };
    for (int i = 0; i < 5; ++i)
    {
        UINT flags = MF_STRING;
        if (m_config.layout.name == layouts[i])
            flags |= MF_CHECKED;
        AppendMenuW(hLayoutMenu, flags, 100 + i, layoutNames[i]);
    }
    AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hLayoutMenu), L"切换布局");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    // 功能项
    AppendMenuW(hMenu, MF_STRING, 200, L"设置面板 (F1)");
    AppendMenuW(hMenu, MF_STRING, 201, L"预览窗口 (F3)");
    AppendMenuW(hMenu, MF_STRING, 202, L"截图 (Ctrl+Alt+S)");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    // 暂停/恢复
    AppendMenuW(hMenu, MF_STRING | (m_renderingPaused ? MF_CHECKED : 0),
                203, m_renderingPaused ? L"恢复渲染" : L"暂停渲染");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    AppendMenuW(hMenu, MF_STRING, 299, L"退出 (Ctrl+Alt+Q)");

    // 需要先 SetForegroundWindow 否则菜单不会自动关闭
    SetForegroundWindow(m_hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN,
                   pt.x, pt.y, 0, m_hwnd, nullptr);
    DestroyMenu(hMenu); // 包含子菜单也会被销毁
}

void Application::HandleTrayCommand(int id)
{
    // 布局切换 (100-104)
    if (id >= 100 && id <= 104)
    {
        const char* layouts[] = { "single", "dual", "triple", "quad", "five-arc" };
        SwitchLayout(layouts[id - 100]);
        return;
    }

    switch (id)
    {
    case 200: HandleAction("toggle_gui"); break;
    case 201: HandleAction("toggle_preview"); break;
    case 202: HandleAction("screenshot"); break;
    case 203: SetPaused(!m_renderingPaused); break;
    case 299: PostQuitMessage(0); break;
    }
}

// ============================================================
// SwitchLayout - 切换布局预设
// ============================================================

void Application::SwitchLayout(const std::string& name)
{
    // 收集当前捕获索引
    std::vector<int> captureIndices;
    for (const auto& screen : m_config.layout.screens)
    {
        captureIndices.push_back(screen.captureIndex);
    }

    // 创建新布局
    m_config.layout = LayoutEngine::CreatePreset(name, captureIndices);
    LayoutEngine::UpdateWorldMatrices(m_config.layout);

    // 重新创建曲面屏资源
    for (int i = 0; i < static_cast<int>(m_config.layout.screens.size()); ++i)
    {
        const auto& screen = m_config.layout.screens[i];
        if (screen.curvatureRad > 0.0f)
        {
            m_renderer->InitCurvedScreen(i, screen.curvatureRad, screen.curveSegments);
        }
    }

    // 更新 GUI 和托盘
    if (m_gui)
    {
        m_gui->SetLayoutName(name);
    }
    UpdateTrayTooltip();

    LOG_INFO("切换布局: %s (%d屏)", name.c_str(),
             static_cast<int>(m_config.layout.screens.size()));
}

// ============================================================
// UpdateTrayTooltip - 更新托盘提示文字
// ============================================================

void Application::UpdateTrayTooltip()
{
    std::wstring tip = L"NrealSpatialDisplay - ";
    tip += std::wstring(m_config.layout.name.begin(), m_config.layout.name.end());
    tip += L" [";
    tip += std::to_wstring(m_config.layout.screens.size());
    tip += L"屏]";

    wcscpy_s(m_trayIcon.szTip, tip.c_str());
    Shell_NotifyIconW(NIM_MODIFY, &m_trayIcon);
}

// ============================================================
// 热键注册
// ============================================================

void Application::RegisterHotkeys()
{
    for (int i = 0; i < static_cast<int>(m_hotkeys.size()); ++i)
    {
        const auto& entry = m_hotkeys[i];

        UINT mod = 0;
        if (entry.ctrl)  mod |= MOD_CONTROL;
        if (entry.alt)   mod |= MOD_ALT;
        if (entry.shift) mod |= MOD_SHIFT;

        int hkId = HOTKEY_BASE + i;
        if (RegisterHotKey(m_hwnd, hkId, mod, static_cast<UINT>(entry.key)))
        {
            m_vkToAction[hkId] = entry.id;
            m_actionToId[entry.id] = hkId;
            LOG_INFO("注册热键: %s (key=0x%02X, mod=0x%02X)", entry.id.c_str(), entry.key, mod);
        }
        else
        {
            LOG_WARN("热键注册失败: %s (可能已被占用)", entry.id.c_str());
        }
    }
}

void Application::UnregisterHotkeys()
{
    for (int i = 0; i < static_cast<int>(m_hotkeys.size()); ++i)
    {
        UnregisterHotKey(m_hwnd, HOTKEY_BASE + i);
    }
    m_vkToAction.clear();
    m_actionToId.clear();
}

int Application::GetHotkeyId(const std::string& action) const
{
    auto it = m_actionToId.find(action);
    if (it != m_actionToId.end())
        return it->second;
    return -1;
}

bool Application::HasConflict(int vk, bool ctrl, bool alt, bool shift,
                               const std::string& excludeAction) const
{
    for (const auto& entry : m_hotkeys)
    {
        if (entry.id == excludeAction) continue;
        if (entry.key == vk && entry.ctrl == ctrl &&
            entry.alt == alt && entry.shift == shift)
        {
            return true;
        }
    }
    return false;
}
