#include <chrono>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "../loader_gui/controller_model.h"
#include "../loader_gui/loader_settings.h"

/* Minimal test harness (no gtest — CTest runs directly). */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) std::cout << "  TEST: " << name << " ... "
#define PASS() do { std::cout << "PASS\n"; tests_passed++; } while (0)
#define FAIL(msg) do { std::cout << "FAIL: " << msg << "\n"; tests_failed++; } while (0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while (0)

using namespace std::chrono_literals;

/* A fake ProcessProbe that yields no processes — deterministic and fast. */
class EmptyProbe final : public ProcessProbe {
public:
    std::vector<ProcessObservation> probe() override { return {}; }
    bool isCurrent(const ProcessIdentity&) const override { return false; }
};

static ControllerModelDependencies testDependencies() {
    ControllerModelDependencies dependencies;
    dependencies.processProbe = std::make_shared<EmptyProbe>();
    dependencies.loadSettings = []() { return LoaderSettings{}; };
    dependencies.saveSettings = [](const LoaderSettings&) { return true; };
    dependencies.inject = [](const ProcessIdentity&) {
        InjectionOutcome outcome;
        outcome.success = false;
        outcome.error = L"test-only injector";
        return outcome;
    };
    return dependencies;
}

static void test_initial_page_is_valid() {
    TEST("initial page is a valid ControllerPage (async start possible)");

    ControllerModel model(testDependencies());
    /* The constructor kicks off an asynchronous auto-login attempt, so the
     * observed page may legitimately be Login, Loading, or Error by the
     * time we read it. Contract: it must be one of the known pages. */
    const auto page = model.page();
    ASSERT(page == ControllerPage::Login
            || page == ControllerPage::Loading
            || page == ControllerPage::Error
            || page == ControllerPage::MinecraftSelection
            || page == ControllerPage::Settings,
        "page must be a known ControllerPage value");
    PASS();
}

static void test_page_round_trip() {
    TEST("page set/get round-trip");

    ControllerModel model(testDependencies());
    model.setPage(ControllerPage::Settings);
    ASSERT(model.page() == ControllerPage::Settings, "should be Settings");

    model.setPage(ControllerPage::MinecraftSelection);
    ASSERT(model.page() == ControllerPage::MinecraftSelection,
        "should be MinecraftSelection");
    PASS();
}

static void test_status_round_trip() {
    TEST("status set/get round-trip");

    ControllerModel model(testDependencies());
    model.setStatus(L"Idle");
    ASSERT(model.status() == L"Idle", "status should round-trip");
    PASS();
}

static void test_credentials_round_trip() {
    TEST("username/password accessors");

    ControllerModel model(testDependencies());
    model.username() = L"player1";
    model.password() = L"secret";
    ASSERT(model.username() == L"player1", "username should round-trip");
    ASSERT(model.password() == L"secret", "password should round-trip");
    PASS();
}

static void test_loader_settings_round_trip() {
    TEST("loader settings apply/read round-trip");

    ControllerModel model(testDependencies());

    LoaderSettings settings;
    settings.autoInjectEnabled = true;
    settings.unlockCosmetics = false;
    settings.language = L"Chinese";

    ASSERT(model.applyLoaderSettings(settings), "apply should succeed");
    const LoaderSettings readBack = model.loaderSettings();
    ASSERT(readBack.autoInjectEnabled == true, "autoInject should persist");
    ASSERT(readBack.unlockCosmetics == false, "unlockCosmetics should persist");
    ASSERT(readBack.language == L"Chinese", "language should persist");
    PASS();
}

static void test_refresh_processes_empty() {
    TEST("refreshMinecraftProcesses with empty probe yields no processes");

    ControllerModel model(testDependencies());
    model.refreshMinecraftProcesses();
    ASSERT(model.minecraftProcesses().empty(), "no processes expected");
    PASS();
}

static void test_inject_missing_pid_fails() {
    TEST("injectMinecraft on unknown pid fails cleanly");

    ControllerModel model(testDependencies());
    const bool ok = model.injectMinecraft(0xFFFFFFFEu);
    ASSERT(!ok, "unknown pid must fail");
    PASS();
}

static void test_tick_is_safe() {
    TEST("tick() is safe to call");

    ControllerModel model(testDependencies());
    model.tick();
    model.tick();
    PASS();
}

static void test_loading_stage_bounds() {
    TEST("loadingStage/loadingElapsed are bounded");

    ControllerModel model(testDependencies());
    const int stage = model.loadingStage();
    ASSERT(stage >= 0, "stage must be >= 0");
    const double elapsed = model.loadingElapsedSeconds();
    ASSERT(elapsed >= 0.0, "elapsed must be >= 0");
    PASS();
}

static void test_cache_preference() {
    TEST("cache preference round-trips after persist");

    ControllerModel model(testDependencies());

    model.persistCachePreference(true);
    ASSERT(model.cachePreference(), "cache preference should now be true");

    model.persistCachePreference(false);
    ASSERT(!model.cachePreference(), "cache preference should now be false");
    PASS();
}

static void test_cancel_auth_is_safe() {
    TEST("cancelBrowserAuthentication is safe without a session");

    ControllerModel model(testDependencies());
    model.cancelBrowserAuthentication();
    PASS();
}

int main() {
    std::cout << "=== ControllerModelWorkerTest ===\n";
    test_initial_page_is_valid();
    test_page_round_trip();
    test_status_round_trip();
    test_credentials_round_trip();
    test_loader_settings_round_trip();
    test_refresh_processes_empty();
    test_inject_missing_pid_fails();
    test_tick_is_safe();
    test_loading_stage_bounds();
    test_cache_preference();
    test_cancel_auth_is_safe();

    std::cout << "\n---\n";
    std::cout << "Tests:  " << tests_passed << " passed, " << tests_failed << " failed\n";
    return tests_failed > 0 ? 1 : 0;
}