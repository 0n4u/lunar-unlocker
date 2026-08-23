#include "injection_coordinator.h"
#include "loader_bootstrap.h"
#include "win32_process_probe.h"

#include <windows.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

namespace {
std::wstring bootstrapObjectName(const wchar_t* prefix,
        std::uint32_t processId) {
    return std::wstring(L"Local\\LunarUnlocker.") + prefix + L"."
        + std::to_wstring(processId);
}

void copyText(char* destination, std::size_t capacity,
        const std::string& source) {
    const std::size_t count = std::min(capacity - 1, source.size());
    memcpy(destination, source.data(), count);
    destination[count] = '\0';
}

bool zeusEndpoint(std::string& host, std::uint16_t& port) {
    char configured[256]{};
    const DWORD length = GetEnvironmentVariableA("LUNARUNLOCKER_ZEUS_ADDRESS", configured,
        static_cast<DWORD>(std::size(configured)));
    const std::string address = length > 0 && length < std::size(configured)
        ? std::string(configured, length) : "127.0.0.1:8091";
    const auto separator = address.find_last_of(':');
    if (separator == std::string::npos || separator == 0
            || separator + 1 == address.size()) {
        return false;
    }
    char* end = nullptr;
    const std::string portText = address.substr(separator + 1);
    const unsigned long parsedPort = strtoul(portText.c_str(), &end, 10);
    host = address.substr(0, separator);
    if (end == portText.c_str() || *end != '\0' || parsedPort == 0
            || parsedPort > 65535
            || host.size() >= sizeof(LunarUnlockerBootstrapV2::serviceZeusHost)) {
        return false;
    }
    port = static_cast<std::uint16_t>(parsedPort);
    return true;
}

bool enableDebugPrivilege() {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        return false;
    }
    TOKEN_PRIVILEGES privileges{};
    privileges.PrivilegeCount = 1;
    LookupPrivilegeValueW(nullptr, L"SeDebugPrivilege",
        &privileges.Privileges[0].Luid);
    privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    SetLastError(ERROR_SUCCESS);
    AdjustTokenPrivileges(token, FALSE, &privileges, sizeof(privileges),
        nullptr, nullptr);
    const bool success = GetLastError() == ERROR_SUCCESS;
    CloseHandle(token);
    return success;
}

const IMAGE_SECTION_HEADER* sectionForRva(const IMAGE_NT_HEADERS64* nt,
        DWORD rva) {
    const auto* section = IMAGE_FIRST_SECTION(nt);
    for (WORD index = 0; index < nt->FileHeader.NumberOfSections;
            ++index, ++section) {
        const DWORD size = std::max(section->Misc.VirtualSize,
            section->SizeOfRawData);
        if (rva >= section->VirtualAddress
                && rva < section->VirtualAddress + size) {
            return section;
        }
    }
    return nullptr;
}

const unsigned char* rvaPointer(const std::vector<unsigned char>& image,
        const IMAGE_NT_HEADERS64* nt, DWORD rva) {
    if (rva < nt->OptionalHeader.SizeOfHeaders) {
        return rva < image.size() ? image.data() + rva : nullptr;
    }
    const auto* section = sectionForRva(nt, rva);
    if (!section) {
        return nullptr;
    }
    const std::size_t offset = section->PointerToRawData
        + (rva - section->VirtualAddress);
    return offset < image.size() ? image.data() + offset : nullptr;
}

DWORD reflectiveLoaderRva(const std::vector<unsigned char>& image) {
    if (image.size() < sizeof(IMAGE_DOS_HEADER)) {
        return 0;
    }
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(image.data());
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0
            || static_cast<std::size_t>(dos->e_lfanew)
                + sizeof(IMAGE_NT_HEADERS64) > image.size()) {
        return 0;
    }
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(
        image.data() + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE
            || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        return 0;
    }
    const auto directory = nt->OptionalHeader.DataDirectory[
        IMAGE_DIRECTORY_ENTRY_EXPORT];
    const auto* exports = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(
        rvaPointer(image, nt, directory.VirtualAddress));
    if (!exports) {
        return 0;
    }
    const auto* names = reinterpret_cast<const DWORD*>(
        rvaPointer(image, nt, exports->AddressOfNames));
    const auto* ordinals = reinterpret_cast<const WORD*>(
        rvaPointer(image, nt, exports->AddressOfNameOrdinals));
    const auto* functions = reinterpret_cast<const DWORD*>(
        rvaPointer(image, nt, exports->AddressOfFunctions));
    if (!names || !ordinals || !functions) {
        return 0;
    }
    for (DWORD index = 0; index < exports->NumberOfNames; ++index) {
        const auto* name = reinterpret_cast<const char*>(
            rvaPointer(image, nt, names[index]));
        if (name && (strstr(name, "?tim@@") != nullptr
                || strstr(name, "ReflectiveLoader") != nullptr)) {
            return functions[ordinals[index]];
        }
    }
    return 0;
}

