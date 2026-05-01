#include "NrealLightIMU.h"
#include "../utils/Log.h"
#include <hidapi.h>
#include <cmath>
#include <cstring>
#include <cstdio>

// CRC32 lookup table from ar-drivers-rs
static const uint32_t CRC32_TABLE[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535,
    0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd,
    0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d,
    0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
    0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4,
    0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac,
    0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab,
    0xb6662d3d, 0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f,
    0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb,
    0x086d3d2d, 0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea,
    0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65, 0x4db26158, 0x3ab551ce,
    0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a,
    0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409,
    0xce61e49f, 0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739,
    0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
    0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344, 0x8708a3d2, 0x1e01f268,
    0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0,
    0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8,
    0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef,
    0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703,
    0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7,
    0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
    0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae,
    0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777, 0x88085ae6,
    0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d,
    0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5,
    0x47b2cf7f, 0x30b5ffe9, 0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605,
    0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d,
};

static uint32_t crc32(const uint8_t* data, size_t len) {
    uint32_t r = 0xffffffffu;
    for (size_t i = 0; i < len; i++) {
        r = (r >> 8) ^ CRC32_TABLE[(data[i] ^ (r & 0xff))];
    }
    return r ^ 0xffffffffu;
}

// Serialize MCU packet: 2:category:cmd_id:data:timestamp:crc:3
// Returns packet size (should be 64)
static int SerializeMCUPacket(uint8_t category, uint8_t cmd_id, const char* data,
                               unsigned char* out, size_t outSize) {
    if (outSize < 64) return 0;
    
    // Build packet without CRC: 2:category:cmd_id:data:0:
    char buf[64];
    int n = snprintf(buf, sizeof(buf), "2:%c:%c:%s:0:", category, cmd_id, data ? data : "x");
    if (n <= 0 || n >= (int)sizeof(buf)) return 0;
    
    // Calculate CRC32 of the packet so far (excluding the trailing colon for CRC)
    uint32_t c = crc32((const uint8_t*)buf, n - 1); // exclude trailing ':'
    
    // Append CRC as 8-digit hex, right-aligned (same format as Rust)
    char crcBuf[16];
    snprintf(crcBuf, sizeof(crcBuf), "%08x", c);
    
    // Build final packet
    n = snprintf((char*)out, outSize, "%s%s:3", buf, crcBuf);
    
    // Pad to 64 bytes with zeros
    if (n > 0 && (size_t)n < outSize) {
        memset(out + n, 0, outSize - n);
    }
    return (n > 0) ? (int)outSize : 0;
}

// Send command to OV580 and wait for 0x02 response
static bool SendOV580Command(hid_device* dev, uint8_t cmd, uint8_t subcmd, unsigned char* outResponse, int& outSize) {
    unsigned char packet[] = { 2, cmd, subcmd, 0, 0, 0, 0 };
    int written = hid_write(dev, packet, sizeof(packet));
    if (written < 0) return false;
    
    for (int i = 0; i < 64; i++) {
        outSize = hid_read_timeout(dev, outResponse, 128, 250);
        if (outSize > 0 && outResponse[0] == 0x02) {
            return true;
        }
    }
    return false;
}

// Send MCU command packet
static bool SendMCUPacket(hid_device* dev, uint8_t category, uint8_t cmd_id, const char* data) {
    if (!dev) return false;
    unsigned char packet[64];
    int size = SerializeMCUPacket(category, cmd_id, data, packet, sizeof(packet));
    if (size <= 0) return false;
    int written = hid_write(dev, packet, size);
    return written == (int)size;
}

