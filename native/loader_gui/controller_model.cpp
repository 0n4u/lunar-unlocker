#include "controller_model.h"
#include "injection_coordinator.h"

#include <windows.h>
#include <winhttp.h>
#include <shellapi.h>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <random>
#include <set>
#include <sstream>
#include <thread>
#include <unordered_map>

namespace {
constexpr UINT WM_CONTROLLER_STATE = WM_APP + 41;

std::wstring onlineBaseUrl() {
    wchar_t value[2048]{};
    const DWORD length = GetEnvironmentVariableW(L"LUNARUNLOCKER_ONLINE_BASE_URL", value,
        static_cast<DWORD>(std::size(value)));
    std::wstring result = length > 0 && length < std::size(value)
        ? std::wstring(value, length) : L"http://127.0.0.1:8080";
    while (!result.empty() && result.back() == L'/') result.pop_back();
    return result;
}

std::string utf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
        static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};
    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
        static_cast<int>(value.size()), result.data(), size, nullptr, nullptr);
    return result;
}

std::string jsonEscape(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    for (const unsigned char character : value) {
        switch (character) {
        case '\\': result += "\\\\"; break;
        case '"': result += "\\\""; break;
        case '\b': result += "\\b"; break;
        case '\f': result += "\\f"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default:
            if (character >= 0x20) result.push_back(static_cast<char>(character));
            break;
        }
    }
    return result;
}



void sweepDirectory(const std::wstring& directory) {
    WIN32_FIND_DATAW data{};
    const std::wstring pattern = directory + L"\\*";
    HANDLE found = FindFirstFileW(pattern.c_str(), &data);
    if (found == INVALID_HANDLE_VALUE) return;
    do {
        if (wcscmp(data.cFileName, L".") == 0 ||
                wcscmp(data.cFileName, L"..") == 0) {
            continue;
        }
        const std::wstring child = directory + L"\\" + data.cFileName;
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            sweepDirectory(child);
            RemoveDirectoryW(child.c_str());
        } else {
            SetFileAttributesW(child.c_str(), FILE_ATTRIBUTE_NORMAL);
            DeleteFileW(child.c_str());
        }
    } while (FindNextFileW(found, &data));
    FindClose(found);
}

std::wstring executableDirectory() {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring result(path);
    const auto separator = result.find_last_of(L"\\/");
    return separator == std::wstring::npos ? L"." : result.substr(0, separator);
}





void ensureClientDirectoryHidden() {
    const std::wstring directory = executableDirectory() + L"\\.lunarunlocker";
    if (CreateDirectoryW(directory.c_str(), nullptr) ||
            GetLastError() == ERROR_ALREADY_EXISTS) {
        const DWORD attributes = GetFileAttributesW(directory.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES &&
                (attributes & FILE_ATTRIBUTE_HIDDEN) == 0) {
            SetFileAttributesW(directory.c_str(),
                attributes | FILE_ATTRIBUTE_HIDDEN);
        }
    }
    
    wchar_t tempRoot[MAX_PATH]{};
    if (GetTempPathW(static_cast<DWORD>(std::size(tempRoot)), tempRoot) != 0 &&
            tempRoot[0] != L'\0') {
        std::wstring root = tempRoot;
        if (!root.empty() && root.back() != L'\\') root.push_back(L'\\');
        DeleteFileW((root + L"injector_dir.txt").c_str());
        DeleteFileW((root + L"injector_diag.txt").c_str());
    }
}
}