InjectionOutcome invalidIdentityOutcome() {
    InjectionOutcome outcome;
    outcome.error = L"Minecraft process identity is no longer current";
    return outcome;
}
} 

InjectionOutcome InjectionCoordinator::injectProductDll(
        const ProcessIdentity& identity, const std::wstring& dllPath,
        std::uint16_t controllerPort, const std::string& serviceHttpBase) {
    InjectionOutcome outcome;
    if (!isValidProcessIdentity(identity)) {
        return invalidIdentityOutcome();
    }
    if (controllerPort == 0) {
        outcome.error = L"Loader control port unavailable";
        return outcome;
    }
    if (serviceHttpBase.empty()
            || serviceHttpBase.size()
                >= sizeof(LunarUnlockerBootstrapV2::serviceHttpBase)) {
        outcome.error = L"LUNARUNLOCKER_ONLINE_BASE_URL too long for the bootstrap block";
        return outcome;
    }
    std::string serviceZeusHost;
    std::uint16_t serviceZeusPort = 0;
    if (!zeusEndpoint(serviceZeusHost, serviceZeusPort)) {
        outcome.error = L"LUNARUNLOCKER_ZEUS_ADDRESS must use the host:port format";
        return outcome;
    }
    if (GetFileAttributesW(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        outcome.error = L"LunarUnlockerNative.dll not found next to the loader";
        return outcome;
    }

    enableDebugPrivilege();
    HANDLE process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION
        | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, identity.pid);
    if (!process) {
        outcome.error = L"Unable to open Minecraft process";
        return outcome;
    }
    if (!openedProcessMatchesIdentity(process, identity)) {
        CloseHandle(process);
        return invalidIdentityOutcome();
    }

    const std::wstring mappingName = bootstrapObjectName(L"Bootstrap",
        identity.pid);
    const std::wstring ackName = bootstrapObjectName(L"BootstrapAck",
        identity.pid);
    HANDLE mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr,
        PAGE_READWRITE, 0, sizeof(LunarUnlockerBootstrapV2), mappingName.c_str());
    const DWORD mappingError = GetLastError();
    if (!mapping || mappingError == ERROR_ALREADY_EXISTS) {
        outcome.error = L"This process already has a loader bootstrap block";
        if (mapping) {
            CloseHandle(mapping);
        }
        CloseHandle(process);
        return outcome;
    }

    auto* block = static_cast<LunarUnlockerBootstrapV2*>(MapViewOfFile(mapping,
        FILE_MAP_ALL_ACCESS, 0, 0, sizeof(LunarUnlockerBootstrapV2)));
    HANDLE ack = CreateEventW(nullptr, TRUE, FALSE, ackName.c_str());
    const DWORD ackError = GetLastError();
    if (!block || !ack || ackError == ERROR_ALREADY_EXISTS) {
        outcome.error = L"Unable to create loader bootstrap object";
        if (block) {
            UnmapViewOfFile(block);
        }
        if (ack) {
            CloseHandle(ack);
        }
        CloseHandle(mapping);
        CloseHandle(process);
        return outcome;
    }

    SecureZeroMemory(block, sizeof(*block));
    block->magic = LUNARUNLOCKER_BOOTSTRAP_MAGIC;
    block->version = LUNARUNLOCKER_BOOTSTRAP_VERSION;
    block->structureSize = static_cast<std::uint16_t>(sizeof(*block));
    block->targetPid = identity.pid;
    block->mode = LUNARUNLOCKER_BOOTSTRAP_MODE_ONLINE;
    block->controllerPort = controllerPort;
    copyText(block->serviceHttpBase, sizeof(block->serviceHttpBase),
        serviceHttpBase);
    copyText(block->serviceZeusHost, sizeof(block->serviceZeusHost),
        serviceZeusHost);
    block->serviceZeusPort = serviceZeusPort;
    block->status = LUNARUNLOCKER_BOOTSTRAP_STATUS_CREATED;

    HANDLE remoteThread = nullptr;
    void* remotePath = nullptr;
    bool remoteThreadCompleted = false;
    const SIZE_T pathBytes = (dllPath.size() + 1) * sizeof(wchar_t);
    SIZE_T written = 0;

    remotePath = VirtualAllocEx(process, nullptr, pathBytes,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remotePath || !WriteProcessMemory(process, remotePath,
            dllPath.c_str(), pathBytes, &written) || written != pathBytes) {
        outcome.error = L"Unable to write product DLL path";
        goto cleanup;
    }

    {
        HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
        FARPROC loadLibrary = kernel32 == nullptr ? nullptr
            : GetProcAddress(kernel32, "LoadLibraryW");
        if (!loadLibrary) {
            outcome.error = L"Unable to resolve LoadLibraryW";
            goto cleanup;
        }
        remoteThread = CreateRemoteThread(process, nullptr, 0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibrary),
            remotePath, 0, nullptr);
    }
    if (!remoteThread) {
        outcome.error = L"Starting product DLL load failed";
        goto cleanup;
    }
    outcome.remoteThreadStarted = true;
    outcome.retrySafe = false;
    if (WaitForSingleObject(remoteThread, 30000) != WAIT_OBJECT_0) {
        outcome.error = L"Product DLL load timed out";
        goto cleanup;
    }
    remoteThreadCompleted = true;
    if (WaitForSingleObject(ack, 30000) != WAIT_OBJECT_0) {
        outcome.error =
            L"Product DLL did not acknowledge the socket bootstrap block";
        goto cleanup;
    }
    if (block->status != LUNARUNLOCKER_BOOTSTRAP_STATUS_CONSUMED) {
        outcome.error = L"Product DLL rejected the socket bootstrap block";
        goto cleanup;
    }
    outcome.success = true;
    outcome.error.clear();