bool NrealLightIMU::TryConnectDevice() {
    int res = hid_init();
    if (res != 0) {
        LOG_WARN("NrealLight: hid_init failed");
        return false;
    }

    // Open MCU (0486:573c) for heartbeat
    struct hid_device_info* mcu_devs = hid_enumerate(MCU_VID, MCU_PID);
    if (mcu_devs) {
        LOG_INFO("NrealLight: Found MCU device");
        m_mcuDev = hid_open_path(mcu_devs->path);
        if (m_mcuDev) {
            LOG_INFO("NrealLight: MCU opened successfully");
            hid_set_nonblocking(m_mcuDev, 1);
        }
        hid_free_enumeration(mcu_devs);
    }

    // Open OV580 (05A9:0680) for IMU data
    struct hid_device_info* devs = hid_enumerate(OV580_VID, OV580_PID);
    if (!devs) {
        LOG_WARN("NrealLight: No OV580 device found");
        hid_exit();
        return false;
    }

    struct hid_device_info* cur = devs;
    while (cur) {
        LOG_INFO("NrealLight: Found OV580 at path=%s iface=%d usage=%04x:%04x",
            cur->path, cur->interface_number, cur->usage_page, cur->usage);

        m_ov580Dev = hid_open_path(cur->path);
        if (m_ov580Dev) {
            LOG_INFO("NrealLight: Opened OV580 device successfully");
            hid_set_nonblocking(m_ov580Dev, 0); // blocking for init

            unsigned char resp[128];
            int respSize = 0;

            // 1. Disable IMU stream
            if (!SendOV580Command(m_ov580Dev, 0x19, 0x00, resp, respSize)) {
                LOG_WARN("NrealLight: Failed to disable IMU");
            } else {
                LOG_INFO("NrealLight: IMU disabled OK");
            }

            // 2. Read config (simplified)
            if (SendOV580Command(m_ov580Dev, 0x14, 0x00, resp, respSize)) {
                LOG_INFO("NrealLight: Config read started");
                for (int i = 0; i < 100; i++) {
                    if (!SendOV580Command(m_ov580Dev, 0x15, 0x00, resp, respSize)) break;
                    if (respSize < 3 || resp[1] != 0x01) break;
                }
            }

            // 3. Enable IMU stream
            if (!SendOV580Command(m_ov580Dev, 0x19, 0x01, resp, respSize)) {
                LOG_WARN("NrealLight: Failed to enable IMU");
                hid_close(m_ov580Dev);
                m_ov580Dev = nullptr;
            } else {
                LOG_INFO("NrealLight: IMU enabled OK, response size=%d", respSize);
                hid_set_nonblocking(m_ov580Dev, 1); // non-blocking for poll loop
                hid_free_enumeration(devs);
                m_hasDriver = true;
                m_connected = true;
                return true;
            }
        } else {
            LOG_WARN("NrealLight: Failed to open path=%s", cur->path);
        }
        cur = cur->next;
    }

    hid_free_enumeration(devs);
    hid_exit();
    LOG_WARN("NrealLight: Could not open any interface");
    return false;
}

bool NrealLightIMU::Start() {
    if (m_running) return true;

    LOG_INFO("NrealLight: Starting...");

    if (!TryConnectDevice()) {
        LOG_WARN("NrealLight: Not connected, NOT using simulation");
        return false;
    }

    m_running = true;
    m_pollThread = std::thread(&NrealLightIMU::PollLoop, this);
    return true;
}

void NrealLightIMU::Stop() {
    if (!m_running) return;

    m_running = false;
    if (m_pollThread.joinable()) {
        m_pollThread.join();
    }

    if (m_ov580Dev) {
        unsigned char disable_imu[] = { 2, 0x19, 0x00, 0, 0, 0, 0 };
        hid_write(m_ov580Dev, disable_imu, sizeof(disable_imu));
        hid_close(m_ov580Dev);
        m_ov580Dev = nullptr;
    }

    if (m_mcuDev) {
        hid_close(m_mcuDev);
        m_mcuDev = nullptr;
    }

    hid_exit();
    m_connected = false;
    m_hasDriver = false;
    LOG_INFO("NrealLight: Stopped");
}

ImuData NrealLightIMU::GetLatest() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_latest;
}

