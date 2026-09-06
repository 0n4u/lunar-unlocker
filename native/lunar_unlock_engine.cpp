#include "lunar_unlock_engine.h"

#include <cstddef>

const char *lunar_feature_name(LunarFeatureId feature) {
    switch (feature) {
    case LUNAR_FEATURE_COSMETICS: return "cosmetics";
    case LUNAR_FEATURE_EMOTES: return "emotes";
    case LUNAR_FEATURE_SPRAYS: return "sprays";
    case LUNAR_FEATURE_JAMS: return "jams";
    case LUNAR_FEATURE_BADGES: return "badges";
    case LUNAR_FEATURE_LUNAR_PLUS: return "lunar_plus";
    default: return "unknown";
    }
}

int lunar_unlock_report_succeeded(const LunarUnlockReport *report) {
    if (report == nullptr || report->feature_count <= 0
            || report->feature_count > LUNAR_FEATURE_COUNT) {
        return 0;
    }
    bool cosmeticsUnlocked = false;
    for (int index = 0; index < report->feature_count; ++index) {
        const LunarFeatureResult& result = report->features[index];
        if (result.feature == LUNAR_FEATURE_COSMETICS) {
            cosmeticsUnlocked = result.state == LUNAR_FEATURE_UNLOCKED;
        }
        if (result.state == LUNAR_FEATURE_FAILED
                || result.state == LUNAR_FEATURE_PENDING) {
            return 0;
        }
        if (result.state == LUNAR_FEATURE_UNLOCKED
                && (result.catalog_count <= 0
                    || result.owned_count != result.catalog_count)) {
            return 0;
        }
    }
    return cosmeticsUnlocked ? 1 : 0;
}

int lunar_unlock_counts_match(int catalog_count, int owned_count,
        int serial_count) {
    return catalog_count > 0 && owned_count == catalog_count
            && serial_count == catalog_count;
}

int lunar_unlock_should_repair_manager(int catalog_count, int owned_count,
        int serial_count) {
    return catalog_count > 0 &&
        (owned_count != catalog_count || serial_count != catalog_count);
}

int lunar_unlock_run(JavaVM *, jvmtiEnv *, JNIEnv *,
        const LunarUnlockSettings *, LunarUnlockReport *) {
    return 0;
}

void lunar_unlock_stop(void) {
}
