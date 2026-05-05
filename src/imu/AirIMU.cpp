#include "AirIMU.h"
#include "../utils/Log.h"

bool AirIMU::TryLoadAirAPIDLL() {
    const char* paths[] = {
        "AirAPI_Windows.dll",
        ".\\AirAPI_Windows.dll",
        "..\\airapi\\AirAPI_Windows.dll",
        "airapi\\AirAPI_Windows.dll",
        "C:\\Users\\Cn47mP\\Desktop\\NrealSpatialDisplay\\airapi\\AirAPI_Windows.dll"
    };

    for (const char* path : paths) {
        m_airAPIDll = LoadLibraryA(path);
        if (m_airAPIDll) {
            LOG_INFO("AirIMU: Loaded DLL from: %s", path);
            break;
        }
    }

    if (!m_airAPIDll) {
        LOG_WARN("AirIMU: AirAPI_Windows.dll not found");
        return false;
    }

    StartConnection = (StartConnection_t)GetProcAddress(m_airAPIDll, "StartConnection");
    StopConnection = (StopConnection_t)GetProcAddress(m_airAPIDll, "StopConnection");
    GetQuaternion = (GetQuaternion_t)GetProcAddress(m_airAPIDll, "GetQuaternion");
    GetEuler = (GetEuler_t)GetProcAddress(m_airAPIDll, "GetEuler");

    if (!StartConnection || !StopConnection || !GetQuaternion) {
        LOG_ERROR("AirIMU: Failed to get AirAPI functions");
        FreeLibrary(m_airAPIDll);
        m_airAPIDll = nullptr;
        return false;
    }

    LOG_INFO("AirIMU: AirAPI functions resolved");
    return true;
}

bool AirIMU::TryConnectAirAPI() {
    if (!m_hasAirAPI) {
        if (!TryLoadAirAPIDLL()) {
            return false;
        }
        m_hasAirAPI = true;
    }

    LOG_INFO("AirIMU: Attempting connection...");

    if (StartConnection) {
        int result = StartConnection();
        LOG_INFO("AirIMU: StartConnection() returned %d", result);
        if (result < 0) {
            LOG_WARN("AirIMU: StartConnection failed with code %d", result);
            m_connected = false;
            return false;
        }
    }

    m_connected = true;
    LOG_INFO("AirIMU: Connection established");
    return true;
}

bool AirIMU::Start() {
    if (m_running) return true;

    LOG_INFO("AirIMU: Starting...");

    TryConnectAirAPI();

    if (m_connected) {
        LOG_INFO("AirIMU: Connected to Nreal device");
    } else {
        LOG_WARN("AirIMU: AirAPI not available, using simulation mode");
    }

    m_running = true;
    m_pollThread = std::thread(&AirIMU::PollLoop, this);
    return true;
}

void AirIMU::Stop() {
    if (!m_running) return;

    m_running = false;
    if (m_pollThread.joinable()) {
        m_pollThread.join();
    }

    if (m_hasAirAPI && StopConnection) {
        StopConnection();
    }

    if (m_airAPIDll) {
        FreeLibrary(m_airAPIDll);
        m_airAPIDll = nullptr;
    }

    m_connected = false;
    m_hasAirAPI = false;
    LOG_INFO("AirIMU: Stopped");
}

ImuData AirIMU::GetLatest() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_latest;
}

void AirIMU::PollLoop() {
    LOG_INFO("PollLoop: starting");
    int failCount = 0;
    while (m_running) {
        ImuData data = {};
        bool success = false;

        if (m_hasAirAPI && GetQuaternion && GetEuler) {
            float* quat = GetQuaternion();
            float* euler = GetEuler();

            if (quat && euler) {
                data.quatX = quat[0];
                data.quatY = quat[1];
                data.quatZ = quat[2];
                data.quatW = quat[3];
                data.pitch = euler[0];
                data.roll = euler[1];
                data.yaw = euler[2];
                data.timestampUs = GetTimeUs();
                success = true;
                failCount = 0;

                if (!m_connected) {
                    m_connected = true;
                    LOG_INFO("PollLoop: Connection restored");
                }
            }
        }

        if (!success) {
            failCount++;
            // Only mark disconnected after sustained failures (avoid false positives on first few reads)
            if (m_connected && failCount > 60) {
                m_connected = false;
                LOG_WARN("PollLoop: Connection lost (after %d consecutive failures)", failCount);
                if (m_disconnectCallback) {
                    m_disconnectCallback();
                }
                if (m_hasAirAPI && StopConnection) {
                    StopConnection();
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_latest = data;
        }

        if (m_callback && success) {
            m_callback(data);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(m_pollIntervalMs));
    }
    LOG_INFO("PollLoop: exiting");
}

int AirIMU::GetBrightness() {
    if (m_hasAirAPI && AirAPI_GetBrightness) {
        return AirAPI_GetBrightness();
    }
    return 50;
}

bool AirIMU::SetBrightness(int level) {
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
}

bool AirIMU::Reconnect() {
    LOG_INFO("AirIMU: Reconnecting...");
    Stop();
    return Start();
}

void AirIMU::SetPollRate(int hz) {
    if (hz > 0) {
        m_pollIntervalMs = 1000 / hz;
    }
}