cleanup:
    SecureZeroMemory(block, sizeof(*block));
    if (remoteThread) {
        CloseHandle(remoteThread);
    }
    if (remotePath && remoteThreadCompleted) {
        VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
    }
    CloseHandle(process);
    UnmapViewOfFile(block);
    CloseHandle(ack);
    CloseHandle(mapping);
    return outcome;
}

bool InjectionCoordinator::injectProductDll(std::uint32_t processId,
        const std::wstring& dllPath, std::uint16_t controllerPort,
        const std::string& serviceHttpBase, std::wstring& error) {
    ProcessIdentity identity;
    if (!queryProcessIdentity(processId, identity)) {
        error = L"Minecraft process identity is no longer current";
        return false;
    }
    InjectionOutcome outcome = injectProductDll(identity, dllPath,
        controllerPort, serviceHttpBase);
    error = std::move(outcome.error);
    return outcome.success;
}

InjectionOutcome InjectionCoordinator::injectReflectiveDll(
        const ProcessIdentity& identity, const std::wstring& dllPath,
        std::uint16_t controllerPort) {
    InjectionOutcome outcome;
    if (!isValidProcessIdentity(identity)) {
        return invalidIdentityOutcome();
    }

    std::ifstream input(dllPath, std::ios::binary);
    if (!input) {
        outcome.error = L"lunarunlocker_v4.dll not found next to the controller";
        return outcome;
    }
    std::vector<unsigned char> image((std::istreambuf_iterator<char>(input)), {});
    const DWORD loaderRva = reflectiveLoaderRva(image);
    if (!loaderRva) {
        outcome.error = L"Reflective loader export not found";
        return outcome;
    }

    enableDebugPrivilege();
    HANDLE process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION
        | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, identity.pid);
    if (!process) {
        outcome.error = L"Unable to open Minecraft process";
        return outcome;
    }
    if (!openedProcessMatchesIdentity(process, identity)) {
        CloseHandle(process);
        return invalidIdentityOutcome();
    }

    void* remote = VirtualAllocEx(process, nullptr, image.size(),
        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    SIZE_T written = 0;
    if (!remote || !WriteProcessMemory(process, remote, image.data(),
            image.size(), &written) || written != image.size()) {
        outcome.error = L"Unable to write reflective image";
        if (remote) {
            VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        }
        CloseHandle(process);
        return outcome;
    }

    auto start = reinterpret_cast<LPTHREAD_START_ROUTINE>(
        static_cast<std::byte*>(remote) + loaderRva);
    HANDLE thread = CreateRemoteThread(process, nullptr, 0, start,
        reinterpret_cast<void*>(static_cast<std::uintptr_t>(controllerPort)),
        0, nullptr);
    if (!thread) {
        outcome.error = L"Starting reflective loader failed";
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        CloseHandle(process);
        return outcome;
    }
    outcome.remoteThreadStarted = true;
    outcome.retrySafe = false;

    const DWORD waitResult = WaitForSingleObject(thread, 30000);
    if (waitResult != WAIT_OBJECT_0) {
        outcome.error = L"Reflective loader timed out";
        CloseHandle(thread);
        CloseHandle(process);
        return outcome;
    }

    DWORD exitCode = 0;
    const bool exitCodeRead = GetExitCodeThread(thread, &exitCode) != FALSE;
    CloseHandle(thread);
    VirtualFreeEx(process, remote, 0, MEM_RELEASE);
    CloseHandle(process);
    if (!exitCodeRead || exitCode == 0) {
        outcome.error = L"Reflective loader returned failure";
        return outcome;
    }
    outcome.success = true;
    outcome.error.clear();
    return outcome;
}

bool InjectionCoordinator::injectReflectiveDll(std::uint32_t processId,
        const std::wstring& dllPath, std::uint16_t controllerPort,
        std::wstring& error) {
    ProcessIdentity identity;
    if (!queryProcessIdentity(processId, identity)) {
        error = L"Minecraft process identity is no longer current";
        return false;
    }
    InjectionOutcome outcome = injectReflectiveDll(identity, dllPath,
        controllerPort);
    error = std::move(outcome.error);
    return outcome.success;
}