bool ControllerModel::materializeEmbeddedDll(std::uint32_t processId,
        std::wstring& output) {
    
    const std::wstring external = executableDirectory() + L"\\LunarUnlockerNative.dll";
    if (GetFileAttributesW(external.c_str()) != INVALID_FILE_ATTRIBUTES) {
        output = external;
        return true;
    }
    const HRSRC resource = FindResourceW(nullptr,
        MAKEINTRESOURCEW(422), MAKEINTRESOURCEW(10));
    if (resource == nullptr) return false;
    const DWORD size = SizeofResource(nullptr, resource);
    const HGLOBAL loaded = LoadResource(nullptr, resource);
    const auto* bytes = loaded == nullptr ? nullptr
        : static_cast<const unsigned char*>(LockResource(loaded));
    if (bytes == nullptr || size < 4) return false;

    ensureClientDirectoryHidden();
    const std::wstring clientRoot = executableDirectory() + L"\\.lunarunlocker";
    wchar_t directory[MAX_PATH]{};
    _snwprintf_s(directory, std::size(directory), _TRUNCATE,
        L"%ls\\LunarUnlockerRecovery", clientRoot.c_str());
    if (!CreateDirectoryW(directory, nullptr)
            && GetLastError() != ERROR_ALREADY_EXISTS) {
        return false;
    }
    
    
    sweepDirectory(directory);
    wchar_t target[MAX_PATH]{};
    _snwprintf_s(target, std::size(target), _TRUNCATE,
        L"%ls\\LunarUnlockerNative-%lu.dll", directory,
        static_cast<unsigned long>(processId));

    HANDLE file = CreateFileW(target, GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        
        
        const DWORD createError = GetLastError();
        if (createError == ERROR_SHARING_VIOLATION ||
                createError == ERROR_ACCESS_DENIED) {
            WIN32_FILE_ATTRIBUTE_DATA existing{};
            if (GetFileAttributesExW(target, GetFileExInfoStandard, &existing) &&
                    existing.nFileSizeLow == size &&
                    existing.nFileSizeHigh == 0) {
                output = target;
                return true;
            }
        }
        return false;
    }

    DWORD offset = 0;
    bool ok = true;
    while (offset < size) {
        DWORD written = 0;
        const DWORD remaining = size - offset;
        if (!WriteFile(file, bytes + offset, remaining, &written, nullptr)
                || written == 0) {
            ok = false;
            break;
        }
        offset += written;
    }
    CloseHandle(file);
    if (!ok) {
        DeleteFileW(target);
        return false;
    }
    output = target;
    return true;
}

ControllerModel::ControllerModel()
    : ControllerModel(ControllerModelDependencies{}) {}

ControllerModel::ControllerModel(ControllerModelDependencies dependencies) {
    processProbe_ = dependencies.processProbe
        ? std::move(dependencies.processProbe)
        : std::make_shared<Win32ProcessProbe>();
    saveSettings_ = dependencies.saveSettings
        ? std::move(dependencies.saveSettings)
        : [](const LoaderSettings& settings) { return settings.save(); };
    injectionOverride_ = std::move(dependencies.inject);
    loaderSettings_ = dependencies.loadSettings
        ? dependencies.loadSettings() : LoaderSettings::load();

    serviceHttpBase_ = onlineBaseUrl();
    const std::wstring setting = cacheDirectory() + L"cache.preference";
    std::wifstream input(setting);
    int enabled = 0;
    if (input >> enabled) {
        cachePreference_ = enabled != 0;
    }
    
    
    autoLoginToService();

    
    
    injectionWorker_ = std::thread(&ControllerModel::injectionWorkerLoop, this);
}

ControllerModel::~ControllerModel() {
    cancelAuth_ = true;
    {
        std::lock_guard lock(mutex_);
        shutdown_ = true;
        ++generation_;
        pendingManual_.reset();
    }
    workerCondition_.notify_all();
    if (injectionWorker_.joinable()) injectionWorker_.join();
    if (authThread_.joinable()) authThread_.join();
    service_.stop();
}

ControllerPage ControllerModel::page() const {
    std::lock_guard lock(mutex_);
    return page_;
}

void ControllerModel::setPage(ControllerPage value) {
    std::lock_guard lock(mutex_);
    page_ = value;
}

std::wstring ControllerModel::status() const {
    std::lock_guard lock(mutex_);
    return status_;
}

void ControllerModel::setStatus(std::wstring value) {
    std::lock_guard lock(mutex_);
    status_ = std::move(value);
}

void ControllerModel::publishState(ControllerPage page, std::wstring status) {
    std::lock_guard lock(mutex_);
    page_ = page;
    status_ = std::move(status);
}

std::wstring& ControllerModel::username() { return username_; }
std::wstring& ControllerModel::password() { return password_; }

std::vector<MinecraftProcess> ControllerModel::minecraftProcesses() const {
    std::lock_guard lock(mutex_);
    return minecraftProcesses_;
}

bool ControllerModel::readinessSettingsEqual(const LoaderSettings& left,
        const LoaderSettings& right) noexcept {
    return left.autoInjectEnabled == right.autoInjectEnabled
        && left.readiness == right.readiness
        && left.settleDelayMs == right.settleDelayMs
        && left.minimumProcessAgeMs == right.minimumProcessAgeMs;
}

