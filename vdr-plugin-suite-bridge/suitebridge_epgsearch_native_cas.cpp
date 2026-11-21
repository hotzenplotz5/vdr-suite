#include "suitebridge_epgsearch_native_cas.h"
#include <vdr/plugin.h>
#include <algorithm>

namespace {
const char *kObserve = "Epgsearch-searchtimer-observe-v1.1";
const char *kUpdate = "Epgsearch-searchtimer-update-cas-v1.0";
const char *kToggle = "Epgsearch-searchtimer-toggle-cas-v1.0";
const char *kDelete = "Epgsearch-searchtimer-delete-cas-v1.0";

cPlugin *nativePlugin() { return cPluginManager::GetPlugin("epgsearch"); }

bool validToken(const std::string &value, const char *prefix) {
    if (value.size() != 35U || value.compare(0, 3, prefix) != 0) return false;
    return std::all_of(value.begin() + 3, value.end(), [](unsigned char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
}
bool validRequest(const SuiteBridgeSearchCasRequest &r) {
    if (r.nativeId < 0 || !validToken(r.generation, "sg_") ||
        !validToken(r.revision, "sr_")) return false;
    if (r.action != SuiteBridgeSearchCasAction::Update) return r.settings.empty();
    return !r.settings.empty() && r.settings.size() <= 8192U &&
        r.settings.find(char(0)) == std::string::npos &&
        r.settings.find(char(10)) == std::string::npos &&
        r.settings.find(char(13)) == std::string::npos;
}
SuiteBridgeSearchCasResult negative(SuiteBridgeSearchCasStatus status, bool proof) {
    SuiteBridgeSearchCasResult out;
    out.status = status;
    out.atomicCompareAndWrite = proof;
    out.noWriteAttested = proof;
    return out;
}
SuiteBridgeSearchCasResult unknown() {
    return negative(SuiteBridgeSearchCasStatus::OutcomeUnknown, false);
}
SuiteBridgeSearchCasResult conclude(const SuiteBridgeSearchCasRequest &req,
        SuiteBridgeSearchCasStatus status, bool atomic, bool noWrite,
        const std::string &gen, const std::string &rev,
        const SuiteBridgeEpgsearchNativeCas &bridge) {
    if (status != SuiteBridgeSearchCasStatus::AppliedVerified) {
        if (status == SuiteBridgeSearchCasStatus::OutcomeUnknown ||
            !atomic || !noWrite || !gen.empty() || !rev.empty()) return unknown();
        return negative(status, true);
    }
    if (!atomic || noWrite || gen != req.generation) return unknown();
    if (req.action == SuiteBridgeSearchCasAction::Delete) {
        if (!rev.empty()) return unknown();
    } else if (!validToken(rev, "sr_") || rev == req.revision) {
        return unknown();
    }
    // Native acknowledgement is never sufficient without an independent
    // exact-generation and exact-revision epgsearch-locked observation.
    const auto seen = bridge.Observe(req.nativeId);
    if (!seen.available) return unknown();
    if (req.action == SuiteBridgeSearchCasAction::Delete) {
        if (seen.found && (!seen.durable || seen.generation == gen)) return unknown();
    } else if (!seen.found || !seen.durable ||
               seen.generation != gen || seen.revision != rev) return unknown();
    SuiteBridgeSearchCasResult answer;
    answer.status = SuiteBridgeSearchCasStatus::AppliedVerified;
    answer.atomicCompareAndWrite = true;
    answer.generation = gen;
    answer.revision = rev;
    return answer;
}
} // namespace

bool SuiteBridgeEpgsearchNativeCas::Available() const {
    cPlugin *p = nativePlugin();
    return p && p->Service(kObserve, nullptr) && p->Service(kUpdate, nullptr) &&
        p->Service(kToggle, nullptr) && p->Service(kDelete, nullptr);
}

SuiteBridgeSearchCasObservation SuiteBridgeEpgsearchNativeCas::Observe(int id) const {
    SuiteBridgeSearchCasObservation result;
    cPlugin *p = nativePlugin();
    if (id < 0 || !p || !p->Service(kObserve, nullptr)) return result;
    Epgsearch_searchtimer_observe_v1_1 req{};
    req.nativeId = id;
    if (!p->Service(kObserve, &req)) return result;
    result.available = true;
    result.found = req.found;
    if (req.found && req.generationPersisted && req.revisionPersisted &&
        validToken(req.generation, "sg_") && validToken(req.revision, "sr_")) {
        result.durable = true;
        result.generation = req.generation;
        result.revision = req.revision;
        result.settings = req.settings;
    }
    return result;
}

SuiteBridgeSearchCasResult SuiteBridgeEpgsearchNativeCas::Execute(
        const SuiteBridgeSearchCasRequest &req) const {
    if (!validRequest(req)) return negative(SuiteBridgeSearchCasStatus::Invalid, false);
    cPlugin *p = nativePlugin();
    if (!p || !Available()) return negative(SuiteBridgeSearchCasStatus::Unavailable, false);
    if (req.action == SuiteBridgeSearchCasAction::Update) {
        Epgsearch_searchtimer_update_cas_v1_0 value{};
        value.nativeId = req.nativeId;
        value.expectedGeneration = req.generation;
        value.expectedRevision = req.revision;
        value.settings = req.settings;
        if (!p->Service(kUpdate, &value)) return unknown();
        auto status = SuiteBridgeSearchCasStatus::OutcomeUnknown;
        switch (value.status) {
            case enucsApplied: status = SuiteBridgeSearchCasStatus::AppliedVerified; break;
            case enucsInvalid: status = SuiteBridgeSearchCasStatus::Invalid; break;
            case enucsNotFound: status = SuiteBridgeSearchCasStatus::NotFound; break;
            case enucsStaleGeneration: status = SuiteBridgeSearchCasStatus::StaleGeneration; break;
            case enucsStaleRevision: status = SuiteBridgeSearchCasStatus::StaleRevision; break;
            case enucsNotInactive: status = SuiteBridgeSearchCasStatus::NotInactive; break;
            case enucsUnpersistedChange: status = SuiteBridgeSearchCasStatus::UnpersistedChange; break;
            case enucsUnavailable: status = SuiteBridgeSearchCasStatus::Unavailable; break;
            case enucsOutcomeUnknown: break;
        }
        return conclude(req, status, value.atomicCompareAndWrite,
            value.noWriteAttested, value.confirmedGeneration, value.resultingRevision, *this);
    }
    if (req.action == SuiteBridgeSearchCasAction::Delete) {
        Epgsearch_searchtimer_delete_cas_v1_0 value{};
        value.nativeId = req.nativeId;
        value.expectedGeneration = req.generation;
        value.expectedRevision = req.revision;
        if (!p->Service(kDelete, &value)) return unknown();
        auto status = SuiteBridgeSearchCasStatus::OutcomeUnknown;
        switch (value.status) {
            case endcsApplied: status = SuiteBridgeSearchCasStatus::AppliedVerified; break;
            case endcsNotFound: status = SuiteBridgeSearchCasStatus::NotFound; break;
            case endcsStaleGeneration: status = SuiteBridgeSearchCasStatus::StaleGeneration; break;
            case endcsStaleRevision: status = SuiteBridgeSearchCasStatus::StaleRevision; break;
            case endcsUnpersistedChange: status = SuiteBridgeSearchCasStatus::UnpersistedChange; break;
            case endcsUnavailable: status = SuiteBridgeSearchCasStatus::Unavailable; break;
            case endcsOutcomeUnknown: break;
        }
        return conclude(req, status, value.atomicCompareAndWrite,
            value.noWriteAttested, value.confirmedDeletedGeneration, {}, *this);
    }
    Epgsearch_searchtimer_toggle_cas_v1_0 value{};
    value.nativeId = req.nativeId;
    value.expectedGeneration = req.generation;
    value.expectedRevision = req.revision;
    value.activate = req.action == SuiteBridgeSearchCasAction::Activate;
    if (!p->Service(kToggle, &value)) return unknown();
    auto status = SuiteBridgeSearchCasStatus::OutcomeUnknown;
    switch (value.status) {
        case entcsApplied: status = SuiteBridgeSearchCasStatus::AppliedVerified; break;
        case entcsNotFound: status = SuiteBridgeSearchCasStatus::NotFound; break;
        case entcsStaleGeneration: status = SuiteBridgeSearchCasStatus::StaleGeneration; break;
        case entcsStaleRevision: status = SuiteBridgeSearchCasStatus::StaleRevision; break;
        case entcsUnpersistedChange: status = SuiteBridgeSearchCasStatus::UnpersistedChange; break;
        case entcsUnavailable: status = SuiteBridgeSearchCasStatus::Unavailable; break;
        case entcsOutcomeUnknown: break;
    }
    return conclude(req, status, value.atomicCompareAndWrite,
        value.noWriteAttested, value.confirmedGeneration, value.resultingRevision, *this);
}
