
#pragma once
#include <DirectXMath.h>
#include <cmath>
#include <algorithm>

using namespace DirectX;

class ImuFilter {
public:
    ImuFilter() = default;

    void SetSmoothing(float s) { m_smoothing = std::clamp(s, 0.0f, 1.0f); }
    float GetSmoothing() const { return m_smoothing; }

    void SetDeadzone(float deg) { m_deadzoneDeg = deg; }
    float GetDeadzone() const { return m_deadzoneDeg; }

    void EnableAutoDriftCompensation(bool enable) { m_autoDrift = enable; }
    bool IsAutoDriftEnabled() const { return m_autoDrift; }

    void ResetZeroReference(float currentYawDeg) {
        m_yawOffset = currentYawDeg;
        m_driftAccum = 0.0f;
    }

    void LockCurrentOrientation(XMVECTOR currentQuat) {
        m_lockedQuat = XMQuaternionNormalize(currentQuat);
        m_locked = true;
    }

    void UnlockOrientation() {
        m_locked = false;
    }

    bool IsLocked() const { return m_locked; }

    XMVECTOR GetHeadLockDelta(XMVECTOR currentQuat) {
        XMVECTOR invLocked = XMQuaternionConjugate(m_lockedQuat);
        return XMQuaternionMultiply(invLocked, currentQuat);
    }

    XMVECTOR UpdateQuaternion(XMVECTOR rawQuat) {
        rawQuat = XMQuaternionNormalize(rawQuat);

        if (!m_initialized) {
            m_filtered = rawQuat;
            m_initialized = true;
            return m_filtered;
        }

        float dot = XMVectorGetX(XMVector4Dot(m_filtered, rawQuat));
        if (dot < 0.0f) {
            rawQuat = XMVectorNegate(rawQuat);
            dot = -dot;
        }

        float t = 1.0f - m_smoothing;
        m_filtered = XMVectorLerp(m_filtered, rawQuat, t);
        m_filtered = XMQuaternionNormalize(m_filtered);

        if (m_autoDrift) {
            float currentYaw = QuaternionToYawDeg(m_filtered);
            float delta = currentYaw - m_lastYaw;
            if (delta > 180.0f) delta -= 360.0f;
            if (delta < -180.0f) delta += 360.0f;
            m_driftAccum = m_driftAccum * 0.999f + delta * 0.001f;
            m_yawOffset += m_driftAccum * m_driftRate;
            m_lastYaw = currentYaw;
        }

        return m_filtered;
    }

    float GetCorrectedYaw() const {
        float yaw = QuaternionToYawDeg(m_filtered);
        yaw -= m_yawOffset;
        while (yaw > 180.0f) yaw -= 360.0f;
        while (yaw < -180.0f) yaw += 360.0f;
        return yaw;
    }

    XMFLOAT3 GetEulerDeg() const {
        XMFLOAT4 q;
        XMStoreFloat4(&q, m_filtered);

        float sinp = 2.0f * (q.w * q.x + q.y * q.z);
        float cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
        float pitch = std::atan2(sinp, cosp) * 180.0f / XM_PI;

        float yaw = GetCorrectedYaw();

        float sinr = 2.0f * (q.w * q.z + q.x * q.y);
        float cosr = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
        float roll = std::atan2(sinr, cosr) * 180.0f / XM_PI;

        return { pitch, yaw, roll };
    }

    void Reset() {
        m_initialized = false;
        m_locked = false;
        m_yawOffset = 0.0f;
        m_driftAccum = 0.0f;
        m_lastYaw = 0.0f;
    }

private:
    float m_smoothing = 0.15f;
    float m_deadzoneDeg = 0.5f;
    bool m_autoDrift = false;
    float m_driftRate = 0.001f;

    XMVECTOR m_filtered = XMVectorSet(0, 0, 0, 1);
    bool m_initialized = false;

    float m_yawOffset = 0.0f;
    float m_driftAccum = 0.0f;
    float m_lastYaw = 0.0f;

    bool m_locked = false;
    XMVECTOR m_lockedQuat = XMVectorSet(0, 0, 0, 1);

    static float QuaternionToYawDeg(XMVECTOR q) {
        XMFLOAT4 v;
        XMStoreFloat4(&v, q);
        float siny = 2.0f * (v.w * v.y + v.x * v.z);
        float cosy = 1.0f - 2.0f * (v.y * v.y + v.x * v.x);
        return std::atan2(siny, cosy) * 180.0f / XM_PI;
    }

    static float ExtractYawDelta(XMVECTOR a, XMVECTOR b) {
        float ya = QuaternionToYawDeg(a);
        float yb = QuaternionToYawDeg(b);
        float d = yb - ya;
        if (d > 180.0f) d -= 360.0f;
        if (d < -180.0f) d += 360.0f;
        return d;
    }
};
