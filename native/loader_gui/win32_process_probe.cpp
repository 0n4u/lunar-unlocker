#include "win32_process_probe.h"

#include <windows.h>
#include <tlhelp32.h>
#include <winternl.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cwchar>
#include <limits>
#include <unordered_map>
#include <vector>

namespace {
using NtQueryInformationProcessFunction = NTSTATUS(NTAPI*)(
    HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
constexpr PROCESSINFOCLASS CommandLineInformationClass =
    static_cast<PROCESSINFOCLASS>(60);
constexpr UINT ResponsivenessTimeoutMs = 75;

std::uint64_t fileTimeValue(const FILETIME& value) noexcept {
    ULARGE_INTEGER result{};
    result.LowPart = value.dwLowDateTime;
    result.HighPart = value.dwHighDateTime;
    return result.QuadPart;
}

bool identityFromHandle(HANDLE process, std::uint32_t processId,
        ProcessIdentity& identity) noexcept {
    FILETIME creation{};
    FILETIME exit{};
    FILETIME kernel{};
    FILETIME user{};
    if (process == nullptr || process == INVALID_HANDLE_VALUE
            || !GetProcessTimes(process, &creation, &exit, &kernel, &user)) {
        return false;
    }
    const std::uint64_t creationTime = fileTimeValue(creation);
    if (processId == 0 || creationTime == 0) {
        return false;
    }
    identity = {processId, creationTime};
    return true;
}

bool processStillActive(HANDLE process) noexcept {
    DWORD exitCode = 0;
    return GetExitCodeProcess(process, &exitCode) != FALSE
        && exitCode == STILL_ACTIVE;
}

bool queryCommandLine(HANDLE process, std::wstring& commandLine) {
    commandLine.clear();
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    const auto query = ntdll == nullptr ? nullptr
        : reinterpret_cast<NtQueryInformationProcessFunction>(
            GetProcAddress(ntdll, "NtQueryInformationProcess"));
    if (query == nullptr) {
        return false;
    }

    ULONG required = 0;
    query(process, CommandLineInformationClass, nullptr, 0, &required);
    if (required < sizeof(UNICODE_STRING)
            || required > 16U * 1024U * 1024U) {
        return false;
    }

    std::vector<unsigned char> buffer(
        static_cast<std::size_t>(required) + sizeof(wchar_t), 0);
    ULONG returned = 0;
    const NTSTATUS status = query(process, CommandLineInformationClass,
        buffer.data(), required, &returned);
    if (status < 0) {
        return false;
    }

    const auto* text = reinterpret_cast<const UNICODE_STRING*>(buffer.data());
    if (text->Buffer == nullptr || (text->Length % sizeof(wchar_t)) != 0) {
        return false;
    }
    const auto begin = reinterpret_cast<std::uintptr_t>(buffer.data());
    const auto end = begin + buffer.size();
    const auto textBegin = reinterpret_cast<std::uintptr_t>(text->Buffer);
    const auto textEnd = textBegin + text->Length;
    if (textBegin < begin || textEnd < textBegin || textEnd > end) {
        return false;
    }

    commandLine.assign(text->Buffer,
        static_cast<std::size_t>(text->Length) / sizeof(wchar_t));
    return true;
}

bool javaExecutable(const wchar_t* executable) noexcept {
    return executable != nullptr
        && (_wcsicmp(executable, L"java.exe") == 0
            || _wcsicmp(executable, L"javaw.exe") == 0);
}

struct WindowScanContext {
    std::vector<ProcessObservation>* observations = nullptr;
    std::unordered_map<std::uint32_t, std::size_t> byPid;
};

BOOL CALLBACK captureWindow(HWND window, LPARAM parameter) {
    auto& context = *reinterpret_cast<WindowScanContext*>(parameter);
    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    const auto found = context.byPid.find(processId);
    if (found == context.byPid.end()) {
        return TRUE;
    }

    ProcessObservation& observation = (*context.observations)[found->second];
    const bool visible = IsWindowVisible(window) != FALSE;
    const int titleLength = GetWindowTextLengthW(window);
    if ((!visible && observation.visible)
            || (titleLength <= 0 && !observation.title.empty())) {
        return TRUE;
    }

    std::wstring title;
    if (titleLength > 0) {
        title.resize(static_cast<std::size_t>(titleLength) + 1);
        const int copied = GetWindowTextW(window, title.data(),
            static_cast<int>(title.size()));
        title.resize(copied > 0 ? static_cast<std::size_t>(copied) : 0);
    }

    wchar_t className[256]{};
    const int classLength = GetClassNameW(window, className,
        static_cast<int>(std::size(className)));

    bool responsive = false;
    if (visible && !title.empty() && !IsHungAppWindow(window)) {
        DWORD_PTR ignored = 0;
        responsive = SendMessageTimeoutW(window, WM_NULL, 0, 0,
            SMTO_ABORTIFHUNG | SMTO_BLOCK, ResponsivenessTimeoutMs,
            &ignored) != 0;
    }

    const bool shouldReplace = !observation.visible || visible
        || observation.title.empty();
    if (shouldReplace) {
        observation.title = std::move(title);
        observation.windowClass.assign(className,
            classLength > 0 ? static_cast<std::size_t>(classLength) : 0);
        observation.visible = visible;
        observation.responsive = responsive;
    }
    return TRUE;
}
} 

bool isValidProcessIdentity(const ProcessIdentity& identity) noexcept {
    return identity.pid != 0 && identity.creationTime100ns != 0;
}

bool sameProcessIdentity(const ProcessIdentity& left,
        const ProcessIdentity& right) noexcept {
    return left.pid == right.pid
        && left.creationTime100ns == right.creationTime100ns;
}

bool ProcessIdentityLess::operator()(const ProcessIdentity& left,
        const ProcessIdentity& right) const noexcept {
    if (left.pid != right.pid) {
        return left.pid < right.pid;
    }
    return left.creationTime100ns < right.creationTime100ns;
}

bool isAutoInjectCandidate(const ProcessObservation& observation,
        const AutoInjectPolicy& policy) {
    if (!observation.commandLineReadable || observation.title.empty()
            || !observation.visible || !observation.responsive
            || observation.processAge < policy.minimumAge
            || observation.titleStableFor < policy.settleDelay) {
        return false;
    }
    return evaluateAutoInjectReadiness(observation, policy).ready;
}

std::optional<ProcessIdentity> selectNewestAutoInjectCandidate(
        const std::vector<ProcessObservation>& observations,
        const AutoInjectPolicy& policy) {
    std::optional<ProcessIdentity> selected;
    for (const ProcessObservation& observation : observations) {
        if (!isAutoInjectCandidate(observation, policy)) {
            continue;
        }
        if (!selected.has_value()
                || observation.identity.creationTime100ns
                    > selected->creationTime100ns
                || (observation.identity.creationTime100ns
                        == selected->creationTime100ns
                    && observation.identity.pid > selected->pid)) {
            selected = observation.identity;
        }
    }
    return selected;
}

bool openedProcessMatchesIdentity(void* nativeProcessHandle,
        const ProcessIdentity& expected) noexcept {
    if (!isValidProcessIdentity(expected)) {
        return false;
    }
    HANDLE process = static_cast<HANDLE>(nativeProcessHandle);
    ProcessIdentity actual;
    return processStillActive(process)
        && identityFromHandle(process, expected.pid, actual)
        && sameProcessIdentity(actual, expected);
}

bool queryProcessIdentity(std::uint32_t processId,
        ProcessIdentity& identity) noexcept {
    identity = {};
    if (processId == 0) {
        return false;
    }
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
        FALSE, processId);
    if (process == nullptr) {
        return false;
    }
    const bool success = processStillActive(process)
        && identityFromHandle(process, processId, identity);
    CloseHandle(process);
    return success;
}

