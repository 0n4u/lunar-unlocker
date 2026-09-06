#ifndef LUNARUNLOCKER_LUNAR_UNLOCK_ENGINE_H
#define LUNARUNLOCKER_LUNAR_UNLOCK_ENGINE_H

#include <jni.h>
#include <jvmti.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum LunarFeatureId {
    LUNAR_FEATURE_COSMETICS = 0,
    LUNAR_FEATURE_EMOTES = 1,
    LUNAR_FEATURE_SPRAYS = 2,
    LUNAR_FEATURE_JAMS = 3,
    LUNAR_FEATURE_BADGES = 4,
    LUNAR_FEATURE_LUNAR_PLUS = 5,
    LUNAR_FEATURE_COUNT = 6
} LunarFeatureId;

typedef enum LunarFeatureState {
    LUNAR_FEATURE_PENDING = 0,
    LUNAR_FEATURE_UNLOCKED = 1,
    LUNAR_FEATURE_UNAVAILABLE = 2,
    LUNAR_FEATURE_FAILED = 3
} LunarFeatureState;

typedef struct LunarFeatureResult {
    LunarFeatureId feature;
    LunarFeatureState state;
    int catalog_count;
    int owned_count;
    char detail[160];
} LunarFeatureResult;

typedef struct LunarUnlockSettings {
    int cosmetics;
    int emotes;
    int sprays;
    int jams;
    int badges;
    int lunar_plus;
} LunarUnlockSettings;

typedef struct LunarUnlockReport {
    LunarFeatureResult features[LUNAR_FEATURE_COUNT];
    int feature_count;
    int used_legacy_fallback;
} LunarUnlockReport;

const char *lunar_feature_name(LunarFeatureId feature);
int lunar_unlock_report_succeeded(const LunarUnlockReport *report);
int lunar_unlock_counts_match(int catalog_count, int owned_count,
        int serial_count);
int lunar_unlock_should_repair_manager(int catalog_count, int owned_count,
        int serial_count);
int lunar_unlock_run(JavaVM *vm, jvmtiEnv *jvmti, JNIEnv *env,
        const LunarUnlockSettings *settings, LunarUnlockReport *report);
void lunar_unlock_stop(void);

#ifdef __cplusplus
}
#endif

#endif
