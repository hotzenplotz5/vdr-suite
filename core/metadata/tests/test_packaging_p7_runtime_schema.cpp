// P7: exercise real C++ repository schema initializers on a disposable
// historical SQLite fixture. The caller controls a fresh private test path.
// NEVER use this helper against an installed VDR-Suite database.
#include "Database.h"
#include "GenreIndexRepository.h"
#include "GlobalSearchRepository.h"
#include "SecurityIdentityRepository.h"
#include "HumanAccountRepository.h"
#include "CredentialVerifierRepository.h"
#include "MutationOperationRepository.h"
#include "TimerIntentRepository.h"
#include "TimerAssignmentRepository.h"
#include "DevicePairingRequestRepository.h"
#include "VdrChannelCacheRepository.h"
#include "BrowserSessionCredentialRepository.h"
#include "ContinueWatching.h"
#include "RecentlyWatched.h"
#include "MediaSessionRepository.h"
#include "MediaTranscodeBackendSettingsRepository.h"
#include "EpgEventRepository.h"
#include "EpgSearchNativeFuzzyCapabilityRepository.h"
#include "VdrRecordingCacheRepository.h"
#include "EpgArtworkRepository.h"
#include "PublicRecordingIdentityRepository.h"
#include "RecordingSeriesHierarchyOverrideRepository.h"
#include "AccountabilityEventRepository.h"
#include "DeviceCredentialVerifierRepository.h"
#include "SecurityPermissionGrantRepository.h"
#include <sqlite3.h>
#include <sys/stat.h>
#include <unistd.h>
#include <filesystem>
#include <iostream>
#include <string>

