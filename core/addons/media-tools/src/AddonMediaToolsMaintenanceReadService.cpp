#include "AddonMediaToolsMaintenanceReadService.h"

#include <vector>

namespace vdrsuite::addons {

MaintenanceReadResult AddonMediaToolsMaintenanceReadService::inspect(
    const RequestSecurityContext& actor,
    const std::string& backendId) const
{
    if (actor.actor.type != ActorType::User ||
        !canInspectInventory(actor).allowed)
        return {false, MaintenanceReadReason::forbidden, 0, 0};

    // The canonical repository validates the backend identifier, checks the
    // resolved administrator context, and fails closed on missing schema.
    const auto intent = intents_.read(actor, "rectools", backendId);
    if (intent.status == IntentStatus::forbidden ||
        intent.status == IntentStatus::invalid)
        return {false, MaintenanceReadReason::forbidden, 0, 0};
    if (intent.status != IntentStatus::ok)
        return {false, MaintenanceReadReason::activationUnavailable, 0, 0};
    if (intent.intent.desiredEnabled || intent.intent.revision <= 0)
        return {false, MaintenanceReadReason::activationNotExplicitlyDisabled,
                intent.intent.revision, 0};

    using State = vdrsuite::operations::MutationOperationState;
    using Status = vdrsuite::operations::MutationOperationRepositoryStatus;
    // outcomeUnknown is nonterminal: a previous import may have written a
    // recording even though its authoritative readback is still unresolved.
    const std::vector<State> unresolved = {
        State::accepted, State::queued, State::dispatching,
        State::executedUnverified, State::outcomeUnknown,
    };
    // The list is global by action family. At the maximum size, fail
    // closed: a matching backend's operation could be beyond the page.
    constexpr std::size_t kMaximumScan = 256;
    const auto result = operations_.listByActionFamilyAndStates(
        "media.import", unresolved, kMaximumScan);
    if (result.status != Status::ok)
        return {false, MaintenanceReadReason::operationsUnavailable,
                intent.intent.revision, 0};
    if (result.operations.size() >= kMaximumScan)
        return {false, MaintenanceReadReason::scanSaturated,
                intent.intent.revision, result.operations.size()};

    std::size_t matching = 0;
    for (const auto& operation : result.operations)
        if (operation.backendId == backendId) ++matching;
    if (matching != 0)
        return {false, MaintenanceReadReason::unresolvedMediaImports,
                intent.intent.revision, matching};

    return {true, MaintenanceReadReason::locallyQuiescentSnapshot,
            intent.intent.revision, 0};
}

} // namespace vdrsuite::addons
