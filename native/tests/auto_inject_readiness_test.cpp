#include <cstdio>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include "../loader_gui/auto_inject_readiness.h"
#include "../loader_gui/loader_settings.h"

/* Minimal test harness (no gtest — CTest runs directly). */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) std::cout << "  TEST: " << name << " ... "
#define PASS() do { std::cout << "PASS\n"; tests_passed++; } while (0)
#define FAIL(msg) do { std::cout << "FAIL: " << msg << "\n"; tests_failed++; } while (0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while (0)

using namespace std::chrono_literals;

namespace {

ProcessObservation readyObservation() {
    ProcessObservation observation;
    observation.identity.pid = 1234;
    observation.identity.creationTime100ns = 133300000000000000ULL;  // plausible
    observation.exeName = L"java.exe";
    observation.commandLine = L"javaw.exe --username foo --uuid 1234 "
        L"--accesstoken abc --usertype mojang --versiontype release "
        L"--gamedir C:\\mc --assetsdir C:\\mc\\assets --assetindex 1.19 "
        L"--version 1.19 com.moonsworth.lunar.genesis.Genesis";
    observation.commandLineReadable = true;
    observation.title = L"Lunar Client 1.19";
    observation.windowClass = L"LWJGL";
    observation.visible = true;
    observation.responsive = true;
    observation.processAge = 60s;
    observation.titleStableFor = 10s;
    return observation;
}

AutoInjectPolicy readyPolicy() {
    AutoInjectPolicy policy;
    policy.mode = AutoInjectReadiness::Conservative;
    policy.settleDelay = 5000ms;
    policy.minimumAge = 15000ms;
    return policy;
}

}  // namespace

static void test_ready_process() {
    TEST("ready java+Lunar process evaluates Ready");

    const AutoInjectEvaluation result = evaluateAutoInjectReadiness(
        readyObservation(), readyPolicy());
    ASSERT(result.ready, "should be ready");
    ASSERT(result.reason == AutoInjectReadinessReason::Ready,
        "reason should be Ready");

    PASS();
}

static void test_rejects_non_java() {
    TEST("non-java executable rejected");

    ProcessObservation observation = readyObservation();
    observation.exeName = L"chrome.exe";

    const AutoInjectEvaluation result = evaluateAutoInjectReadiness(
        observation, readyPolicy());
    ASSERT(!result.ready, "should not be ready");
    ASSERT(result.reason == AutoInjectReadinessReason::NotJavaExecutable,
        "reason should be NotJavaExecutable");

    PASS();
}

static void test_rejects_gradle_ide_tooling() {
    TEST("gradle/IDE/tooling command lines rejected");

    const wchar_t* badCommandLines[] = {
        L"java.exe org.gradle.launcher.GradleMain",
        L"java.exe -javaagent:C:\\idea_rt.jar com.intellij.rt",
        L"java.exe C:\\gradle-8.8\\lib\\gradle-launcher-8.8.jar",
        L"java.exe net.minecraft.server.MinecraftServer",
        L"java.exe server.jar nogui",
        L"java.exe runclient",
    };

    for (const wchar_t* commandLine : badCommandLines) {
        ProcessObservation observation = readyObservation();
        observation.commandLine = commandLine;

        const AutoInjectEvaluation result = evaluateAutoInjectReadiness(
            observation, readyPolicy());
        if (result.ready) {
            std::wcout << L"    FAIL: accepted rejected command line: "
                << commandLine << L"\n";
            tests_failed++;
            return;
        }
        ASSERT(result.reason == AutoInjectReadinessReason::RejectedCommandLine,
            "reason should be RejectedCommandLine");
    }

    PASS();
}

static void test_rejects_vanilla_missing_game_markers() {
    TEST("non-game java process (no positive game markers) rejected");

    ProcessObservation observation = readyObservation();
    observation.commandLine = L"java.exe -jar some_random_app.jar";

    const AutoInjectEvaluation result = evaluateAutoInjectReadiness(
        observation, readyPolicy());
    ASSERT(!result.ready, "should not be ready");
    ASSERT(result.reason == AutoInjectReadinessReason::GameCommandLineNotFound,
        "reason should be GameCommandLineNotFound");

    PASS();
}

static void test_accepts_vanilla_with_mc_args() {
    TEST("vanilla MC with --gamedir/--assetsdir/--version accepted");

    ProcessObservation observation = readyObservation();
    observation.commandLine = L"java.exe -Xmx1G --gamedir C:\\mc "
        L"--assetsdir C:\\mc\\assets --assetindex 1.19 --version 1.19 "
        L"net.minecraft.client.main.Main";

    const AutoInjectEvaluation result = evaluateAutoInjectReadiness(
        observation, readyPolicy());
    ASSERT(result.ready, "vanilla MC should be ready");

    PASS();
}