bool ControllerModel::probeAndPublish(std::uint64_t generation,
        std::vector<ProcessObservation>& observations) {
    std::vector<ProcessObservation> probed = processProbe_->probe();
    const auto now = std::chrono::steady_clock::now();

    std::lock_guard lock(mutex_);
    if (shutdown_ || generation != generation_) {
        return false;
    }

    std::set<ProcessIdentity, ProcessIdentityLess> present;
    std::vector<MinecraftProcess> rows;
    for (ProcessObservation& observation : probed) {
        if (!isValidProcessIdentity(observation.identity)) {
            continue;
        }
        present.insert(observation.identity);
        if (!observation.title.empty()) {
            auto history = titleHistory_.find(observation.identity);
            if (history == titleHistory_.end()
                    || history->second.title != observation.title) {
                titleHistory_[observation.identity] = {observation.title, now};
                observation.titleStableFor = std::chrono::milliseconds::zero();
            } else {
                observation.titleStableFor =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - history->second.stableSince);
            }
        } else {
            titleHistory_.erase(observation.identity);
            observation.titleStableFor = std::chrono::milliseconds::zero();
        }
        observation.alreadyInjected = injectedProcesses_.find(
            observation.identity) != injectedProcesses_.end();
        observation.alreadyAttempted = attemptedProcesses_.find(
            observation.identity) != attemptedProcesses_.end();
        if (observation.visible && !observation.title.empty()) {
            rows.emplace_back(observation.identity, observation.title,
                observation.alreadyInjected);
        }
    }
    for (auto history = titleHistory_.begin(); history != titleHistory_.end();) {
        if (present.find(history->first) == present.end()) {
            history = titleHistory_.erase(history);
        } else {
            ++history;
        }
    }
    std::sort(rows.begin(), rows.end(),
        [](const MinecraftProcess& left, const MinecraftProcess& right) {
            if (left.pid != right.pid) return left.pid < right.pid;
            return left.creationTime100ns < right.creationTime100ns;
        });
    minecraftProcesses_ = std::move(rows);
    lastMinecraftRefresh_ = now;
    observations = std::move(probed);
    return true;
}

void ControllerModel::refreshMinecraftProcesses() {
    std::uint64_t generation = 0;
    {
        std::lock_guard lock(mutex_);
        generation = generation_;
    }
    std::vector<ProcessObservation> ignored;
    probeAndPublish(generation, ignored);
}

bool ControllerModel::injectMinecraft(std::uint32_t processId) {
    {
        std::lock_guard lock(mutex_);
        autoSessionArmed_ = false;
        if (injectionClaimed_ || pendingManual_.has_value()) {
            status_ = L"Injection is already in progress (busy)";
            return false;
        }
        const auto row = std::find_if(minecraftProcesses_.begin(),
            minecraftProcesses_.end(), [processId](const MinecraftProcess& process) {
                return process.pid == processId;
            });
        if (row == minecraftProcesses_.end()
                || !isValidProcessIdentity(*row)) {
            status_ = L"Minecraft process is no longer available";
            return false;
        }
        const ProcessIdentity identity{row->pid, row->creationTime100ns};
        if (injectedProcesses_.find(identity) != injectedProcesses_.end()
                || attemptedProcesses_.find(identity) != attemptedProcesses_.end()) {
            status_ = L"This Minecraft process was already injected or attempted";
            return false;
        }

        ++generation_;
        titleHistory_.clear();
        attemptedProcesses_.insert(identity);
        pendingManual_ = InjectionRequest{identity, generation_};
        status_ = L"Injection queued";
    }
    workerCondition_.notify_all();
    return true;
}

LoaderSettings ControllerModel::loaderSettings() const {
    std::lock_guard lock(mutex_);
    return loaderSettings_;
}

bool ControllerModel::applyLoaderSettings(const LoaderSettings& settings) {
    bool readinessChanged = false;
    {
        std::lock_guard lock(mutex_);
        readinessChanged = !readinessSettingsEqual(loaderSettings_, settings);
        loaderSettings_ = settings;
        if (readinessChanged) {
            ++generation_;
            titleHistory_.clear();
            attemptedProcesses_.clear();
            pendingManual_.reset();
        }
    }
    if (readinessChanged) {
        workerCondition_.notify_all();
    }
    return saveSettings_(settings);
}

