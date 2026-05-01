
#pragma once
#include <windows.h>
#include <wrl/client.h>
#include <string>
#include <stdexcept>

using Microsoft::WRL::ComPtr;

#define HR_CHECK(hr) \
    do { HRESULT _hr = (hr); if (FAILED(_hr)) { \
        throw std::runtime_error("HRESULT failed: 0x" + \
            std::to_string(_hr) + " at " __FILE__ ":" + std::to_string(__LINE__)); \
    }} while(0)

#define HR_CHECK_RET(hr) \
    do { if (FAILED(hr)) return false; } while(0)

class ComInitializer {
public:
    ComInitializer() {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
            throw std::runtime_error("CoInitializeEx failed");
        }
    }
    ~ComInitializer() { CoUninitialize(); }
    ComInitializer(const ComInitializer&) = delete;
    ComInitializer& operator=(const ComInitializer&) = delete;
};
