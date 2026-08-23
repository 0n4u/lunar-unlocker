#include <chrono>
#include <cstdio>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include <windows.h>

#include "../loader_gui/auto_inject_readiness.h"
#include "../loader_gui/win32_process_probe.h"

 
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) std::cout << "  TEST: " << name << " ... "
#define PASS() do { std::cout << "PASS\n"; tests_passed++; } while (0)
#define FAIL(msg) do { std::cout << "FAIL: " << msg << "\n"; tests_failed++; } while (0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while (0)

using namespace std::chrono_literals;

static ProcessIdentity identity(std::uint32_t pid, std::uint64_t creation) {
    ProcessIdentity value;
    value.pid = pid;
    value.creationTime100ns = creation;
    return value;
}

static void test_valid_identity() {
    TEST("isValidProcessIdentity");
    ASSERT(!isValidProcessIdentity(identity(0, 1000)), "pid 0 invalid");
    ASSERT(!isValidProcessIdentity(identity(100, 0)), "creation 0 invalid");
    ASSERT(isValidProcessIdentity(identity(100, 1000)), "valid identity");
    PASS();
}

static void test_same_identity() {
    TEST("sameProcessIdentity");
    ASSERT(sameProcessIdentity(identity(1, 100), identity(1, 100)), "identical");
    ASSERT(!sameProcessIdentity(identity(1, 100), identity(2, 100)), "pid differs");
    ASSERT(!sameProcessIdentity(identity(1, 100), identity(1, 200)), "creation differs");
    PASS();
}

static void test_identity_less() {
    TEST("ProcessIdentityLess ordering");
    ProcessIdentityLess less;
    ASSERT(less(identity(1, 100), identity(2, 100)), "pid ordering");
    ASSERT(less(identity(2, 50), identity(2, 100)), "creation ordering");
    ASSERT(!less(identity(2, 100), identity(2, 100)), "not less than equal");
    PASS();
}

static void test_is_auto_inject_candidate() {
    TEST("isAutoInjectCandidate delegates to readiness evaluation");

    ProcessObservation observation;
    observation.identity = identity(1234, 133300000000000000ULL);
    observation.exeName = L"java.exe";
    observation.commandLine = L"java.exe --gamedir C:\\mc --assetsdir C:\\mc\\assets "
        L"--assetindex 1.19 --version 1.19 net.minecraft.client.main.Main";
    observation.commandLineReadable = true;
    observation.title = L"Lunar Client";
    observation.windowClass = L"LWJGL";
    observation.visible = true;
    observation.responsive = true;
    observation.processAge = 60s;
    observation.titleStableFor = 10s;

    AutoInjectPolicy policy;
    policy.settleDelay = 5000ms;
    policy.minimumAge = 15000ms;

    ASSERT(isAutoInjectCandidate(observation, policy), "ready candidate");

    observation.visible = false;
    ASSERT(!isAutoInjectCandidate(observation, policy), "hidden rejected");
    PASS();
}

static void test_select_newest() {
    TEST("selectNewestAutoInjectCandidate picks newest ready");

    ProcessObservation older;
    older.identity = identity(100, 1000);
    older.exeName = L"java.exe";
    older.commandLine = L"java.exe --gamedir C:\\mc --assetsdir C:\\mc\\assets "
        L"--assetindex 1.19 --version 1.19 net.minecraft.client.main.Main";
    older.commandLineReadable = true;
    older.title = L"Minecraft";
    older.windowClass = L"LWJGL";
    older.visible = true;
    older.responsive = true;
    older.processAge = 60s;
    older.titleStableFor = 10s;

    ProcessObservation newer = older;
    newer.identity = identity(200, 2000);

    ProcessObservation rejected = newer;
    rejected.identity = identity(300, 3000);
    rejected.visible = false;

    AutoInjectPolicy policy;
    policy.settleDelay = 5000ms;
    policy.minimumAge = 15000ms;

    const auto selected = selectNewestAutoInjectCandidate(
        {rejected, older, newer}, policy);
    ASSERT(selected.has_value(), "should select");
    ASSERT(selected->pid == 200, "should select newest ready (pid 200)");

    const auto none = selectNewestAutoInjectCandidate({rejected}, policy);
    ASSERT(!none.has_value(), "no ready candidates");

    const auto empty = selectNewestAutoInjectCandidate({}, policy);
    ASSERT(!empty.has_value(), "empty list");

    PASS();
}

static void test_query_identity_self() {
    TEST("queryProcessIdentity for own process");

    ProcessIdentity own;
    ASSERT(queryProcessIdentity(GetCurrentProcessId(), own), "should resolve own pid");
    ASSERT(own.pid == GetCurrentProcessId(), "pid matches");
    ASSERT(own.creationTime100ns != 0, "creation time non-zero");

    ProcessIdentity invalid;
    ASSERT(!queryProcessIdentity(0xFFFFFFFFu, invalid), "invalid pid rejected");
    PASS();
}

static void test_opened_process_matches_identity() {
    TEST("openedProcessMatchesIdentity");

    ProcessIdentity own;
    ASSERT(queryProcessIdentity(GetCurrentProcessId(), own), "resolve own");

    HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
        GetCurrentProcessId());
    ASSERT(handle != nullptr, "OpenProcess self");

    ASSERT(openedProcessMatchesIdentity(handle, own), "handle matches identity");
    ASSERT(!openedProcessMatchesIdentity(handle, identity(1, 1)), "wrong identity rejected");

    CloseHandle(handle);
    PASS();
}

static void test_probe_lists_java_processes() {
    TEST("Win32ProcessProbe returns observations without crashing");

    Win32ProcessProbe probe;
    const std::vector<ProcessObservation> observations = probe.probe();
     
    for (const auto& observation : observations) {
        ASSERT(observation.identity.pid != 0,
            "probe must not return zero pid");
        ASSERT(observation.identity.creationTime100ns != 0,
            "probe must not return zero creation time");
    }

    ProcessIdentity realSelf;
    ASSERT(queryProcessIdentity(GetCurrentProcessId(), realSelf),
        "resolve own identity");
    ASSERT(probe.isCurrent(realSelf),
        "isCurrent should accept the real current identity");

    const ProcessIdentity ghost = identity(0xFFFFFFFEu, 1);
    ASSERT(!probe.isCurrent(ghost), "isCurrent should reject non-existent pid");
    PASS();
}

int main() {
    std::cout << "=== Win32ProcessProbeTest ===\n";
    test_valid_identity();
    test_same_identity();
    test_identity_less();
    test_is_auto_inject_candidate();
    test_select_newest();
    test_query_identity_self();
    test_opened_process_matches_identity();
    test_probe_lists_java_processes();

    std::cout << "\n---\n";
    std::cout << "Tests:  " << tests_passed << " passed, " << tests_failed << " failed\n";
    return tests_failed > 0 ? 1 : 0;
}