InjectionOutcome ControllerModel::performInjection(
        const ProcessIdentity& identity) {
    InjectionOutcome outcome;
    if (!processProbe_->isCurrent(identity)) {
        outcome.error = L"Minecraft process identity is no longer current";
        return outcome;
    }

    std::wstring dllPath;
    if (!materializeEmbeddedDll(identity.pid, dllPath)) {
        outcome.error = L"Unable to extract the embedded LunarUnlockerNative.dll";
        return outcome;
    }
    ensureClientDirectoryHidden();

    std::string token;
    std::wstring serviceHttpBase;
    bool cacheEnabled = false;
    {
        std::lock_guard lock(mutex_);
        token = accessToken_;
        serviceHttpBase = serviceHttpBase_;
        cacheEnabled = cachePreference_;
    }
    if (token.empty()) {
        if (!loginToService()) {
            outcome.error =
                L"Unable to log in to the local service (start Minecraft first)";
            return outcome;
        }
        std::lock_guard lock(mutex_);
        token = accessToken_;
        serviceHttpBase = serviceHttpBase_;
        cacheEnabled = cachePreference_;
    }
    if (token.empty()) {
        outcome.error =
            L"Unable to log in to the local service (start Minecraft first)";
        return outcome;
    }

    struct TokenCleaner {
        std::string& value;
        ~TokenCleaner() {
            if (!value.empty()) SecureZeroMemory(value.data(), value.size());
        }
    } tokenCleaner{token};

    if (!service_.start(token, cacheEnabled, true)) {
        outcome.error = L"Unable to create loader control socket";
        return outcome;
    }
    struct ServiceStopGuard {
        LocalControllerService& service;
        bool keepRunning = false;
        ~ServiceStopGuard() {
            if (!keepRunning) service.stop();
        }
    } serviceGuard{service_};

    const std::string serviceBase = utf8(serviceHttpBase);
    outcome = InjectionCoordinator::injectProductDll(identity, dllPath,
        service_.port(), serviceBase);
    if (outcome.success) {
        serviceGuard.keepRunning = true;
    }
    return outcome;
}

void ControllerModel::injectionWorkerLoop() {
    for (;;) {
        InjectionRequest request;
        bool haveRequest = false;
        std::uint64_t probeGeneration = 0;
        {
            std::unique_lock lock(mutex_);
            workerCondition_.wait(lock, [this] {
                return shutdown_ || pendingManual_.has_value()
                    || (autoSessionArmed_ && loaderSettings_.autoInjectEnabled);
            });
            if (shutdown_) return;
            if (pendingManual_.has_value()) {
                request = *pendingManual_;
                pendingManual_.reset();
                injectionClaimed_ = true;
                haveRequest = true;
            } else {
                probeGeneration = generation_;
            }
        }

        if (!haveRequest) {
            std::vector<ProcessObservation> observations;
            if (!probeAndPublish(probeGeneration, observations)) {
                continue;
            }

            LoaderSettings settings;
            {
                std::lock_guard lock(mutex_);
                if (shutdown_ || generation_ != probeGeneration
                        || !autoSessionArmed_
                        || !loaderSettings_.autoInjectEnabled) {
                    continue;
                }
                settings = loaderSettings_;
            }
            AutoInjectPolicy policy;
            policy.mode = settings.readiness;
            policy.settleDelay = std::chrono::milliseconds(
                std::max(settings.settleDelayMs, 0));
            policy.minimumAge = std::chrono::milliseconds(
                std::max(settings.minimumProcessAgeMs, 0));
            const auto selected = selectNewestAutoInjectCandidate(
                observations, policy);

            std::unique_lock lock(mutex_);
            if (shutdown_) return;
            if (generation_ != probeGeneration || !autoSessionArmed_
                    || !loaderSettings_.autoInjectEnabled) {
                continue;
            }
            if (selected.has_value()
                    && attemptedProcesses_.find(*selected)
                        == attemptedProcesses_.end()
                    && injectedProcesses_.find(*selected)
                        == injectedProcesses_.end()) {
                attemptedProcesses_.insert(*selected);
                request = InjectionRequest{*selected, generation_};
                injectionClaimed_ = true;
                haveRequest = true;
            } else {
                const std::uint64_t observedGeneration = generation_;
                workerCondition_.wait_for(lock, std::chrono::milliseconds(500),
                    [this, observedGeneration] {
                        return shutdown_ || pendingManual_.has_value()
                            || generation_ != observedGeneration
                            || !autoSessionArmed_
                            || !loaderSettings_.autoInjectEnabled;
                    });
            }
        }
        if (!haveRequest) {
            continue;
        }

        InjectionOutcome outcome = injectionOverride_
            ? injectionOverride_(request.identity)
            : performInjection(request.identity);
        const int completedStage = outcome.success ? service_.stage() : 0;
        bool stale = false;
        {
            std::lock_guard lock(mutex_);
            injectionClaimed_ = false;
            stale = shutdown_ || generation_ != request.generation;
            if (!stale) {
                if (outcome.success) {
                    injectedProcesses_.insert(request.identity);
                    for (MinecraftProcess& process : minecraftProcesses_) {
                        if (sameProcessIdentity(process, request.identity)) {
                            process.alreadyInjected = true;
                        }
                    }
                    if (!accessToken_.empty()) {
                        SecureZeroMemory(accessToken_.data(), accessToken_.size());
                        accessToken_.clear();
                    }
                    loadingStage_ = completedStage;
                    loadingStarted_ = std::chrono::steady_clock::now();
                    stageStarted_ = loadingStarted_;
                    page_ = ControllerPage::Loading;
                    status_.clear();
                } else {
                    page_ = ControllerPage::Error;
                    status_ = outcome.error.empty()
                        ? L"Injection of LunarUnlockerNative.dll failed"
                        : std::move(outcome.error);
                }
            }
        }
        if (stale && outcome.success && !injectionOverride_) {
            service_.stop();
        }
        workerCondition_.notify_all();
    }
}