namespace {
bool privateScratchPath(const std::filesystem::path& path)
{
    namespace fs = std::filesystem;
    if (geteuid() == 0 || !path.is_absolute() ||
        path.lexically_normal() != path || path.filename() != "old.db")
        return false;
    const fs::path folder = path.parent_path();
    const std::string name = folder.filename().string();
    if (name.rfind("p7-cpp-repository-", 0) != 0)
        return false;
    std::error_code ec;
    if (fs::is_symlink(fs::symlink_status(folder, ec)) || ec ||
        fs::is_symlink(fs::symlink_status(path, ec)) || ec ||
        fs::canonical(folder, ec) != folder || ec)
        return false;
    struct stat directory = {};
    struct stat database = {};
    if (::stat(folder.c_str(), &directory) != 0 ||
        ::stat(path.c_str(), &database) != 0)
        return false;
    return S_ISDIR(directory.st_mode) &&
           S_ISREG(database.st_mode) &&
           directory.st_uid == geteuid() &&
           database.st_uid == geteuid() &&
           (directory.st_mode & 0077) == 0;
}

bool scalarPositive(sqlite3* db, const char* sql)
{
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;
    const bool ok = sqlite3_step(stmt) == SQLITE_ROW &&
                    sqlite3_column_int(stmt, 0) > 0;
    sqlite3_finalize(stmt);
    return ok;
}
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "expected one isolated SQLite fixture path\n";
        return 2;
    }
    if (!privateScratchPath(argv[1])) {
        std::cerr << "P7 fixture path rejected: requires private scratch old.db\n";
        return 2;
    }
    Database database;
    if (!database.open(argv[1])) return 3;
    if (!database.execute("PRAGMA foreign_keys=ON;")) return 4;

    // New instances each pass defeat any in-memory schemaReady_ shortcut.
    for (int pass = 0; pass < 2; ++pass) {
        GenreIndexRepository genres(database);
        if (!genres.ensureSchema()) {
            std::cerr << "P7_METADATA_RUNTIME_SCHEMA=FAIL pass=" << pass << "\n";
            return 5;
        }
        GlobalSearchRepository globalSearch(database);
        if (!globalSearch.ensureSchema()) {
            std::cerr << "P7_GLOBAL_SEARCH_RUNTIME_SCHEMA=FAIL pass=" << pass << "\n";
            return 6;
        }
        SecurityIdentityRepository identities(database);
        HumanAccountRepository accounts(database);
        CredentialVerifierRepository verifiers(database);
        vdrsuite::operations::MutationOperationRepository operations(database);
        vdrsuite::timers::TimerIntentRepository intents(database);
        vdrsuite::timers::TimerAssignmentRepository assignments(database);
        DevicePairingRequestRepository pairing(database);
        VdrChannelCacheRepository channels(database);
        BrowserSessionCredentialRepository browserSessions(database);
        ContinueWatchingRepository continueWatching(database);
        RecentlyWatchedRepository recentlyWatched(database);
        MediaSessionRepository mediaSessions(database);
        MediaTranscodeBackendSettingsRepository transcodeSettings(database);
        EpgEventRepository epgEvents(database);
        EpgSearchNativeFuzzyCapabilityRepository fuzzyCapability(database);
        VdrRecordingCacheRepository recordingCache(database);
        EpgArtworkRepository epgArtwork(database);
        PublicRecordingIdentityRepository recordingIdentities(database);
        RecordingSeriesHierarchyOverrideRepository seriesOverrides(database);
        AccountabilityEventRepository accountability(database);
        DeviceCredentialVerifierRepository deviceVerifiers(database);
        SecurityPermissionGrantRepository permissions(database);
        if (!identities.ensureSchema() ||
            !accounts.ensureSchema() ||
            !verifiers.ensureSchema()) {
            std::cerr << "P7_SECURITY_RUNTIME_SCHEMA=FAIL pass=" << pass << "\n";
            return 8;
        }
        if (!operations.ensureSchema()) {
            std::cerr << "P7_OPERATIONS_RUNTIME_SCHEMA=FAIL pass=" << pass << "\n";
            return 9;
        }
        if (!intents.ensureSchema() || !assignments.ensureSchema()) {
            std::cerr << "P7_TIMERS_RUNTIME_SCHEMA=FAIL pass=" << pass << "\n";
            return 10;
        }
        if (!pairing.ensureSchema()) {
            std::cerr << "P7_PAIRING_RUNTIME_SCHEMA=FAIL pass=" << pass << "\n";
            return 11;
        }
        if (!channels.ensureSchema()) {
            std::cerr << "P7_CHANNEL_CACHE_RUNTIME_SCHEMA=FAIL pass=" << pass << "\n";
            return 12;
        }
        if (!browserSessions.ensureSchema()) {
            std::cerr << "P7_BROWSER_SESSION_RUNTIME_SCHEMA=FAIL pass=" << pass << "\n";
            return 13;
        }
        if (!continueWatching.ensureSchema() || !recentlyWatched.ensureSchema()) {
            std::cerr << "P7_PLAYBACK_STATE_RUNTIME_SCHEMA=FAIL pass=" << pass << "\n";
            return 14;
        }
        if (!mediaSessions.ensureSchema() || !transcodeSettings.ensureSchema()) {
            std::cerr << "P7_MEDIA_RUNTIME_SCHEMA=FAIL pass=" << pass << "\n";
            return 15;
        }
        if (!epgEvents.ensureSchema() || !fuzzyCapability.ensureSchema()) {
            std::cerr << "P7_EPG_RUNTIME_SCHEMA=FAIL pass=" << pass << "\n";
            return 16;
        }
        if (!recordingCache.ensureSchema() || !recordingIdentities.ensureSchema() ||
            !seriesOverrides.ensureSchema()) {
            std::cerr << "P7_RECORDING_RUNTIME_SCHEMA=FAIL pass=" << pass << "\n";
            return 17;
        }
        if (!epgArtwork.ensureSchema()) {
            std::cerr << "P7_EPG_ARTWORK_RUNTIME_SCHEMA=FAIL pass=" << pass << "\n";
            return 18;
        }
        if (!accountability.ensureSchema() || !deviceVerifiers.ensureSchema() ||
            !permissions.ensureSchema()) {
            std::cerr << "P7_SECURITY_GRANTS_AUDIT_SCHEMA=FAIL pass=" << pass << "\n";
            return 19;
        }
        if (!scalarPositive(database.handle(),
            "SELECT count(*) FROM suite_metadata_schema_versions;") ||
            !scalarPositive(database.handle(),
            "SELECT count(*) FROM vdr_global_search_schema_versions;") ||
            !scalarPositive(database.handle(),
            "SELECT count(*) FROM pragma_integrity_check WHERE integrity_check='ok';") ||
            !scalarPositive(database.handle(),
            "SELECT CASE WHEN (SELECT count(*) FROM pragma_foreign_key_check)=0 THEN 1 ELSE 0 END;") ||
            !scalarPositive(database.handle(),
            "SELECT CASE WHEN (SELECT count(*) FROM sqlite_master WHERE name IN "
            "('security_actors','security_human_accounts','security_credentials',"
            "'security_basic_credential_verifiers','mutation_operations',"
            "'mutation_operation_payloads','timer_intents','timer_assignments',"
            "'security_device_pairing_requests','vdr_channel_cache',"
            "'security_browser_session_credentials','continue_watching_state',"
            "'recently_watched_state','media_sessions','media_routes',"
            "'media_provider_stream_leases','media_access_grants',"
            "'backend_media_transcode_settings','epg_events',"
            "'epgsearch_native_fuzzy_capability_probes',"
            "'vdr_recording_cache','public_recording_identity',"
            "'recording_series_hierarchy_overrides','epg_event_artwork',"
            "'epg_scraper_metadata_cache','epg_scraper_metadata_images',"
            "'epg_scraper_metadata_people','accountability_events',"
            "'security_device_credential_verifiers',"
            "'security_actor_permission_grants'))=30 "
            "THEN 1 ELSE 0 END;")) {
            std::cerr << "P7_RUNTIME_SQLITE_INTEGRITY=FAIL\n";
            return 7;
        }
    }
    database.close();
    std::cout << "P7_RUNTIME_SCHEMA_REPOSITORIES=PASS\n";
    return 0;
}
