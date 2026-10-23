#include "suitebridge_epgsearch_native_cas_control.h"

#include <algorithm>
#include <climits>
#include <cstdio>
#include <sstream>
#include <vector>

namespace {
using vdrsuite::agent::control::Operation;

bool decimalId(const std::string& token, int& value) {
    if (token.empty() || token.size() > 10U ||
        (token.size() > 1U && token.front() == '0')) return false;
    unsigned long long parsed = 0;
    for (char ch : token) {
        if (ch < '0' || ch > '9') return false;
        parsed = parsed * 10U + static_cast<unsigned>(ch - '0');
        if (parsed > INT_MAX) return false;
    }
    value = static_cast<int>(parsed);
    return true;
}
bool lowerHexToken(const std::string& value, const char* prefix,
                   std::size_t hexDigits) {
    const std::string p(prefix);
    if (value.size() != p.size() + hexDigits ||
        value.compare(0, p.size(), p) != 0) return false;
    return std::all_of(value.begin() + p.size(), value.end(),
        [](unsigned char ch) {
            return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f');
        });
}
char hexDigit(unsigned value) {
    return static_cast<char>(value < 10 ? '0' + value : 'a' + value - 10);
}
std::string hexEncode(const std::string& input) {
    std::string out;
    out.reserve(input.size() * 2);
    for (unsigned char ch : input) {
        out.push_back(hexDigit(ch >> 4));
        out.push_back(hexDigit(ch & 15));
    }
    return out.empty() ? "-" : out;
}
std::vector<std::string> split(const std::string& payload) {
    std::vector<std::string> result;
    std::size_t start = 0;
    while (start <= payload.size()) {
        const std::size_t next = payload.find('\n', start);
        result.push_back(payload.substr(start,
            next == std::string::npos ? std::string::npos : next - start));
        if (next == std::string::npos) break;
        start = next + 1;
        if (result.size() > 8U) break;
    }
    return result;
}
SuiteBridgeCommandResult reply(int code, const std::string& payload) {
    return {true, code, payload};
}
const char* resultName(SuiteBridgeSearchCasStatus status) {
    switch (status) {
        case SuiteBridgeSearchCasStatus::Unavailable: return "unavailable";
        case SuiteBridgeSearchCasStatus::Invalid: return "invalid";
        case SuiteBridgeSearchCasStatus::NotFound: return "not_found";
        case SuiteBridgeSearchCasStatus::StaleGeneration: return "stale_generation";
        case SuiteBridgeSearchCasStatus::StaleRevision: return "stale_revision";
        case SuiteBridgeSearchCasStatus::NotInactive: return "not_inactive";
        case SuiteBridgeSearchCasStatus::UnpersistedChange: return "unpersisted_change";
        case SuiteBridgeSearchCasStatus::AppliedVerified: return "applied_verified";
        case SuiteBridgeSearchCasStatus::OutcomeUnknown: return "outcome_unknown";
    }
    return "outcome_unknown";
}
const char* name(SuiteBridgeSearchCasAction action) {
    switch (action) {
        case SuiteBridgeSearchCasAction::Update: return "update";
        case SuiteBridgeSearchCasAction::Activate: return "activate";
        case SuiteBridgeSearchCasAction::Deactivate: return "deactivate";
        case SuiteBridgeSearchCasAction::Delete: return "delete";
    }
    return "unknown";
}
int code(SuiteBridgeSearchCasStatus status) {
    switch (status) {
        case SuiteBridgeSearchCasStatus::AppliedVerified: return 900;
        case SuiteBridgeSearchCasStatus::StaleGeneration:
        case SuiteBridgeSearchCasStatus::StaleRevision:
        case SuiteBridgeSearchCasStatus::NotFound:
        case SuiteBridgeSearchCasStatus::NotInactive:
        case SuiteBridgeSearchCasStatus::UnpersistedChange: return 555;
        case SuiteBridgeSearchCasStatus::Unavailable: return 556;
        case SuiteBridgeSearchCasStatus::Invalid: return 501;
        case SuiteBridgeSearchCasStatus::OutcomeUnknown: return 558;
    }
    return 558;
}
} // namespace

SuiteBridgeCommandResult SuiteBridgeEpgsearchCasControl::Handle(
    Operation operation, const std::string& payload) const {
    if (operation == Operation::SearchTimerCasCapability) {
        if (!payload.empty()) return reply(501, "s1cas-rejected/1 invalid_capability");
        return reply(900, std::string("s1cas-cap/1 ") +
            (native_.Available() ? "enabled" : "disabled"));
    }

    if (operation == Operation::SearchTimerCasObserve) {
        int id = -1;
        if (!decimalId(payload, id))
            return reply(501, "s1cas-rejected/1 invalid_observe");
        const auto observed = native_.Observe(id);
        if (!observed.available)
            return reply(556, "s1cas-rejected/1 provider_unavailable");
        // Native settings are sensitive. This answer only travels over the
        // existing SO_PEERCRED-protected local control-plane, never SVDRP.
        std::ostringstream output;
        output << "s1cas-observe/1 " << payload << ' '
            << (observed.found ? "1" : "0") << ' '
            << (observed.durable ? "1" : "0") << ' '
            << (observed.durable ? observed.generation : "-") << ' '
            << (observed.durable ? observed.revision : "-") << ' '
            << (observed.durable ? hexEncode(observed.settings) : "-");
        return reply(900, output.str());
    }

    if (operation != Operation::SearchTimerCasExecute)
        return {};

    // Only the credential-checked local agent socket is admitted here.
    // This is a one-shot native call: no alternate writer and no retry.
    // The server must already have claimed the immutable actor-scoped
    // operation with the exact canonical evidence/fingerprint.
    const auto fields = split(payload);
    if (fields.size() != 8U || fields[0] != "1" ||
        !lowerHexToken(fields[2], "op_", 32) ||
        !lowerHexToken(fields[3], "sha256:", 64))
        return reply(501, "s1cas-rejected/1 invalid_operation");

    SuiteBridgeSearchCasRequest req;
    if (fields[1] == "update") req.action = SuiteBridgeSearchCasAction::Update;
    else if (fields[1] == "activate") req.action = SuiteBridgeSearchCasAction::Activate;
    else if (fields[1] == "deactivate") req.action = SuiteBridgeSearchCasAction::Deactivate;
    else if (fields[1] == "delete") req.action = SuiteBridgeSearchCasAction::Delete;
    else return reply(501, "s1cas-rejected/1 invalid_action");
    if (!decimalId(fields[4], req.nativeId) ||
        !lowerHexToken(fields[5], "sg_", 32) ||
        !lowerHexToken(fields[6], "sr_", 32))
        return reply(501, "s1cas-rejected/1 invalid_identity");
    req.generation = fields[5];
    req.revision = fields[6];
    req.settings = fields[7] == "-" ? "" : fields[7];
    if (req.settings.size() > 8192U ||
        (req.action != SuiteBridgeSearchCasAction::Update && !req.settings.empty()))
        return reply(501, "s1cas-rejected/1 invalid_settings");

    const SuiteBridgeSearchCasResult result = native_.Execute(req);
    std::ostringstream output;
    output << "s1cas-result/1 " << fields[2] << ' ' << name(req.action)
        << ' ' << resultName(result.status) << ' '
        << (result.atomicCompareAndWrite ? "1" : "0") << ' '
        << (result.noWriteAttested ? "1" : "0") << ' '
        << (result.generation.empty() ? "-" : result.generation) << ' '
        << (result.revision.empty() ? "-" : result.revision);
    return reply(code(result.status), output.str());
}