std::string ControllerModel::httpPostJson(const std::wstring& baseUrl,
                                          const wchar_t* path,
                                          const std::string& body) {
    URL_COMPONENTS components{};
    components.dwStructSize = sizeof(components);
    components.dwSchemeLength = static_cast<DWORD>(-1);
    components.dwHostNameLength = static_cast<DWORD>(-1);
    components.dwUrlPathLength = static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(baseUrl.c_str(), 0, 0, &components) ||
            components.dwHostNameLength == 0) {
        return {};
    }
    const std::wstring host(components.lpszHostName, components.dwHostNameLength);
    HINTERNET session = WinHttpOpen(L"LunarUnlocker4/Loader",
        WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) return {};
    WinHttpSetTimeouts(session, 5000, 5000, 5000, 5000);
    HINTERNET connection = WinHttpConnect(session, host.c_str(), components.nPort, 0);
    const DWORD flags = components.nScheme == INTERNET_SCHEME_HTTPS
        ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request = connection ? WinHttpOpenRequest(connection, L"POST", path,
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags) : nullptr;
    std::string response;
    const wchar_t* headers = L"Content-Type: application/json; charset=utf-8";
    if (request && WinHttpSendRequest(request, headers, static_cast<DWORD>(-1L),
            const_cast<char*>(body.data()), static_cast<DWORD>(body.size()),
            static_cast<DWORD>(body.size()), 0) && WinHttpReceiveResponse(request, nullptr)) {
        DWORD statusCode = 0;
        DWORD statusSize = sizeof(statusCode);
        if (WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE |
                WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX) && statusCode == 200) {
            for (;;) {
                DWORD available = 0;
                if (!WinHttpQueryDataAvailable(request, &available) || available == 0) break;
                const auto offset = response.size();
                response.resize(offset + available);
                DWORD read = 0;
                if (!WinHttpReadData(request, response.data() + offset, available, &read)) break;
                response.resize(offset + read);
            }
        }
    }
    if (request) WinHttpCloseHandle(request);
    if (connection) WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return response;
}

std::string ControllerModel::jsonString(const std::string& json, const char* key) {
    const std::string needle = std::string("\"") + key + "\"";
    auto position = json.find(needle);
    if (position == std::string::npos) return {};
    position = json.find(':', position + needle.size());
    position = json.find('"', position == std::string::npos ? position : position + 1);
    if (position == std::string::npos) return {};
    const auto end = json.find('"', position + 1);
    return end == std::string::npos ? std::string{} : json.substr(position + 1, end - position - 1);
}

void ControllerModel::beginBrowserAuthentication(void* windowHandle) {
    
    
    
    
    (void)windowHandle;
    cancelAuth_ = true;
    if (authThread_.joinable()) authThread_.join();
    cancelAuth_ = false;
    setPage(ControllerPage::Login);
    setStatus(L"");
    autoLoginToService();
}

