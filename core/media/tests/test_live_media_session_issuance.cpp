#include "Database.h"
#include "MediaSessionIssuanceService.h"
#include "MediaSessionRepository.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <string>

namespace
{
class Entropy
{
public:
    bool fill(unsigned char* output, std::size_t size)
    {
        if (output == nullptr) return false;
        for (std::size_t i = 0; i < size; ++i)
            output[i] = static_cast<unsigned char>((counter_++ % 251) + 1);
        return true;
    }
private:
    unsigned int counter_ = 1;
};
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    MediaSessionRepository repository(database);
    assert(repository.ensureSchema());

    Entropy entropy;
    MediaSessionIssuanceService service(
        repository,
        [&entropy](unsigned char* output, std::size_t size) {
            return entropy.fill(output, size);
        },
        [] {
            return std::chrono::system_clock::from_time_t(1'800'000'000);
        });

    MediaSessionIssuanceRequest request;
    request.actorId = "actor_live";
    request.backendId = "default";
    request.resourceKind = "live-channel";
    request.resourceId = "S19.2E-1-1019-10301";
    request.presentationProfileId = "browser-hls-fmp4-copy";
    request.providerId = "suitebridge-native-live";
    request.lifetimeSeconds = 1800;

    auto issued = service.issue(request);
    assert(issued.issued);
    assert(issued.reasonCode.empty());
    assert(issued.session.sessionId.rfind("ms_", 0) == 0);
    assert(issued.session.leaseId.rfind("pl_", 0) == 0);
    assert(!issued.session.accessCredential.empty());

    const auto stored = repository.findSession(issued.session.sessionId);
    assert(stored.has_value());
    assert(stored->backendId == "default");
    assert(stored->resourceKind == "live-channel");
    assert(stored->resourceId == "S19.2E-1-1019-10301");
    assert(stored->presentationProfileId == "browser-hls-fmp4-copy");
    assert(stored->state == "provisioning");

    // Existing Recording issuance remains allowed.
    request.resourceKind = "recording";
    request.resourceId = "rec_1";
    request.providerId = "local-vdr-recording";
    auto recording = service.issue(request);
    assert(recording.issued);

    // Growing Recording and untyped resources are not silently enabled by
    // the Live-TV vertical.
    request.resourceKind = "growing-recording";
    request.resourceId = "grow_1";
    auto growing = service.issue(request);
    assert(!growing.issued);
    assert(growing.reasonCode == "invalid_media_session_request");

    request.resourceKind = "live-channel";
    request.backendId.clear();
    auto missingBackend = service.issue(request);
    assert(!missingBackend.issued);

    return 0;
}
