#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "injection_coordinator.h"
#include "loader_settings.h"
#include "local_controller_service.h"
#include "win32_process_probe.h"

enum class ControllerPage {
    Login,
    BrowserAuth,
    MinecraftSelection,
    Loading,
    CachePrompt,
    LoadingComplete,
    OutdatedLauncher,
    Error,
    Settings
};

struct MinecraftProcess : ProcessIdentity {
    std::wstring title;
    bool alreadyInjected{};

    MinecraftProcess() = default;
    MinecraftProcess(ProcessIdentity value, std::wstring windowTitle,
            bool injected)
        : ProcessIdentity(value), title(std::move(windowTitle)),
          alreadyInjected(injected) {}
};

struct ControllerModelDependencies {
    std::shared_ptr<ProcessProbe> processProbe;
    std::function<LoaderSettings()> loadSettings;
    std::function<bool(const LoaderSettings&)> saveSettings;
    std::function<InjectionOutcome(const ProcessIdentity&)> inject;
};

class ControllerModel {
public:
    ControllerModel();
    explicit ControllerModel(ControllerModelDependencies dependencies);
    ~ControllerModel();
    ControllerModel(const ControllerModel&) = delete;
    ControllerModel& operator=(const ControllerModel&) = delete;

    ControllerPage page() const;
    void setPage(ControllerPage value);
    std::wstring status() const;
    void setStatus(std::wstring value);

    std::wstring& username();
    std::wstring& password();
    std::vector<MinecraftProcess> minecraftProcesses() const;

    void refreshMinecraftProcesses();
    bool injectMinecraft(std::uint32_t processId);
    LoaderSettings loaderSettings() const;
    bool applyLoaderSettings(const LoaderSettings& settings);
    void beginBrowserAuthentication(void* windowHandle);
    void reopenBrowserAuthentication();
    void cancelBrowserAuthentication();
    void persistCachePreference(bool enabled);
    bool cachePreference() const;
    std::wstring cacheDirectory() const;
    void tick();
    int loadingStage() const;
    double loadingElapsedSeconds() const;
    double stageElapsedSeconds() const;
    void submitCredentialAuthentication();
    void autoLoginToService();
    bool loginToService();

private:
    struct TitleHistory {
        std::wstring title;
        std::chrono::steady_clock::time_point stableSince{};
    };

    struct InjectionRequest {
        ProcessIdentity identity;
        std::uint64_t generation = 0;
    };

    static std::string httpPostJson(const std::wstring& baseUrl,
        const wchar_t* path, const std::string& body);
    static std::string jsonString(const std::string& json, const char* key);
    static bool readinessSettingsEqual(const LoaderSettings& left,
        const LoaderSettings& right) noexcept;
    bool materializeEmbeddedDll(std::uint32_t processId, std::wstring& output);
    bool probeAndPublish(std::uint64_t generation,
        std::vector<ProcessObservation>& observations);
    InjectionOutcome performInjection(const ProcessIdentity& identity);
    void injectionWorkerLoop();
    void publishState(ControllerPage page, std::wstring status);

    mutable std::mutex mutex_;
    std::condition_variable workerCondition_;
    ControllerPage page_{ControllerPage::Login};
    std::wstring status_;
    std::wstring username_;
    std::wstring password_;
    std::vector<MinecraftProcess> minecraftProcesses_;
    std::set<ProcessIdentity, ProcessIdentityLess> injectedProcesses_;
    std::set<ProcessIdentity, ProcessIdentityLess> attemptedProcesses_;
    std::map<ProcessIdentity, TitleHistory, ProcessIdentityLess> titleHistory_;
    std::string accessToken_;
    std::wstring serviceHttpBase_;
    std::wstring browserUrl_;
    LocalControllerService service_;
    std::shared_ptr<ProcessProbe> processProbe_;
    std::function<bool(const LoaderSettings&)> saveSettings_;
    std::function<InjectionOutcome(const ProcessIdentity&)> injectionOverride_;
    LoaderSettings loaderSettings_;
    std::uint64_t generation_{1};
    bool shutdown_{};
    bool autoSessionArmed_{true};
    bool injectionClaimed_{};
    std::optional<InjectionRequest> pendingManual_;
    std::atomic<bool> cancelAuth_{false};
    std::thread authThread_;
    std::thread injectionWorker_;
    bool cachePreference_{};
    int loadingStage_{};
    std::chrono::steady_clock::time_point loadingStarted_{};
    std::chrono::steady_clock::time_point stageStarted_{};
    std::chrono::steady_clock::time_point lastMinecraftRefresh_{};
};
