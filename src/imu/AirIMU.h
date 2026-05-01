#pragma once
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <cstdint>
#include <windows.h>
#include "../utils/TimeHelper.h"

struct ImuData {
    float quatX, quatY, quatZ, quatW;
    float pitch, roll, yaw;
    uint64_t timestampUs;
};

using ImuCallback = std::function<void(const ImuData&)>;
using DisconnectCallback = std::function<void()>;

class AirIMU {
public:
    AirIMU() = default;
    ~AirIMU() { Stop(); }

    bool Start();
    void Stop();
    bool IsConnected() const { return m_connected; }
    ImuData GetLatest() const;
    void SetCallback(ImuCallback cb) { m_callback = std::move(cb); }
    void SetDisconnectCallback(DisconnectCallback cb) { m_disconnectCallback = std::move(cb); }
    int GetBrightness();
    bool SetBrightness(int level);
    bool Reconnect();
    void SetPollRate(int hz);

    bool HasAirAPI() const { return m_hasAirAPI; }

private:
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_connected{false};
    std::thread m_pollThread;
    mutable std::mutex m_mutex;
    ImuData m_latest{};
    ImuCallback m_callback;
    DisconnectCallback m_disconnectCallback;
    bool m_hasAirAPI = false;
    int m_pollIntervalMs = 4;

    void PollLoop();
    bool TryConnectAirAPI();
    bool TryLoadAirAPIDLL();

    HMODULE m_airAPIDll = nullptr;
    HMODULE m_hidapiDll = nullptr;

    typedef int(*StartConnection_t)();
    typedef int(*StopConnection_t)();
    typedef float*(*GetQuaternion_t)();
    typedef float*(*GetEuler_t)();
    typedef int(*GetBrightnessAPI_t)();

    StartConnection_t StartConnection = nullptr;
    StopConnection_t StopConnection = nullptr;
    GetQuaternion_t GetQuaternion = nullptr;
    GetEuler_t GetEuler = nullptr;
    GetBrightnessAPI_t AirAPI_GetBrightness = nullptr;
};