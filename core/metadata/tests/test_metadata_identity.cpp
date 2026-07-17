#include "MetadataIdentity.h"

#include <cassert>
#include <iostream>
#include <set>
#include <string>

namespace
{

void test_generates_distinct_valid_suite_owned_ids()
{
    std::set<std::string> entityIds;
    std::set<std::string> assignmentIds;
    std::set<std::string> targetIds;

    for (int index = 0; index < 16; ++index)
    {
        const MetadataEntityId entityId = MetadataEntityId::generate();
        const MetadataAssignmentId assignmentId =
            MetadataAssignmentId::generate();
        const MetadataTargetId targetId = MetadataTargetId::generate();

        assert(entityId.isValid());
        assert(assignmentId.isValid());
        assert(targetId.isValid());

        assert(entityId.value().size() == 38);
        assert(assignmentId.value().size() == 38);
        assert(targetId.value().size() == 38);

        assert(entityId.value().compare(0, 6, "mdent_") == 0);
        assert(assignmentId.value().compare(0, 6, "mdasg_") == 0);
        assert(targetId.value().compare(0, 6, "mdtgt_") == 0);

        entityIds.insert(entityId.value());
        assignmentIds.insert(assignmentId.value());
        targetIds.insert(targetId.value());
    }

    assert(entityIds.size() == 16);
    assert(assignmentIds.size() == 16);
    assert(targetIds.size() == 16);
}

void test_rejects_noncanonical_opaque_ids()
{
    const std::string hex(32, 'a');

    assert(MetadataEntityId::isValidValue("mdent_" + hex));
    assert(MetadataAssignmentId::isValidValue("mdasg_" + hex));
    assert(MetadataTargetId::isValidValue("mdtgt_" + hex));

    assert(!MetadataEntityId::isValidValue("mdasg_" + hex));
    assert(!MetadataEntityId::isValidValue("mdent_" + hex.substr(1)));
    assert(!MetadataEntityId::isValidValue("mdent_" + hex + "0"));
    assert(!MetadataEntityId::isValidValue(
        "mdent_A" + hex.substr(1)));
    assert(!MetadataEntityId::isValidValue(
        "mdent_/" + hex.substr(1)));
    assert(!MetadataEntityId::isValidValue("movies/13/poster.jpg"));

    const MetadataEntityId invalidEntity("13");
    assert(!invalidEntity.isValid());
    assert(!invalidEntity.empty());

    const MetadataAssignmentId emptyAssignment;
    assert(emptyAssignment.empty());
    assert(!emptyAssignment.isValid());
}

void test_provider_ids_are_stable_slugs_not_external_urls()
{
    const MetadataProviderId tvscraper(
        "restfulapi-scraper-bridge");
    const MetadataProviderId sidecar("sidecar.local");
    const MetadataProviderId manual("manual");

    assert(tvscraper.isValid());
    assert(sidecar.isValid());
    assert(manual.isValid());

    assert(!MetadataProviderId("TVScraper").isValid());
    assert(!MetadataProviderId("provider name").isValid());
    assert(!MetadataProviderId("provider/path").isValid());
    assert(!MetadataProviderId("https://provider.invalid").isValid());
    assert(!MetadataProviderId("-provider").isValid());
    assert(!MetadataProviderId("provider-").isValid());
    assert(!MetadataProviderId(std::string(65, 'a')).isValid());
}

void test_target_refs_require_suite_owned_target_identity()
{
    const MetadataTargetId targetId = MetadataTargetId::generate();

    MetadataTargetRef recording;
    recording.type = MetadataTargetType::Recording;
    recording.targetId = targetId;

    assert(recording.isValid());
    assert(recording.canonicalKey() ==
           "recording:" + targetId.value());
    assert(recording.canonicalKey().find("/srv/vdr/video") ==
           std::string::npos);

    MetadataTargetRef programEvent;
    programEvent.type = MetadataTargetType::ProgramEvent;
    programEvent.targetId = targetId;

    assert(programEvent.isValid());
    assert(programEvent.canonicalKey() ==
           "program-event:" + targetId.value());
    assert(programEvent != recording);

    MetadataTargetRef unknown;
    unknown.type = MetadataTargetType::Unknown;
    unknown.targetId = targetId;
    assert(!unknown.isValid());
    assert(unknown.canonicalKey().empty());

    MetadataTargetRef backendNativeRecording;
    backendNativeRecording.type = MetadataTargetType::Recording;
    backendNativeRecording.targetId = MetadataTargetId("7");
    assert(!backendNativeRecording.isValid());
    assert(backendNativeRecording.canonicalKey().empty());
}

void test_names_are_provider_neutral_and_deterministic()
{
    assert(std::string(metadataMediaTypeName(
               MetadataMediaType::Unknown)) == "unknown");
    assert(std::string(metadataMediaTypeName(
               MetadataMediaType::Movie)) == "movie");
    assert(std::string(metadataMediaTypeName(
               MetadataMediaType::Series)) == "series");
    assert(std::string(metadataMediaTypeName(
               MetadataMediaType::Season)) == "season");
    assert(std::string(metadataMediaTypeName(
               MetadataMediaType::Episode)) == "episode");
    assert(std::string(metadataMediaTypeName(
               MetadataMediaType::Programme)) == "programme");
    assert(std::string(metadataMediaTypeName(
               MetadataMediaType::Person)) == "person");

    assert(std::string(metadataTargetTypeName(
               MetadataTargetType::Recording)) == "recording");
    assert(std::string(metadataTargetTypeName(
               MetadataTargetType::ProgramEvent)) ==
           "program-event");
    assert(std::string(metadataTargetTypeName(
               MetadataTargetType::TimerIntent)) ==
           "timer-intent");
}

}

int main()
{
    test_generates_distinct_valid_suite_owned_ids();
    test_rejects_noncanonical_opaque_ids();
    test_provider_ids_are_stable_slugs_not_external_urls();
    test_target_refs_require_suite_owned_target_identity();
    test_names_are_provider_neutral_and_deterministic();

    std::cout << "test_metadata_identity passed" << std::endl;
    return 0;
}
