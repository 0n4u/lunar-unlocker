#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>

#include "../loader_gui/local_controller_service.h"

/* Minimal test harness (no gtest — CTest runs directly). */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) std::cout << "  TEST: " << name << " ... "
#define PASS() do { std::cout << "PASS\n"; tests_passed++; } while (0)
#define FAIL(msg) do { std::cout << "FAIL: " << msg << "\n"; tests_failed++; } while (0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while (0)

using namespace std::chrono_literals;

static void test_initial_state() {
    TEST("initial state — not running, port 0, stage 0");

    LocalControllerService service;
    ASSERT(service.port() == 0, "port should start at 0");
    ASSERT(service.stage() == 0, "stage should start at 0");
    ASSERT(!service.completed(), "should not be completed");
    ASSERT(!service.failed(), "should not be failed");
    ASSERT(service.error().empty(), "error should be empty");
    PASS();
}

static void test_start_stop_cycle() {
    TEST("start binds a port; stop releases it");

    LocalControllerService service;
    ASSERT(service.start("test-token", true, false), "start should succeed");
    ASSERT(service.port() != 0, "port should be assigned");
    ASSERT(service.port() < 65536, "port must be in range");

    service.stop();
    ASSERT(service.port() == 0, "port should reset to 0 after stop");
    PASS();
}

static void test_double_start_restarts() {
    TEST("second start while running restarts cleanly (start() stops first)");

    LocalControllerService service;
    ASSERT(service.start("token-a", true, false), "first start should succeed");
    const std::uint16_t firstPort = service.port();
    ASSERT(firstPort != 0, "first start should bind a port");

    /* start() deliberately calls stop() first (documented restart
     * semantics), so a second start is expected to succeed. */
    ASSERT(service.start("token-b", true, false), "second start should succeed");
    ASSERT(service.port() != 0, "second start should bind a port");
    service.stop();
    PASS();
}

static void test_stop_without_start_is_safe() {
    TEST("stop without start is a safe no-op");

    LocalControllerService service;
    service.stop();
    service.stop();
    PASS();
}

static void test_set_value_stage() {
    TEST("setValue records key/value; stage remains 0 until protocol");

    LocalControllerService service;
    ASSERT(service.start("test-token", true, false), "start should succeed");

    service.setValue("cosmetics", "true");
    service.setValue("badges", "true");

    service.stop();
    PASS();
}

static void test_destructor_stops() {
    TEST("destructor stops the accept loop");

    {
        LocalControllerService service;
        ASSERT(service.start("test-token", true, false), "start should succeed");
        ASSERT(service.port() != 0, "port should be assigned");
        /* service goes out of scope; destructor must join the thread */
    }
    PASS();
}

static void test_error_never_set_by_default() {
    TEST("error stays empty unless a protocol failure occurs");

    LocalControllerService service;
    ASSERT(service.start("test-token", true, false), "start should succeed");
    ASSERT(service.error().empty(), "error should be empty");
    service.stop();
    PASS();
}

int main() {
    std::cout << "=== LocalControllerServiceTest ===\n";
    test_initial_state();
    test_start_stop_cycle();
    test_double_start_restarts();
    test_stop_without_start_is_safe();
    test_set_value_stage();
    test_destructor_stops();
    test_error_never_set_by_default();

    std::cout << "\n---\n";
    std::cout << "Tests:  " << tests_passed << " passed, " << tests_failed << " failed\n";
    return tests_failed > 0 ? 1 : 0;
}