#include "Database.h"
#include "IVdrRecordingNativeMetadataResolver.h"
#include "VdrRecordingNativeIdentity.h"
#include "VdrRecordingNativeMetadataEnrichmentService.h"

#include <cassert>
#include <cstdio>
#include <deque>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace
{
class MockResolver final : public IVdrRecordingNativeMetadataResolver
{
public:
    std::map<std::string, std::deque<VdrRecordingNativeMetadata>> replies;
    std::map<std::string, int> calls;

    VdrRecordingNativeMetadata resolve(const std::string& recordingKey) override
    {
        ++calls[recordingKey];
        auto iterator = replies.find(recordingKey);
        if (iterator == replies.end() || iterator->second.empty())
        {
            VdrRecordingNativeMetadata metadata;
            metadata.availability = VdrRecordingNativeMetadataAvailability::TransportError;
            metadata.recordingKey = recordingKey;
            metadata.diagnostic = "missing mock reply";
            return metadata;
        }
        VdrRecordingNativeMetadata metadata = iterator->second.front();
        if (iterator->second.size() > 1)
        {
            iterator->second.pop_front();
        }
        return metadata;
    }
};

VdrRecording recording(const std::string& nativeId, const std::string& title)
{
    VdrRecording value;
    value.backendNativeId = nativeId;
    value.title = title;
    return value;
}

VdrRecordingNativeMetadata found(const std::string& key, const std::string& title)
{
    VdrRecordingNativeMetadata metadata;
    metadata.availability = VdrRecordingNativeMetadataAvailability::Found;
    metadata.schema = 1;
    metadata.recordingIdentitySchema = 1;
    metadata.recordingKey = key;
    metadata.found = true;
    metadata.reason = "none";
    metadata.provider = "tvscraper";
    metadata.mediaType = "movie";
    metadata.providerId = 13;
    metadata.title = title;
    metadata.preferredArtwork.provider = "none";
    VdrRecordingNativePerson person;
    person.role = "actor";
    person.name = "Tom Hanks";
    person.characterName = "Forrest Gump";
    person.image.provider = "none";
    metadata.people.push_back(person);
    return metadata;
}

VdrRecordingNativeMetadata notFound(const std::string& key)
{
    VdrRecordingNativeMetadata metadata;
    metadata.availability = VdrRecordingNativeMetadataAvailability::NotFound;
    metadata.schema = 1;
    metadata.recordingIdentitySchema = 1;
    metadata.recordingKey = key;
    metadata.found = false;
    metadata.reason = "provider-no-match";
    metadata.provider = "none";
    metadata.mediaType = "none";
    metadata.preferredArtwork.provider = "none";
    return metadata;
}

VdrRecordingNativeMetadata failure(
    const std::string& key,
    VdrRecordingNativeMetadataAvailability availability,
    const std::string& diagnostic)
{
    VdrRecordingNativeMetadata metadata;
    metadata.availability = availability;
    metadata.recordingKey = key;
    metadata.diagnostic = diagnostic;
    return metadata;
}
}

