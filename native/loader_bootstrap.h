#ifndef LUNARUNLOCKER_LOADER_BOOTSTRAP_H
#define LUNARUNLOCKER_LOADER_BOOTSTRAP_H

#include <stdint.h>

#define LUNARUNLOCKER_BOOTSTRAP_MAGIC 0x54423456u
#define LUNARUNLOCKER_BOOTSTRAP_VERSION 2u
#define LUNARUNLOCKER_BOOTSTRAP_MODE_ONLINE 1u
#define LUNARUNLOCKER_BOOTSTRAP_STATUS_CREATED 1u
#define LUNARUNLOCKER_BOOTSTRAP_STATUS_CONSUMED 2u
#define LUNARUNLOCKER_BOOTSTRAP_STATUS_FAILED 3u

#pragma pack(push, 1)
typedef struct LunarUnlockerBootstrapV2 {
    uint32_t magic;
    uint16_t version;
    uint16_t structure_size;
    uint32_t target_pid;
    uint32_t mode;
    uint16_t controller_port;
    uint16_t reserved0;
    char service_http_base[256];
    char service_zeus_host[128];
    uint16_t service_zeus_port;
    uint8_t reserved[14];
    uint32_t status;
} LunarUnlockerBootstrapV2;
#pragma pack(pop)

typedef char LunarUnlockerBootstrapV2_size_must_be_424[
        sizeof(LunarUnlockerBootstrapV2) == 424 ? 1 : -1];

int lunarunlocker_loader_bootstrap_initialize(void);
const char *lunarunlocker_loader_access_token(void);
int lunarunlocker_loader_bootstrap_failed(void);
void lunarunlocker_loader_report_progress(int step);
void lunarunlocker_loader_report_completed(void);
void lunarunlocker_loader_report_failure(const char *message);
void lunarunlocker_loader_bootstrap_clear(void);

#endif