static void test_rejects_hidden_window() {
    TEST("invisible window rejected");

    ProcessObservation observation = readyObservation();
    observation.visible = false;

    const AutoInjectEvaluation result = evaluateAutoInjectReadiness(
        observation, readyPolicy());
    ASSERT(!result.ready, "should not be ready");
    ASSERT(result.reason == AutoInjectReadinessReason::WindowNotVisible,
        "reason should be WindowNotVisible");

    PASS();
}

static void test_rejects_unresponsive_window() {
    TEST("unresponsive window rejected");

    ProcessObservation observation = readyObservation();
    observation.responsive = false;

    const AutoInjectEvaluation result = evaluateAutoInjectReadiness(
        observation, readyPolicy());
    ASSERT(!result.ready, "should not be ready");
    ASSERT(result.reason == AutoInjectReadinessReason::WindowNotResponsive,
        "reason should be WindowNotResponsive");

    PASS();
}

static void test_rejects_blank_title() {
    TEST("blank window title rejected");

    ProcessObservation observation = readyObservation();
    observation.title = L"   \t ";

    const AutoInjectEvaluation result = evaluateAutoInjectReadiness(
        observation, readyPolicy());
    ASSERT(!result.ready, "should not be ready");
    ASSERT(result.reason == AutoInjectReadinessReason::WindowTitleBlank,
        "reason should be WindowTitleBlank");

    PASS();
}

static void test_rejects_non_game_window() {
    TEST("non-game window class/title rejected");

    ProcessObservation observation = readyObservation();
    observation.title = L"Notepad";
    observation.windowClass = L"Notepad";

    const AutoInjectEvaluation result = evaluateAutoInjectReadiness(
        observation, readyPolicy());
    ASSERT(!result.ready, "should not be ready");
    ASSERT(result.reason == AutoInjectReadinessReason::GameWindowNotFound,
        "reason should be GameWindowNotFound");

    PASS();
}

static void test_rejects_minecraft_server_title() {
    TEST("minecraft server window title rejected");

    ProcessObservation observation = readyObservation();
    observation.title = L"Minecraft Server 1.19";

    const AutoInjectEvaluation result = evaluateAutoInjectReadiness(
        observation, readyPolicy());
    ASSERT(!result.ready, "should not be ready");

    PASS();
}

static void test_rejects_young_process() {
    TEST("process younger than minimumAge rejected");

    ProcessObservation observation = readyObservation();
    observation.processAge = 5s;

    const AutoInjectEvaluation result = evaluateAutoInjectReadiness(
        observation, readyPolicy());
    ASSERT(!result.ready, "should not be ready");
    ASSERT(result.reason == AutoInjectReadinessReason::ProcessTooYoung,
        "reason should be ProcessTooYoung");

    PASS();
}

static void test_rejects_unstable_window() {
    TEST("window not stable for settleDelay rejected");

    ProcessObservation observation = readyObservation();
    observation.titleStableFor = 1s;

    const AutoInjectEvaluation result = evaluateAutoInjectReadiness(
        observation, readyPolicy());
    ASSERT(!result.ready, "should not be ready");
    ASSERT(result.reason == AutoInjectReadinessReason::WindowNotStable,
        "reason should be WindowNotStable");

    PASS();
}

static void test_rejects_invalid_identity() {
    TEST("zero pid or creation time rejected");

    ProcessObservation observation = readyObservation();
    observation.identity.pid = 0;

    const AutoInjectEvaluation result = evaluateAutoInjectReadiness(
        observation, readyPolicy());
    ASSERT(!result.ready, "pid 0 should not be ready");
    ASSERT(result.reason == AutoInjectReadinessReason::InvalidIdentity,
        "reason should be InvalidIdentity");

    observation = readyObservation();
    observation.identity.creationTime100ns = 0;
    const AutoInjectEvaluation result2 = evaluateAutoInjectReadiness(
        observation, readyPolicy());
    ASSERT(!result2.ready, "creationTime 0 should not be ready");

    PASS();
}

static void test_rejects_already_injected() {
    TEST("alreadyInjected / alreadyAttempted rejected");

    ProcessObservation observation = readyObservation();
    observation.alreadyInjected = true;
    const AutoInjectEvaluation result = evaluateAutoInjectReadiness(
        observation, readyPolicy());
    ASSERT(!result.ready, "alreadyInjected should not be ready");
    ASSERT(result.reason == AutoInjectReadinessReason::AlreadyInjected,
        "reason should be AlreadyInjected");

    observation = readyObservation();
    observation.alreadyAttempted = true;
    const AutoInjectEvaluation result2 = evaluateAutoInjectReadiness(
        observation, readyPolicy());
    ASSERT(!result2.ready, "alreadyAttempted should not be ready");
    ASSERT(result2.reason == AutoInjectReadinessReason::AlreadyAttempted,
        "reason should be AlreadyAttempted");

    PASS();
}

