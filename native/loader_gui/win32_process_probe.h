#pragma once

#include "auto_inject_readiness.h"

#include <cstdint>
#include <optional>
#include <vector>

bool isValidProcessIdentity(const ProcessIdentity& identity) noexcept;
bool sameProcessIdentity(const ProcessIdentity& left,
    const ProcessIdentity& right) noexcept;

struct ProcessIdentityLess {
    bool operator()(const ProcessIdentity& left,
        const ProcessIdentity& right) const noexcept;
};

bool isAutoInjectCandidate(const ProcessObservation& observation,
    const AutoInjectPolicy& policy);
std::optional<ProcessIdentity> selectNewestAutoInjectCandidate(
    const std::vector<ProcessObservation>& observations,
    const AutoInjectPolicy& policy);

bool queryProcessIdentity(std::uint32_t processId,
    ProcessIdentity& identity) noexcept;
bool openedProcessMatchesIdentity(void* nativeProcessHandle,
    const ProcessIdentity& expected) noexcept;

class ProcessProbe {
public:
    virtual ~ProcessProbe() = default;
    virtual std::vector<ProcessObservation> probe() = 0;
    virtual bool isCurrent(const ProcessIdentity& identity) const = 0;
};

class Win32ProcessProbe final : public ProcessProbe {
public:
    std::vector<ProcessObservation> probe() override;
    bool isCurrent(const ProcessIdentity& identity) const override;
};
