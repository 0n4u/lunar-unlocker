#include "lunar_unlock_engine.h"

#include <windows.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

extern "C" int lunarunlocker_direct_engine_run(HMODULE module);

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

LunarFeatureResult result(LunarFeatureId feature, LunarFeatureState state,
                          int catalog, int owned) {
    LunarFeatureResult value{};
    value.feature = feature;
    value.state = state;
    value.catalog_count = catalog;
    value.owned_count = owned;
    return value;
}

}  // namespace

int main() {
    require(lunarunlocker_direct_engine_run(nullptr) == 0,
            "the embedded direct engine rejects a missing module");
    LunarUnlockReport complete{};
    complete.features[0] = result(LUNAR_FEATURE_COSMETICS,
                                  LUNAR_FEATURE_UNLOCKED, 5513, 5513);
    complete.features[1] = result(LUNAR_FEATURE_EMOTES,
                                  LUNAR_FEATURE_UNLOCKED, 182, 182);
    complete.features[2] = result(LUNAR_FEATURE_SPRAYS,
                                  LUNAR_FEATURE_UNAVAILABLE, 0, 0);
    complete.feature_count = 3;
    require(lunar_unlock_report_succeeded(&complete) == 1,
            "unavailable optional features do not make a verified run fail");

    LunarUnlockReport partial = complete;
    partial.features[1].state = LUNAR_FEATURE_FAILED;
    require(lunar_unlock_report_succeeded(&partial) == 0,
            "a present failed feature must make the run fail");

    LunarUnlockReport missingCore = complete;
    missingCore.features[0].state = LUNAR_FEATURE_UNAVAILABLE;
    require(lunar_unlock_report_succeeded(&missingCore) == 0,
            "cosmetics are required even when a profile lacks optional features");

    LunarUnlockReport mismatched = complete;
    mismatched.features[0].owned_count = 5512;
    require(lunar_unlock_report_succeeded(&mismatched) == 0,
            "catalog and owned counts must match exactly");
    require(lunar_unlock_counts_match(5513, 5513, 5513) == 1,
            "matching live ownership counts verify");
    require(lunar_unlock_counts_match(5513, 5512, 5512) == 0,
            "a partial live catalog never verifies");
    require(lunar_unlock_should_repair_manager(5513, 4, 4) == 1,
            "a replacement manager with reset ownership is repaired");
    require(lunar_unlock_should_repair_manager(5513, 5513, 5513) == 0,
            "a fully owned manager is not needlessly rewritten");

    const char* name = lunar_feature_name(LUNAR_FEATURE_LUNAR_PLUS);
    require(name != nullptr && std::strcmp(name, "lunar_plus") == 0,
            "feature names are stable for controller status messages");
    return 0;
}
