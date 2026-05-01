#pragma once
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <cstdint>
#include "../utils/TimeHelper.h"
#include "AirIMU.h"

struct hid_device_;
typedef struct hid_device_ hid_device;

class NrealLightIMU {
public:
    NrealLightIMU() = default;
    ~NrealLightIMU() { Stop(); }

    bool Start();
    void Stop();
    bool IsConnected() const { return m_connected.load(); }
    ImuData GetLatest() const;
    void SetCallback(ImuCallback cb) { m_callback = std::move(cb); }
    void SetPollRate(int hz) { if (hz > 0) m_pollIntervalMs = 1000 / hz; }
    bool HasDriver() const { return m_hasDriver.load(); }

private:
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_hasDriver{false};
    std::thread m_pollThread;
    mutable std::mutex m_mutex;
    ImuData m_latest{};
    ImuCallback m_callback;
    int m_pollIntervalMs = 4;

    void PollLoop();
    bool TryConnectDevice();

    hid_device* m_ov580Dev = nullptr;
    hid_device* m_mcuDev = nullptr;

    static constexpr unsigned short OV580_VID = 0x05A9;
    static constexpr unsigned short OV580_PID = 0x0680;
    static constexpr unsigned short MCU_VID = 0x0486;
    static constexpr unsigned short MCU_PID = 0x573c;
};