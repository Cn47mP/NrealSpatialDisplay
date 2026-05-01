#pragma once
#include <atomic>
#include <mutex>
#include <string>
#include <cstddef>

class UnityStream {
public:
    UnityStream();
    ~UnityStream();

    bool Start(const std::string& address = "127.0.0.1", int port = 4242);
    void Stop();
    bool IsRunning() const { return m_running.load(); }

    void SendImuData(float quatX, float quatY, float quatZ, float quatW,
                     float pitch, float yaw, float roll, uint64_t timestampUs);
    void SendJson(const std::string& json);
    void SetBroadcast(bool enable);

private:
    void NetworkLoop();
    int InitWinsock();
    void CleanupWinsock();

    size_t m_socket = 0;
    std::atomic<bool> m_running{false};
    std::thread m_thread;
    mutable std::mutex m_mutex;

    std::string m_buffer;
    bool m_hasNewData = false;
    bool m_broadcast = false;
    unsigned long m_targetAddr = 0;
    unsigned short m_targetPort = 0;
};