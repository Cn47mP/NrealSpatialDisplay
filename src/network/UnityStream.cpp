#include "UnityStream.h"
#include "../utils/Log.h"
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

UnityStream::UnityStream() {}

UnityStream::~UnityStream() {
    Stop();
}

int UnityStream::InitWinsock() {
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData);
}

void UnityStream::CleanupWinsock() {
    WSACleanup();
}

bool UnityStream::Start(const std::string& address, int port) {
    if (InitWinsock() != 0) {
        LOG_ERROR("UnityStream: Failed to init Winsock");
        return false;
    }

    m_socket = (size_t)::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == 0 || m_socket == (size_t)-1) {
        LOG_ERROR("UnityStream: socket creation failed");
        CleanupWinsock();
        return false;
    }

    if (m_broadcast) {
        int broadcast = 1;
        ::setsockopt((SOCKET)m_socket, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast));
    }

    m_targetAddr = htonl(0x7F000001);
    m_targetPort = htons((unsigned short)port);

    m_running = true;
    m_thread = std::thread(&UnityStream::NetworkLoop, this);

    LOG_INFO("UnityStream: Started UDP stream to %s:%d", address.c_str(), port);
    return true;
}

void UnityStream::Stop() {
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
    if (m_socket != 0 && m_socket != (size_t)-1) {
        ::closesocket((SOCKET)m_socket);
        m_socket = 0;
    }
    CleanupWinsock();
    LOG_INFO("UnityStream: Stopped");
}

void UnityStream::SendImuData(float qx, float qy, float qz, float qw,
                              float pitch, float yaw, float roll, uint64_t timestampUs) {
    char packet[128];
    size_t offset = 0;
    packet[offset++] = 'I';

    memcpy(packet + offset, &qx, sizeof(float)); offset += sizeof(float);
    memcpy(packet + offset, &qy, sizeof(float)); offset += sizeof(float);
    memcpy(packet + offset, &qz, sizeof(float)); offset += sizeof(float);
    memcpy(packet + offset, &qw, sizeof(float)); offset += sizeof(float);
    memcpy(packet + offset, &pitch, sizeof(float)); offset += sizeof(float);
    memcpy(packet + offset, &yaw, sizeof(float)); offset += sizeof(float);
    memcpy(packet + offset, &roll, sizeof(float)); offset += sizeof(float);
    memcpy(packet + offset, &timestampUs, sizeof(uint64_t)); offset += sizeof(uint64_t);

    std::lock_guard<std::mutex> lock(m_mutex);
    m_buffer.assign(packet, offset);
    m_hasNewData = true;
}

void UnityStream::SendJson(const std::string& json) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_buffer = json;
    m_hasNewData = true;
}

void UnityStream::SetBroadcast(bool enable) {
    m_broadcast = enable;
}

void UnityStream::NetworkLoop() {
    while (m_running) {
        std::string dataToSend;
        bool hasData = false;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_hasNewData) {
                dataToSend = m_buffer;
                m_hasNewData = false;
                hasData = true;
            }
        }

        if (hasData && m_socket != 0 && m_socket != (size_t)-1) {
            struct sockaddr_in dest;
            dest.sin_family = AF_INET;
            dest.sin_port = m_targetPort;
            dest.sin_addr.s_addr = m_targetAddr;
            memset(dest.sin_zero, 0, sizeof(dest.sin_zero));

            sendto((SOCKET)m_socket, dataToSend.c_str(), (int)dataToSend.size(), 0,
                   (sockaddr*)&dest, sizeof(dest));
        }

        Sleep(2);
    }
}