static void test_rejects_invalid_policy() {
    TEST("negative policy durations rejected");

    AutoInjectPolicy policy = readyPolicy();
    policy.minimumAge = -1ms;

    const AutoInjectEvaluation result = evaluateAutoInjectReadiness(
        readyObservation(), policy);
    ASSERT(!result.ready, "negative minAge should not be ready");
    ASSERT(result.reason == AutoInjectReadinessReason::InvalidPolicy,
        "reason should be InvalidPolicy");

    PASS();
}

static void test_window_stable_mode_relaxes_cmdline() {
    TEST("WindowStable mode accepts unreadable command line");

    AutoInjectPolicy policy = readyPolicy();
    policy.mode = AutoInjectReadiness::WindowStable;

    ProcessObservation observation = readyObservation();
    observation.commandLineReadable = false;

    const AutoInjectEvaluation result = evaluateAutoInjectReadiness(
        observation, policy);
    ASSERT(result.ready, "WindowStable should accept unreadable cmdline");

    PASS();
}

static void test_conservative_requires_readable_cmdline() {
    TEST("Conservative mode rejects unreadable command line");

    ProcessObservation observation = readyObservation();
    observation.commandLineReadable = false;

    const AutoInjectEvaluation result = evaluateAutoInjectReadiness(
        observation, readyPolicy());
    ASSERT(!result.ready, "should not be ready");
    ASSERT(result.reason == AutoInjectReadinessReason::CommandLineUnreadable,
        "reason should be CommandLineUnreadable");

    PASS();
}

static void test_select_candidate_picks_newest() {
    TEST("selectAutoInjectCandidate picks newest ready process");

    ProcessObservation older = readyObservation();
    older.identity.pid = 100;
    older.identity.creationTime100ns = 1000;

    ProcessObservation newer = readyObservation();
    newer.identity.pid = 200;
    newer.identity.creationTime100ns = 2000;

    ProcessObservation notReady = readyObservation();
    notReady.identity.pid = 300;
    notReady.identity.creationTime100ns = 3000;
    notReady.visible = false;  // not ready

    const std::vector<ProcessObservation> observations = {older, notReady, newer};
    const auto selected = selectAutoInjectCandidate(observations, readyPolicy());
    ASSERT(selected.has_value(), "should select a candidate");
    ASSERT(selected->pid == 200, "should select newest ready (pid 200)");

    PASS();
}

static void test_select_candidate_empty() {
    TEST("selectAutoInjectCandidate on empty/not-ready list returns nullopt");

    ASSERT(!selectAutoInjectCandidate({}, readyPolicy()).has_value(),
        "empty list should yield nullopt");

    ProcessObservation notReady = readyObservation();
    notReady.visible = false;
    ASSERT(!selectAutoInjectCandidate({notReady}, readyPolicy()).has_value(),
        "not-ready list should yield nullopt");

    PASS();
}

static void test_select_candidate_tiebreak_lower_pid() {
    TEST("creation-time tie breaks to lower pid");

    ProcessObservation a = readyObservation();
    a.identity.pid = 500;
    a.identity.creationTime100ns = 1000;

    ProcessObservation b = readyObservation();
    b.identity.pid = 100;
    b.identity.creationTime100ns = 1000;

    const auto selected = selectAutoInjectCandidate({a, b}, readyPolicy());
    ASSERT(selected.has_value(), "should select");
    ASSERT(selected->pid == 100, "tie should pick lower pid");

    PASS();
}

int main() {
    std::cout << "=== AutoInjectReadinessTest ===\n";
    test_ready_process();
    test_rejects_non_java();
    test_rejects_gradle_ide_tooling();
    test_rejects_vanilla_missing_game_markers();
    test_accepts_vanilla_with_mc_args();
    test_rejects_hidden_window();
    test_rejects_unresponsive_window();
    test_rejects_blank_title();
    test_rejects_non_game_window();
    test_rejects_minecraft_server_title();
    test_rejects_young_process();
    test_rejects_unstable_window();
    test_rejects_invalid_identity();
    test_rejects_already_injected();
    test_rejects_invalid_policy();
    test_window_stable_mode_relaxes_cmdline();
    test_conservative_requires_readable_cmdline();
    test_select_candidate_picks_newest();
    test_select_candidate_empty();
    test_select_candidate_tiebreak_lower_pid();

    std::cout << "\n---\n";
    std::cout << "Tests:  " << tests_passed << " passed, " << tests_failed << " failed\n";
    return tests_failed > 0 ? 1 : 0;
}