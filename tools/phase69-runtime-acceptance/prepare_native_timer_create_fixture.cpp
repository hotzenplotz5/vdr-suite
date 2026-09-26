#include "Database.h"
#include "TimerAssignmentSchedulingService.h"
#include "TimerIntentRepository.h"

#include <cstdint>
#include <ctime>
#include <iostream>
#include <limits>
#include <string>

using namespace vdrsuite::timers;

namespace
{

struct FixtureOptions
{
    std::string databasePath;
    std::string fixtureId;
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::string channelId;
    std::string day;
    std::string startTime;
    std::string endTime;
    std::int64_t startAt = 0;
    std::int64_t stopAt = 0;
};

bool parsePositiveUnsigned(
    const std::string& value,
    std::uint64_t& parsed)
{
    try
    {
        std::size_t consumed = 0;
        const unsigned long long number = std::stoull(value, &consumed, 10);
        if (consumed != value.size() || number == 0)
            return false;
        parsed = static_cast<std::uint64_t>(number);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool parsePositiveInt64(
    const std::string& value,
    std::int64_t& parsed)
{
    try
    {
        std::size_t consumed = 0;
        const long long number = std::stoll(value, &consumed, 10);
        if (consumed != value.size() || number <= 0)
            return false;
        parsed = static_cast<std::int64_t>(number);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool safeFixtureToken(const std::string& value)
{
    if (value.empty() || value.size() > 64)
        return false;
    for (const unsigned char character : value)
    {
        if (!(std::isalnum(character) ||
              character == '-' ||
              character == '_' ||
              character == '.'))
        {
            return false;
        }
    }
    return true;
}

bool optionsValid(const FixtureOptions& options)
{
    NativeTimerSpecification specification;
    specification.channelId = options.channelId;
    specification.title = "VDR-Suite Phase69C Acceptance";
    specification.day = options.day;
    specification.weekdays = "-------";
    specification.startTime = options.startTime;
    specification.endTime = options.endTime;
    specification.priority = 50;
    specification.lifetime = 1;
    specification.enabled = true;

    return !options.databasePath.empty()
        && safeFixtureToken(options.fixtureId)
        && !options.backendId.empty()
        && options.backendId.size() <= 128
        && options.backendGeneration > 0
        && options.startAt > 0
        && options.stopAt > options.startAt
        && nativeTimerSpecificationValid(specification);
}

int prepareFixture(const FixtureOptions& options)
{
    if (!optionsValid(options))
    {
        std::cerr << "FIXTURE_PREPARATION=FAIL\n"
                  << "REASON=invalid_arguments\n";
        return 2;
    }

    Database database;
    if (!database.open(options.databasePath))
    {
        std::cerr << "FIXTURE_PREPARATION=FAIL\n"
                  << "REASON=database_open_failed\n";
        return 3;
    }

    TimerIntentRepository intentRepository(database);
    TimerAssignmentRepository assignmentRepository(database);
    if (!intentRepository.ensureSchema() ||
        !assignmentRepository.ensureSchema())
    {
        std::cerr << "FIXTURE_PREPARATION=FAIL\n"
                  << "REASON=schema_unavailable\n";
        return 4;
    }

    const std::string intentId =
        "intent:phase69c-acceptance:" + options.fixtureId;
    const std::string assignmentId =
        "assignment:phase69c-acceptance:" + options.fixtureId;

    if (intentRepository.findById(intentId).status !=
            TimerIntentRepositoryStatus::notFound ||
        assignmentRepository.findById(assignmentId).status !=
            TimerAssignmentRepositoryStatus::notFound)
    {
        std::cerr << "FIXTURE_PREPARATION=FAIL\n"
                  << "REASON=fixture_identity_already_exists\n";
        return 5;
    }

    const std::int64_t now =
        static_cast<std::int64_t>(std::time(nullptr));
    if (now <= 0 ||
        now > std::numeric_limits<std::int64_t>::max() - 2)
    {
        std::cerr << "FIXTURE_PREPARATION=FAIL\n"
                  << "REASON=clock_unavailable\n";
        return 6;
    }

    TimerIntent draft;
    draft.timerIntentId = intentId;
    draft.state = TimerIntentState::draft;
    draft.createdByActorId = "system:phase69c-real-acceptance";
    draft.spec.intentType = TimerIntentType::manualWindow;
    draft.spec.ownerActorId = draft.createdByActorId;
    draft.spec.channelRequirement.canonicalChannelId =
        options.channelId;
    draft.spec.schedule.startAt = options.startAt;
    draft.spec.schedule.stopAt = options.stopAt;
    draft.spec.schedule.timezone = "Europe/Berlin";
    draft.spec.recordingOptions.priority = 50;
    draft.spec.recordingOptions.lifetimeDays = 1;
    draft.spec.assignmentPolicy.preferredBackendIds = {
        options.backendId};
    draft.spec.replicaPolicy.desiredAssignments = 1;
    draft.createdAt = now;
    draft.updatedAt = now;
    draft.expiresAt = options.stopAt + 86400;

    const auto created = intentRepository.create(draft);
    if (!created.ok())
    {
        std::cerr << "FIXTURE_PREPARATION=FAIL\n"
                  << "REASON=intent_create_failed\n";
        return 7;
    }

    TimerIntent active = created.intent;
    active.state = TimerIntentState::active;
    active.updatedAt = now + 1;
    const auto activated = intentRepository.update(
        active,
        created.intent.intentRevision);
    if (!activated.ok())
    {
        std::cerr << "FIXTURE_PREPARATION=FAIL\n"
                  << "REASON=intent_activation_failed\n"
                  << "TIMER_INTENT_ID=" << intentId << "\n";
        return 8;
    }

    TimerAssignmentPlanningBackendCandidate candidate;
    candidate.backendId = options.backendId;
    candidate.siteId = "site:phase69c-real-acceptance";
    candidate.currentBackendGeneration = options.backendGeneration;
    candidate.state = TimerAssignmentPlanningBackendState::online;
    candidate.writeAllowed = true;
    candidate.executionAuthorityCurrent = true;
    candidate.executionAuthorityFence =
        "phase69c-real-acceptance:" + options.fixtureId;

    candidate.capability.backendGeneration =
        options.backendGeneration;
    candidate.capability.revision =
        "phase69c-capability:" + options.fixtureId;
    candidate.capability.current = true;
    candidate.capability.timerCreate = true;
    candidate.capability.timerReadback = true;

    candidate.health.backendGeneration =
        options.backendGeneration;
    candidate.health.revision =
        "phase69c-health:" + options.fixtureId;
    candidate.health.current = true;
    candidate.health.state =
        TimerAssignmentPlanningHealthState::healthy;
    candidate.health.timerWritesAvailable = true;

    candidate.channel.backendGeneration =
        options.backendGeneration;
    candidate.channel.mappingRevision =
        "phase69c-channel:" + options.fixtureId;
    candidate.channel.mappingSource =
        "phase69c-real-acceptance";
    candidate.channel.canonicalChannelId = options.channelId;
    candidate.channel.backendChannelId = options.channelId;
    candidate.channel.current = true;
    candidate.channel.ambiguous = false;

    candidate.desiredNativeTimerSpecificationPresent = true;
    candidate.desiredNativeTimerSpecification.channelId =
        options.channelId;
    candidate.desiredNativeTimerSpecification.title =
        "VDR-Suite Phase69C Acceptance";
    candidate.desiredNativeTimerSpecification.day = options.day;
    candidate.desiredNativeTimerSpecification.weekdays = "-------";
    candidate.desiredNativeTimerSpecification.startTime =
        options.startTime;
    candidate.desiredNativeTimerSpecification.endTime =
        options.endTime;
    candidate.desiredNativeTimerSpecification.priority = 50;
    candidate.desiredNativeTimerSpecification.lifetime = 1;
    candidate.desiredNativeTimerSpecification.enabled = true;
    candidate.desiredNativeTimerSpecification.vps = false;

    candidate.conflict =
        TimerAssignmentPlanningConflictState::confirmedClear;

    TimerAssignmentPrimarySchedulingRequest scheduling;
    scheduling.timerAssignmentId = assignmentId;
    scheduling.timerIntentId = activated.intent.timerIntentId;
    scheduling.expectedIntentRevision =
        activated.intent.intentRevision;
    scheduling.createdAt = now + 2;
    scheduling.candidates = {candidate};

    TimerAssignmentSchedulingService scheduler(
        intentRepository,
        assignmentRepository);
    const auto scheduled = scheduler.schedulePrimary(scheduling);
    if (!scheduled.ok() ||
        scheduled.decision.outcome !=
            TimerAssignmentPlanningOutcome::selected ||
        scheduled.assignment.state !=
            TimerAssignmentState::selected ||
        scheduled.assignment.backendId != options.backendId ||
        scheduled.assignment.backendGeneration !=
            options.backendGeneration ||
        !scheduled.assignment.desiredNativeTimerSpecificationPresent ||
        scheduled.assignment.desiredNativeTimerSpecification.channelId !=
            options.channelId)
    {
        std::cerr << "FIXTURE_PREPARATION=FAIL\n"
                  << "REASON=assignment_schedule_failed\n"
                  << "TIMER_INTENT_ID=" << intentId << "\n";
        return 9;
    }

    std::cout
        << "FIXTURE_PREPARATION=PASS\n"
        << "TIMER_INTENT_ID=" << intentId << "\n"
        << "TIMER_INTENT_REVISION="
        << activated.intent.intentRevision << "\n"
        << "TIMER_ASSIGNMENT_ID="
        << scheduled.assignment.timerAssignmentId << "\n"
        << "ASSIGNMENT_REVISION="
        << scheduled.assignment.assignmentRevision << "\n"
        << "ASSIGNMENT_STATE=selected\n"
        << "BACKEND_ID=" << scheduled.assignment.backendId << "\n"
        << "BACKEND_GENERATION="
        << scheduled.assignment.backendGeneration << "\n"
        << "CHANNEL_ID="
        << scheduled.assignment.desiredNativeTimerSpecification.channelId
        << "\n";
    return 0;
}

FixtureOptions selfTestOptions()
{
    const std::int64_t now =
        static_cast<std::int64_t>(std::time(nullptr));

    FixtureOptions options;
    options.databasePath = ":memory:";
    options.fixtureId = "selftest";
    options.backendId = "default";
    options.backendGeneration = 7;
    options.channelId = "S19.2E-1-1019-10301";
    options.day = "2030-01-01";
    options.startTime = "1000";
    options.endTime = "1100";
    options.startAt = now + 3600;
    options.stopAt = now + 7200;
    return options;
}

void usage(const char* program)
{
    std::cerr
        << "usage: " << program
        << " <database> <fixture-id> <backend-id>"
        << " <backend-generation> <channel-id> <day>"
        << " <start-hhmm> <end-hhmm> <start-epoch> <stop-epoch>\n"
        << "       " << program << " --self-test\n";
}

} // namespace

int main(int argc, char** argv)
{
    if (argc == 2 && std::string(argv[1]) == "--self-test")
        return prepareFixture(selfTestOptions());

    if (argc != 11)
    {
        usage(argv[0]);
        return 64;
    }

    FixtureOptions options;
    options.databasePath = argv[1];
    options.fixtureId = argv[2];
    options.backendId = argv[3];
    if (!parsePositiveUnsigned(argv[4], options.backendGeneration))
    {
        usage(argv[0]);
        return 64;
    }
    options.channelId = argv[5];
    options.day = argv[6];
    options.startTime = argv[7];
    options.endTime = argv[8];
    if (!parsePositiveInt64(argv[9], options.startAt) ||
        !parsePositiveInt64(argv[10], options.stopAt))
    {
        usage(argv[0]);
        return 64;
    }

    return prepareFixture(options);
}
