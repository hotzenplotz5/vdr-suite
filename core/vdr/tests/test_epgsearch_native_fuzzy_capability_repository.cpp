#include "Database.h"
#include "EpgSearchNativeFuzzyCapabilityDetector.h"
#include "EpgSearchNativeFuzzyCapabilityRepository.h"

#include <cassert>
#include <cstdio>
#include <iostream>

static const char* dbPath = "/tmp/vdr-suite-native-fuzzy-capability-repository-test.db";

static Database openDatabase()
{
    std::remove(dbPath);

    Database database;
    assert(database.open(dbPath));
    assert(database.isOpen());

    return database;
}

static void assertProbeResult(
    const EpgSearchNativeFuzzyCapabilityProbeResult& result,
    bool createAccepted,
    bool readbackAvailable,
    bool modePreserved,
    bool tolerancePreserved,
    bool cleanupSucceeded)
{
    assert(result.createAccepted == createAccepted);
    assert(result.readbackAvailable == readbackAvailable);
    assert(result.modePreserved == modePreserved);
    assert(result.tolerancePreserved == tolerancePreserved);
    assert(result.cleanupSucceeded == cleanupSucceeded);
}

static void test_schema_is_created()
{
    Database database = openDatabase();
    EpgSearchNativeFuzzyCapabilityRepository repository(database);

    assert(!database.tableExists("epgsearch_native_fuzzy_capability_probes"));
    assert(repository.ensureSchema());
    assert(database.tableExists("epgsearch_native_fuzzy_capability_probes"));
}

static void test_missing_backend_returns_empty_result()
{
    Database database = openDatabase();
    EpgSearchNativeFuzzyCapabilityRepository repository(database);

    assert(repository.ensureSchema());
    assert(!repository.load("missing").has_value());
}

static void test_successful_probe_roundtrip()
{
    Database database = openDatabase();
    EpgSearchNativeFuzzyCapabilityRepository repository(database);
    EpgSearchNativeFuzzyCapabilityDetector detector;

    assert(repository.ensureSchema());

    const EpgSearchNativeFuzzyCapabilityProbeResult successful =
        EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip();

    assert(repository.save("default", successful));

    const auto loaded = repository.load("default");
    assert(loaded.has_value());
    assertProbeResult(*loaded, true, true, true, true, true);
    assert(detector.nativeFuzzyAvailable(*loaded));
}

static void test_failed_probe_overwrites_previous_success()
{
    Database database = openDatabase();
    EpgSearchNativeFuzzyCapabilityRepository repository(database);
    EpgSearchNativeFuzzyCapabilityDetector detector;

    assert(repository.ensureSchema());

    assert(repository.save(
        "default",
        EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip()));

    EpgSearchNativeFuzzyCapabilityProbeResult failed =
        EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip();
    failed.cleanupSucceeded = false;

    assert(repository.save("default", failed));

    const auto loaded = repository.load("default");
    assert(loaded.has_value());
    assertProbeResult(*loaded, true, true, true, true, false);
    assert(!detector.nativeFuzzyAvailable(*loaded));
}

static void test_multiple_backends_are_independent()
{
    Database database = openDatabase();
    EpgSearchNativeFuzzyCapabilityRepository repository(database);
    EpgSearchNativeFuzzyCapabilityDetector detector;

    assert(repository.ensureSchema());

    EpgSearchNativeFuzzyCapabilityProbeResult livingRoom =
        EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip();

    EpgSearchNativeFuzzyCapabilityProbeResult holidayHome =
        EpgSearchNativeFuzzyCapabilityDetector::successfulRoundTrip();
    holidayHome.modePreserved = false;

    assert(repository.save("living-room", livingRoom));
    assert(repository.save("holiday-home", holidayHome));

    const auto loadedLivingRoom = repository.load("living-room");
    const auto loadedHolidayHome = repository.load("holiday-home");

    assert(loadedLivingRoom.has_value());
    assert(loadedHolidayHome.has_value());

    assert(detector.nativeFuzzyAvailable(*loadedLivingRoom));
    assert(!detector.nativeFuzzyAvailable(*loadedHolidayHome));
}

int main()
{
    test_schema_is_created();
    test_missing_backend_returns_empty_result();
    test_successful_probe_roundtrip();
    test_failed_probe_overwrites_previous_success();
    test_multiple_backends_are_independent();

    std::cout
        << "test_epgsearch_native_fuzzy_capability_repository passed"
        << std::endl;

    return 0;
}