std::vector<ProcessObservation> Win32ProcessProbe::probe() {
    std::vector<ProcessObservation> observations;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return observations;
    }

    FILETIME nowFileTime{};
    GetSystemTimeAsFileTime(&nowFileTime);
    const std::uint64_t now100ns = fileTimeValue(nowFileTime);

    PROCESSENTRY32W entry{sizeof(entry)};
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (!javaExecutable(entry.szExeFile)) {
                continue;
            }

            HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
                FALSE, entry.th32ProcessID);
            if (process == nullptr) {
                continue;
            }

            ProcessObservation observation;
            observation.exeName = entry.szExeFile;
            if (!processStillActive(process)
                    || !identityFromHandle(process, entry.th32ProcessID,
                        observation.identity)) {
                CloseHandle(process);
                continue;
            }
            observation.commandLineReadable = queryCommandLine(process,
                observation.commandLine);
            CloseHandle(process);

            if (now100ns > observation.identity.creationTime100ns) {
                const std::uint64_t age100ns = now100ns
                    - observation.identity.creationTime100ns;
                const std::uint64_t ageMilliseconds = age100ns / 10000ULL;
                observation.processAge = std::chrono::milliseconds(
                    std::min<std::uint64_t>(ageMilliseconds,
                        static_cast<std::uint64_t>(
                            std::numeric_limits<std::int64_t>::max())));
            }
            observations.push_back(std::move(observation));
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);

    WindowScanContext context;
    context.observations = &observations;
    for (std::size_t index = 0; index < observations.size(); ++index) {
        context.byPid.emplace(observations[index].identity.pid, index);
    }
    EnumWindows(captureWindow, reinterpret_cast<LPARAM>(&context));

    std::sort(observations.begin(), observations.end(),
        [](const ProcessObservation& left, const ProcessObservation& right) {
            return left.identity.pid < right.identity.pid;
        });
    return observations;
}

bool Win32ProcessProbe::isCurrent(const ProcessIdentity& identity) const {
    ProcessIdentity current;
    return queryProcessIdentity(identity.pid, current)
        && sameProcessIdentity(current, identity);
}