void NrealLightIMU::PollLoop() {
    LOG_INFO("NrealLight: PollLoop starting");

    unsigned char buffer[128];
    static bool firstPacket = true;
    int packetCount = 0;
    auto lastHeartbeat = std::chrono::steady_clock::now();

    while (m_running) {
        // Send MCU heartbeat every 250ms
        if (m_mcuDev) {
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastHeartbeat).count() > 250) {
                SendMCUPacket(m_mcuDev, '@', 'K', "x");
                lastHeartbeat = now;
            }
        }

        if (m_ov580Dev) {
            int res = hid_read_timeout(m_ov580Dev, buffer, sizeof(buffer), 100);
            if (res > 0) {
                packetCount++;
                if (firstPacket) {
                    LOG_INFO("NrealLight: First packet received, size=%d, byte0=0x%02x", res, buffer[0]);
                    firstPacket = false;
                }
                
                if (buffer[0] == 0x01 && res >= 64) {
                    uint64_t gyro_ts = *reinterpret_cast<uint64_t*>(&buffer[44]);
                    uint32_t gyro_mul = *reinterpret_cast<uint32_t*>(&buffer[52]);
                    uint32_t gyro_div = *reinterpret_cast<uint32_t*>(&buffer[56]);
                    int32_t gyro_x = *reinterpret_cast<int32_t*>(&buffer[60]);
                    int32_t gyro_y = *reinterpret_cast<int32_t*>(&buffer[64]);
                    int32_t gyro_z = *reinterpret_cast<int32_t*>(&buffer[68]);

                    uint64_t acc_ts = *reinterpret_cast<uint64_t*>(&buffer[72]);
                    uint32_t acc_mul = *reinterpret_cast<uint32_t*>(&buffer[80]);
                    uint32_t acc_div = *reinterpret_cast<uint32_t*>(&buffer[84]);
                    int32_t acc_x = *reinterpret_cast<int32_t*>(&buffer[88]);
                    int32_t acc_y = *reinterpret_cast<int32_t*>(&buffer[92]);
                    int32_t acc_z = *reinterpret_cast<int32_t*>(&buffer[96]);

                    float gx = (gyro_x * (float)gyro_mul / (float)gyro_div) * 3.14159265f / 180.0f;
                    float gy = -(gyro_y * (float)gyro_mul / (float)gyro_div) * 3.14159265f / 180.0f;
                    float gz = -(gyro_z * (float)gyro_mul / (float)gyro_div) * 3.14159265f / 180.0f;

                    float ax = (acc_x * (float)acc_mul / (float)acc_div) * 9.81f;
                    float ay = -(acc_y * (float)acc_mul / (float)acc_div) * 9.81f;
                    float az = -(acc_z * (float)acc_mul / (float)acc_div) * 9.81f;

                    static float pitch = 0, roll = 0, yaw = 0;
                    pitch += gx * 0.004f;
                    roll += gy * 0.004f;
                    yaw += gz * 0.004f;

                    float accel_pitch = atan2f(ax, sqrtf(ay*ay + az*az)) * 180.0f / 3.14159265f;
                    float accel_roll = atan2f(ay, az) * 180.0f / 3.14159265f;

                    pitch = pitch * 0.98f + accel_pitch * 0.02f;
                    roll = roll * 0.98f + accel_roll * 0.02f;

                    float cr = cosf(roll * 0.5f * 3.14159265f / 180.0f);
                    float sr = sinf(roll * 0.5f * 3.14159265f / 180.0f);
                    float cp = cosf(pitch * 0.5f * 3.14159265f / 180.0f);
                    float sp = sinf(pitch * 0.5f * 3.14159265f / 180.0f);
                    float cy = cosf(yaw * 0.5f * 3.14159265f / 180.0f);
                    float sy = sinf(yaw * 0.5f * 3.14159265f / 180.0f);

                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_latest.quatX = sr * cp * cy - cr * sp * sy;
                    m_latest.quatY = cr * sp * cy + sr * cp * sy;
                    m_latest.quatZ = cr * cp * sy - sr * sp * cy;
                    m_latest.quatW = cr * cp * cy + sr * sp * sy;
                    m_latest.pitch = pitch;
                    m_latest.roll = roll;
                    m_latest.yaw = yaw;
                    m_latest.timestampUs = GetTimeUs();

                    m_connected = true;
                    if (m_callback) {
                        m_callback(m_latest);
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(m_pollIntervalMs));
    }

    LOG_INFO("NrealLight: PollLoop exiting, total packets=%d", packetCount);
}
