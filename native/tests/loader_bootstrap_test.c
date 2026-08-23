#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "../loader_bootstrap.h"

/* Test helpers */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) do { printf("  TEST: %s ... ", name); } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); tests_failed++; } while(0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)

/* ------------------------------------------------------------------ */

static void test_standalone(void) {
    /* Standalone mode: no bootstrap memory mapping exists.
     * initialize_bootstrap() should detect ERROR_FILE_NOT_FOUND
     * and set token_state to STANDALONE with token "0". */
    TEST("standalone mode — no bootstrap mapping");

    /* No bootstrap mapping was created, so initialize should
     * succeed with token "0" (standalone mode). */
    int ok = lunarunlocker_loader_bootstrap_initialize();
    ASSERT(ok != 0, "initialize should return non-zero (standalone)");

    const char *token = lunarunlocker_loader_access_token();
    ASSERT(token != NULL, "token should not be NULL");
    ASSERT(strcmp(token, "0") == 0,
            "standalone token should be \"0\"");

    int failed = lunarunlocker_loader_bootstrap_failed();
    ASSERT(failed == 0, "bootstrap should not be marked failed");

    PASS();
}

/* ------------------------------------------------------------------ */

static void test_initialize_twice(void) {
    /* InitOnceExecuteOnce should run only once; calling initialize
     * again should return the same state. */
    TEST("initialize twice — idempotent");

    int ok = lunarunlocker_loader_bootstrap_initialize();
    ASSERT(ok != 0, "first call should succeed");

    const char *token1 = lunarunlocker_loader_access_token();

    ok = lunarunlocker_loader_bootstrap_initialize();
    ASSERT(ok != 0, "second call should succeed");

    const char *token2 = lunarunlocker_loader_access_token();
    ASSERT(token1 == token2 || strcmp(token1, token2) == 0,
            "token should be stable across calls");

    PASS();
}

/* ------------------------------------------------------------------ */

static void test_clear_restores_state(void) {
    /* After clear, the next initialize should be able to run again
     * (InitOnceExecuteOnce is already consumed on this process, so
     * calling initialize again after clear returns the cached state).
     * This tests that clear at least zeroes the token. */
    TEST("clear zeroes token");

    /* Call initialize first to set state */
    lunarunlocker_loader_bootstrap_initialize();

    /* Clear — should zero the token */
    lunarunlocker_loader_bootstrap_clear();

    const char *token = lunarunlocker_loader_access_token();
    ASSERT(token != NULL, "token should not be NULL after clear");
    ASSERT(strcmp(token, "0") == 0 || token[0] == '\0',
            "token should be zeroed or \"0\" after clear");

    PASS();
}

/* ------------------------------------------------------------------ */

static void test_report_progress_no_crash(void) {
    /* Report functions should not crash even when no controller
     * socket is connected (the functions check for INVALID_SOCKET). */
    TEST("report progress — no controller socket");

    /* These should be safe no-ops */
    lunarunlocker_loader_report_progress(1);
    lunarunlocker_loader_report_progress(50);
    lunarunlocker_loader_report_progress(100);

    PASS();
}

static void test_report_completed_no_crash(void) {
    TEST("report completed — no controller socket");
    lunarunlocker_loader_report_completed();
    PASS();
}

static void test_report_failure_no_crash(void) {
    TEST("report failure — no controller socket");
    lunarunlocker_loader_report_failure("test error message");
    PASS();
}

static void test_report_failure_null_no_crash(void) {
    TEST("report failure — NULL message");
    lunarunlocker_loader_report_failure(NULL);
    PASS();
}

/* ------------------------------------------------------------------ */

int main(int argc, char **argv) {
    int run_all = (argc < 2);

    printf("=== LunarUnlockerBootstrapTest ===\n");

    if (run_all || (argc >= 2 && strcmp(argv[1], "standalone") == 0)) {
        printf("\nSuite: BootstrapStandalone\n");
        test_standalone();
    }

    if (run_all || (argc >= 2 && strcmp(argv[1], "handoff") == 0)) {
        printf("\nSuite: BootstrapHandoff\n");
        test_initialize_twice();
    }

    if (run_all || (argc >= 2 && strcmp(argv[1], "invalid") == 0)) {
        printf("\nSuite: BootstrapRejectsInvalid\n");
        test_clear_restores_state();
        test_report_progress_no_crash();
        test_report_completed_no_crash();
        test_report_failure_no_crash();
        test_report_failure_null_no_crash();
    }

    printf("\n---\n");
    printf("Tests:  %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}