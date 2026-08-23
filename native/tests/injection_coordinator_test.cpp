#include <cstdio>
#include <iostream>
#include <string>

#include <windows.h>

#include "../loader_gui/injection_coordinator.h"

 
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) std::cout << "  TEST: " << name << " ... "
#define PASS() do { std::cout << "PASS\n"; tests_passed++; } while (0)
#define FAIL(msg) do { std::cout << "FAIL: " << msg << "\n"; tests_failed++; } while (0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while (0)

static ProcessIdentity identity(std::uint32_t pid, std::uint64_t creation) {
    ProcessIdentity value;
    value.pid = pid;
    value.creationTime100ns = creation;
    return value;
}

 

static void test_inject_nonexistent_pid() {
    TEST("injectProductDll — nonexistent pid fails cleanly");

    InjectionOutcome outcome = InjectionCoordinator::injectProductDll(
        identity(0xFFFFFFFEu, 1), L"C:\\nonexistent\\payload.dll",
        43100, "http://127.0.0.1:43100");
    ASSERT(!outcome.success, "injection must fail for nonexistent pid");
    ASSERT(!outcome.error.empty() || outcome.retrySafe,
        "error message should be set or failure must be retry-safe");
    PASS();
}

static void test_inject_zero_pid() {
    TEST("injectProductDll — pid 0 rejected");

    InjectionOutcome outcome = InjectionCoordinator::injectProductDll(
        identity(0, 1), L"C:\\nonexistent\\payload.dll", 43100, "");
    ASSERT(!outcome.success, "pid 0 must fail");
    ASSERT(!outcome.remoteThreadStarted, "no remote thread may start for pid 0");
    PASS();
}

static void test_inject_self_missing_dll() {
    TEST("injectProductDll — self process with missing DLL fails");

    InjectionOutcome outcome = InjectionCoordinator::injectProductDll(
        identity(GetCurrentProcessId(), 1),
        L"C:\\definitely\\missing\\payload.dll", 43100, "");
    ASSERT(!outcome.success, "missing DLL must fail");
    PASS();
}

static void test_inject_self_missing_dll_bool_overload() {
    TEST("injectProductDll — bool overload reports error");

    std::wstring error;
    const bool ok = InjectionCoordinator::injectProductDll(
        GetCurrentProcessId(), L"C:\\definitely\\missing\\payload.dll",
        43100, "", error);
    ASSERT(!ok, "missing DLL must fail");
    ASSERT(!error.empty(), "bool overload should populate error");
    PASS();
}

static void test_inject_reflective_nonexistent_pid() {
    TEST("injectReflectiveDll — nonexistent pid fails cleanly");

    InjectionOutcome outcome = InjectionCoordinator::injectReflectiveDll(
        identity(0xFFFFFFFEu, 1), L"C:\\nonexistent\\payload.dll", 43100);
    ASSERT(!outcome.success, "reflective injection must fail for nonexistent pid");
    PASS();
}

static void test_inject_reflective_zero_pid() {
    TEST("injectReflectiveDll — pid 0 rejected");

    InjectionOutcome outcome = InjectionCoordinator::injectReflectiveDll(
        identity(0, 1), L"C:\\nonexistent\\payload.dll", 43100);
    ASSERT(!outcome.success, "pid 0 must fail");
    PASS();
}

static void test_inject_reflective_self_missing_dll() {
    TEST("injectReflectiveDll — self with missing DLL fails");

    InjectionOutcome outcome = InjectionCoordinator::injectReflectiveDll(
        identity(GetCurrentProcessId(), 1),
        L"C:\\definitely\\missing\\payload.dll", 43100);
    ASSERT(!outcome.success, "missing DLL must fail");
    PASS();
}

int main() {
    std::cout << "=== InjectionCoordinatorTest ===\n";
    test_inject_nonexistent_pid();
    test_inject_zero_pid();
    test_inject_self_missing_dll();
    test_inject_self_missing_dll_bool_overload();
    test_inject_reflective_nonexistent_pid();
    test_inject_reflective_zero_pid();
    test_inject_reflective_self_missing_dll();

    std::cout << "\n---\n";
    std::cout << "Tests:  " << tests_passed << " passed, " << tests_failed << " failed\n";
    return tests_failed > 0 ? 1 : 0;
}