int main()
{
    const char* databasePath = "/tmp/test_vdr_recording_native_metadata_enrichment.db";
    std::remove(databasePath);

    Database database;
    assert(database.open(databasePath));
    VdrRecordingNativeMetadataRepository repository(database);
    assert(repository.ensureSchema());

    MockResolver resolver;
    VdrRecordingNativeMetadataEnrichmentConfig config;
    config.foundTtlSeconds = 100;
    config.negativeTtlSeconds = 20;
    config.retryInitialSeconds = 10;
    config.retryMaximumSeconds = 40;
    config.maximumRetryCount = 3;
    config.maximumQueuedRecordings = 2;
    config.maximumBatchSize = 2;

    VdrRecordingNativeMetadataEnrichmentService service(
        "default",
        repository,
        resolver,
        config);

    const std::string forrestNative =
        "/srv/vdr/video/Forrest_Gump/2026-07-20.20.15.1-0.rec";
    const std::string unknownNative =
        "/srv/vdr/video/Unknown/2026-07-20.22.15.1-0.rec";
    const std::string thirdNative =
        "/srv/vdr/video/Third/2026-07-21.00.15.1-0.rec";
    const std::string forrestKey =
        VdrRecordingNativeIdentity::keyForNativeId(forrestNative);
    const std::string unknownKey =
        VdrRecordingNativeIdentity::keyForNativeId(unknownNative);
    const std::string thirdKey =
        VdrRecordingNativeIdentity::keyForNativeId(thirdNative);

    resolver.replies[forrestKey].push_back(found(forrestKey, "Forrest Gump"));
    resolver.replies[unknownKey].push_back(notFound(unknownKey));
    resolver.replies[thirdKey].push_back(found(thirdKey, "Third"));

    const std::vector<VdrRecording> initial = {
        recording(forrestNative, "Forrest Gump"),
        recording(forrestNative, "Forrest Gump Alias"),
        recording(unknownNative, "Unknown"),
        recording(thirdNative, "Third")
    };

    assert(service.reconcileInventory(initial, 1000) == 2);
    assert(service.status().queuedCount == 2);
    assert(service.processBatch(1000, 10) == 2);
    assert(resolver.calls[forrestKey] + resolver.calls[unknownKey] + resolver.calls[thirdKey] == 2);
    assert(service.status().queuedCount == 0);

    // The queue bound delayed one recording; the next reconciliation picks it up.
    assert(service.reconcileInventory(initial, 1001) == 1);
    assert(service.processBatch(1001) == 1);
    assert(repository.find("default", forrestKey).metadata.title == "Forrest Gump");
    { auto x = repository.searchPeople("default", {"Tom Hanks", "", "", "actor", 20, 0}); assert(x.totalCount == 2); }

    // Found and negative entries stay quiet until their separate TTL expires.
    assert(service.reconcileInventory(initial, 1010) == 0);
    assert(service.reconcileInventory(initial, 1021) == 1);
    service.clearQueue();
    assert(service.status().queuedCount == 0);

    // A move removes the old key and schedules the new native identity.
    const std::string movedNative =
        "/srv/vdr/video/Movies/Forrest_Gump/2026-07-20.20.15.1-0.rec";
    const std::string movedKey =
        VdrRecordingNativeIdentity::keyForNativeId(movedNative);
    resolver.replies[movedKey].push_back(found(movedKey, "Forrest Gump"));
    const std::vector<VdrRecording> movedInventory = {
        recording(movedNative, "Forrest Gump"),
        recording(unknownNative, "Unknown"),
        recording(thirdNative, "Third")
    };
    assert(service.reconcileInventory(movedInventory, 1030) >= 1);
    assert(!repository.find("default", forrestKey).exists());
    service.clearQueue();

    // Retry is exponential, bounded and stops after the configured attempt count.
    const std::string retryNative =
        "/srv/vdr/video/Retry/2026-07-21.01.15.1-0.rec";
    const std::string retryKey =
        VdrRecordingNativeIdentity::keyForNativeId(retryNative);
    resolver.replies[retryKey].push_back(failure(
        retryKey,
        VdrRecordingNativeMetadataAvailability::TransportError,
        "transport one"));
    resolver.replies[retryKey].push_back(failure(
        retryKey,
        VdrRecordingNativeMetadataAvailability::ProviderUnavailable,
        "provider two"));
    resolver.replies[retryKey].push_back(found(retryKey, "Retry Success"));
    const std::vector<VdrRecording> retryInventory = {
        recording(retryNative, "Retry")
    };

    assert(service.reconcileInventory(retryInventory, 2000) == 1);
    assert(service.processBatch(2000) == 1);
    auto retryRecord = repository.find("default", retryKey);
    assert(retryRecord.retryCount == 1);
    assert(retryRecord.nextRetryAt == 2010);
    assert(service.reconcileInventory(retryInventory, 2009) == 0);
    assert(service.reconcileInventory(retryInventory, 2010) == 1);
    assert(service.processBatch(2010) == 1);
    retryRecord = repository.find("default", retryKey);
    assert(retryRecord.retryCount == 2);
    assert(retryRecord.nextRetryAt == 2030);
    assert(service.reconcileInventory(retryInventory, 2030) == 1);
    assert(service.processBatch(2030) == 1);
    retryRecord = repository.find("default", retryKey);
    assert(retryRecord.contentState == "found");
    assert(retryRecord.retryCount == 0);
    assert(retryRecord.metadata.title == "Retry Success");

    const std::string exhaustedNative =
        "/srv/vdr/video/Exhausted/2026-07-21.02.15.1-0.rec";
    const std::string exhaustedKey =
        VdrRecordingNativeIdentity::keyForNativeId(exhaustedNative);
    resolver.replies[exhaustedKey].push_back(failure(
        exhaustedKey,
        VdrRecordingNativeMetadataAvailability::TransportError,
        "always failing"));
    const std::vector<VdrRecording> exhaustedInventory = {
        recording(exhaustedNative, "Exhausted")
    };
    assert(service.reconcileInventory(exhaustedInventory, 3000) == 1);
    assert(service.processBatch(3000) == 1);
    assert(service.reconcileInventory(exhaustedInventory, 3010) == 1);
    assert(service.processBatch(3010) == 1);
    assert(service.reconcileInventory(exhaustedInventory, 3030) == 1);
    assert(service.processBatch(3030) == 1);
    const auto exhausted = repository.find("default", exhaustedKey);
    assert(exhausted.retryCount == 3);
    assert(exhausted.nextRetryAt == 0);
    assert(service.reconcileInventory(exhaustedInventory, 9999) == 0);
    assert(service.status().exhaustedRecordings >= 1);

    // Invalid identities are never sent to SuiteBridge.
    const int callsBeforeInvalid = resolver.calls[""];
    assert(service.reconcileInventory({recording("bad\npath", "Invalid")}, 4000) == 0);
    assert(resolver.calls[""] == callsBeforeInvalid);
    assert(service.status().invalidRecordings >= 1);

    std::remove(databasePath);
    std::cout << "test_vdr_recording_native_metadata_enrichment_service passed\n";
    return 0;
}
