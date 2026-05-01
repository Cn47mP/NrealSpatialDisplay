#pragma once
#include <DirectXMath.h>
#include "../imu/AirIMU.h"
#include "../imu/ImuFilter.h"

using namespace DirectX;

enum class CameraMode { WorldLock, HeadLock };

class Camera {
public:
    Camera() = default;

    void UpdateFromIMU(const ImuData& imu) {
        XMVECTOR rawQuat = XMVectorSet(imu.quatX, imu.quatY, imu.quatZ, imu.quatW);
        XMVECTOR filtered = m_filter.UpdateQuaternion(rawQuat);

        if (m_mode == CameraMode::WorldLock) {
            m_rotation = filtered;
        } else {
            m_rotation = m_filter.GetHeadLockDelta(filtered);
        }
    }

    void SetRotation(float pitch, float yaw, float roll) {
        float p = pitch * XM_PI / 180.0f;
        float y = yaw * XM_PI / 180.0f;
        float r = roll * XM_PI / 180.0f;
        m_rotation = XMQuaternionRotationRollPitchYaw(p, y, r);
    }

    XMMATRIX GetViewMatrix() const {
        XMVECTOR forward = XMVector3Rotate(XMVectorSet(0, 0, -1, 0), m_rotation);
        XMVECTOR up = XMVector3Rotate(XMVectorSet(0, 1, 0, 0), m_rotation);
        return XMMatrixLookAtLH(m_position, XMVectorAdd(m_position, forward), up);
    }

    void SetMode(CameraMode mode) {
        if (m_mode != mode) {
            m_mode = mode;
            if (mode == CameraMode::HeadLock) {
                m_filter.LockCurrentOrientation(m_rotation);
            } else {
                m_filter.UnlockOrientation();
            }
        }
    }

    void ToggleMode() {
        SetMode(m_mode == CameraMode::WorldLock ? CameraMode::HeadLock : CameraMode::WorldLock);
    }

    CameraMode GetMode() const { return m_mode; }

    void ResetYawZero() {
        m_filter.ResetZeroReference(m_filter.GetEulerDeg().y);
    }

    void EnableAutoDriftComp(bool enable) { m_filter.EnableAutoDriftCompensation(enable); }

    void SetPosition(XMVECTOR pos) { m_position = pos; }
    XMVECTOR GetPosition() const { return m_position; }

    ImuFilter& GetFilter() { return m_filter; }
    const ImuFilter& GetFilter() const { return m_filter; }

    XMFLOAT3 GetEulerDeg() const { return m_filter.GetEulerDeg(); }

private:
    CameraMode m_mode = CameraMode::WorldLock;
    ImuFilter m_filter;
    XMVECTOR m_rotation = XMVectorSet(0, 0, 0, 1);
    XMVECTOR m_position = XMVectorSet(0, 0, 0, 0);
};