void ControllerModel::reopenBrowserAuthentication() {
    std::wstring url;
    {
        std::lock_guard lock(mutex_);
        url = browserUrl_;
    }
    if (!url.empty()) ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void ControllerModel::cancelBrowserAuthentication() {
    cancelAuth_ = true;
    setPage(ControllerPage::Login);
}

std::wstring ControllerModel::cacheDirectory() const {
    const std::wstring directory = executableDirectory() + L"\\.lunarunlocker\\";
    return directory;
}

void ControllerModel::persistCachePreference(bool enabled) {
    {
        std::lock_guard lock(mutex_);
        cachePreference_ = enabled;
    }
    const auto directory = cacheDirectory();
    if (CreateDirectoryW(directory.c_str(), nullptr) ||
            GetLastError() == ERROR_ALREADY_EXISTS) {
        const std::wstring clientRoot = executableDirectory() + L"\\.lunarunlocker";
        const DWORD attributes = GetFileAttributesW(clientRoot.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES &&
                (attributes & FILE_ATTRIBUTE_HIDDEN) == 0) {
            SetFileAttributesW(clientRoot.c_str(),
                attributes | FILE_ATTRIBUTE_HIDDEN);
        }
    }
    std::wofstream output(directory + L"cache.preference", std::ios::trunc);
    output << (enabled ? 1 : 0);
}

bool ControllerModel::cachePreference() const {
    std::lock_guard lock(mutex_);
    return cachePreference_;
}

void ControllerModel::tick() {
    const auto now = std::chrono::steady_clock::now();
    const auto currentPage = page();
    if (currentPage == ControllerPage::MinecraftSelection) {
        bool refresh = false;
        {
            std::lock_guard lock(mutex_);
            const bool autoWorkerRefreshes = autoSessionArmed_
                && loaderSettings_.autoInjectEnabled;
            refresh = !autoWorkerRefreshes
                && (lastMinecraftRefresh_.time_since_epoch().count() == 0
                    || now - lastMinecraftRefresh_ >= std::chrono::seconds(2));
        }
        if (refresh) refreshMinecraftProcesses();
        return;
    }
    if (currentPage != ControllerPage::Loading) return;

    const int serviceStage = service_.stage();
    {
        std::lock_guard lock(mutex_);
        if (serviceStage != loadingStage_) {
            loadingStage_ = serviceStage;
            stageStarted_ = now;
        }
    }
    if (service_.completed()) {
        publishState(ControllerPage::LoadingComplete, L"");
    } else if (service_.failed()) {
        const std::string detail = service_.error();
        publishState(ControllerPage::Error,
            detail.empty() ? L"Native load connection closed unexpectedly"
                           : std::wstring(detail.begin(), detail.end()));
    } else if (loadingElapsedSeconds() >= 90.0) {
        publishState(ControllerPage::Error,
            L"Native load timed out\nNote: on 26+ versions, inject after opening a world");
    }
}

int ControllerModel::loadingStage() const {
    std::lock_guard lock(mutex_);
    return loadingStage_;
}

double ControllerModel::loadingElapsedSeconds() const {
    std::lock_guard lock(mutex_);
    if (loadingStarted_.time_since_epoch().count() == 0) return 0.0;
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - loadingStarted_).count();
}

double ControllerModel::stageElapsedSeconds() const {
    std::lock_guard lock(mutex_);
    if (stageStarted_.time_since_epoch().count() == 0) return 0.0;
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - stageStarted_).count();
}

void ControllerModel::submitCredentialAuthentication() {
    const std::wstring usernameValue = username_;
    const std::string usernameUtf8 = utf8(usernameValue);
    if (usernameUtf8.empty()) {
        setStatus(L"Please enter a username");
        return;
    }
    const std::string response = httpPostJson(serviceHttpBase_, L"/loader/login",
        "{\"username\":\"" + jsonEscape(usernameUtf8) + "\"}");
    const std::string token = jsonString(response, "token");
    if (token.empty()) {
        setStatus(L"Unable to log in to the local service");
        return;
    }
    {
        std::lock_guard lock(mutex_);
        accessToken_ = token;
        status_.clear();
    }
    refreshMinecraftProcesses();
    setPage(ControllerPage::MinecraftSelection);
}

bool ControllerModel::loginToService() {
    
    
    
    
    
    std::string token;
    token.reserve(48);
    std::mt19937 generator(static_cast<unsigned int>(
        std::chrono::steady_clock::now().time_since_epoch().count())
        ^ static_cast<unsigned int>(GetCurrentProcessId()));
    const char alphabet[] = "0123456789abcdef";
    for (int i = 0; i < 48; ++i) {
        token.push_back(alphabet[generator() % 16]);
    }
    {
        std::lock_guard lock(mutex_);
        accessToken_ = token;
        status_.clear();
    }
    return true;
}

void ControllerModel::autoLoginToService() {
    if (loginToService()) {
        refreshMinecraftProcesses();
    }
    setPage(ControllerPage::MinecraftSelection);
}
