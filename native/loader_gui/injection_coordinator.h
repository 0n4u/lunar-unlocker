#pragma once

#include "auto_inject_readiness.h"

#include <cstdint>
#include <string>

struct InjectionOutcome {
    bool success = false;
    std::wstring error;
    bool remoteThreadStarted = false;
    bool retrySafe = true;
};

class InjectionCoordinator {
public:
    static InjectionOutcome injectProductDll(const ProcessIdentity& identity,
        const std::wstring& dllPath, std::uint16_t controllerPort,
        const std::string& serviceHttpBase);

    static bool injectProductDll(std::uint32_t processId,
        const std::wstring& dllPath, std::uint16_t controllerPort,
        const std::string& serviceHttpBase, std::wstring& error);

    static InjectionOutcome injectReflectiveDll(const ProcessIdentity& identity,
        const std::wstring& dllPath, std::uint16_t controllerPort);

    static bool injectReflectiveDll(std::uint32_t processId,
        const std::wstring& dllPath, std::uint16_t controllerPort,
        std::wstring& error);
};
