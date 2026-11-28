#include "BrowserLiveEventSubscription.h"
#include "ContinueWatchingApiRuntime.h"
#include "VdrRecordingCacheRepository.h"
#include "ApiRouter.h"
#include "BackendRegistry.h"
#include "BackendRegistryController.h"
#include "VdrCapabilitySet.h"
#include "CapabilityResolver.h"
#include "CapabilityReportService.h"
#include "CapabilityReportJsonSerializer.h"
#include "CapabilityReportBuilder.h"
#include "CapabilityController.h"
#include "BackendRegistryJsonSerializer.h"
#include "BackendRegistryService.h"
#include "DashboardController.h"
#include "EpgController.h"
#include "EpgSearchResultJsonSerializer.h"
#include "EpgSearchService.h"
#include "EpgQueryService.h"
#include "DashboardFacade.h"
#include "DashboardJsonSerializer.h"
#include "Database.h"
#include "TestDatabasePath.h"
#include "JobDashboardService.h"
#include "JobRepository.h"
#include "JobsController.h"
#include "LiveTransportController.h"
#include "MetadataController.h"
#include "MetadataRepository.h"
#include "MockVdrAdapter.h"
#include "RecordingDashboardService.h"
#include "RecordingRepository.h"
#include "RecordingsController.h"
#include "RecordingActionExecutionController.h"
#include "RecordingActionValidationController.h"
#include "RecordingActionValidationRequestParser.h"
#include "RecordingActionExecutionResultJsonSerializer.h"
#include "RecordingActionPreviewController.h"
#include "RecordingActionRequestPreviewResultJsonSerializer.h"
#include "RecordingActionRequestPreviewService.h"
#include "RecordingActionValidationResultJsonSerializer.h"
#include "RecordingActionBackendExecutorAdapterRegistry.h"
#include "RecordingActionExecutionService.h"
#include "RecordingActionValidationService.h"
#include "RuntimeDiagnosticsController.h"
#include "RuntimeDiagnosticsJsonSerializer.h"
#include "RuntimeDiagnosticsService.h"
#include "RuntimeMeasurement.h"
#include "SearchTimerDiscoveryController.h"
#include "SearchTimerDiscoveryJsonSerializer.h"
#include "SearchTimerDiscoveryService.h"
#include "SearchTimerDiscoveryStaticProvider.h"
#include "SnapshotAccessService.h"
#include "SnapshotChangeFeed.h"
#include "SnapshotChangeFeedController.h"
#include "SnapshotChangeFeedJsonSerializer.h"
#include "SseLiveTransport.h"
#include "DeviceLiveEventHub.h"
#include "SnapshotCache.h"
#include "SnapshotCacheService.h"
#include "TestHttpServer.h"
#include "HumanAccountRepository.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityPermissionGrantRepository.h"
#include "HomeCacheScope.h"
#include "VdrController.h"
#include "VdrOverviewJsonSerializer.h"
#include "VdrOverviewService.h"
#include "VdrRecordingQueryController.h"
#include "VdrTimerActionController.h"
#include "VdrTimerActionExecutionService.h"
#include "VdrTimerActionExecutorAdapterRegistry.h"
#include "VdrTimerActionRequestParser.h"
#include "VdrTimerActionResultJsonSerializer.h"
#include "VdrRecordingQueryResultJsonSerializer.h"
#include "VdrRecordingQueryService.h"
#include "VdrService.h"
#include "VdrSnapshot.h"
#include "VdrSnapshotReadJsonSerializer.h"
#include "VdrSnapshotReadService.h"

#include <cassert>
#include <chrono>
#include <memory>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <string>

static VdrSnapshot makeHttpServerSnapshot()
{
    VdrSnapshot snapshot;
    snapshot.backendId = "default";
    snapshot.status.enabled = true;
    snapshot.status.mode = "http-server-snapshot";
    snapshot.status.host = "http-server-host";
    snapshot.status.port = 8002;
    snapshot.status.state = "cached";
    return snapshot;
}

static void assertJsonResponse(
    const HttpServerResponse& response,
    int expectedStatusCode)
{
    assert(response.statusCode == expectedStatusCode);
    assert(response.headers.at("Content-Type") == "application/json");
}

static void authorize(HttpServerRequest& request)
{
    request.headers["Authorization"] =
        "Basic cGhhc2U2Mi1hZG1pbjp0ZXN0LXBhc3N3b3Jk";
}

static void authorizeRetiredLegacy(HttpServerRequest& request)
{
    request.headers["Authorization"] =
        "Basic YWRtaW46dmRyLXN1aXRl";
}

static void assertEmptyDiscoveryResponse(
    const HttpServerResponse& response,
    const std::string& expectedBackendId)
{
    assertJsonResponse(response, 200);

    const std::string expectedBody =
        "{"
        "\"backendId\":\"" + expectedBackendId + "\","
        "\"counts\":{"
        "\"extendedEpgInfo\":0,"
        "\"channelGroups\":0,"
        "\"blacklists\":0,"
        "\"recordingDirectories\":0"
        "},"
        "\"extendedEpgInfo\":[],"
        "\"channelGroups\":[],"
        "\"blacklists\":[],"
        "\"recordingDirectories\":[]"
        "}";

    assert(response.body == expectedBody);
}

int main()
{
    {
        Database historyDb;
        assert(historyDb.open(":memory:"));
        VdrRecordingCacheRepository historyCache(historyDb);
        assert(historyCache.ensureSchema());
        VdrRecording recording;
        recording.backendId = "default";
        recording.id = "history-r1";
        recording.backendNativeId = "/video/history-r1.rec";
        recording.path = recording.backendNativeId;
        recording.title = "History fixture";
        recording.durationSeconds = 1800;
        recording.recordingDurationKnown = true;
        assert(historyCache.upsertRecordingsForBackend("default", {recording}));

        auto& runtime = ContinueWatchingApiRuntime::instance();
        runtime.reset();
        int lookups = 0;
        assert(runtime.configure(historyDb, historyCache,
            [&lookups](const std::string& backend, const std::string& native)
            {
                ++lookups;
                assert(backend == "default");
                assert(native == "/video/history-r1.rec");
                return std::string("rec_0123456789abcdef0123456789abcdef");
            }, [](const VdrRecording&, const ManualRecordingMetadataAssignment&) {
                return std::string("0123456789abcdef0123456789abcdef");
            }));
        ApiResponse history;
        const std::string actor = "history-actor";
        const std::string cw = "/api/media/continue-watching";
        const std::string rw = "/api/media/recently-watched";
        const std::string list = R"({"operation":"list","backendId":"default"})";
        assert(runtime.tryHandlePost(cw,
            R"({"operation":"progress","backendId":"default","recordingId":"history-r1","operationId":"history-progress","positionSeconds":120,"resumeSupported":true})",
            actor, history));
        assert(history.statusCode == 200);
        assert(runtime.tryHandlePost(rw,
            R"({"operation":"activity","backendId":"default","recordingId":"history-r1","operationId":"history-activity","positionSeconds":120,"resumeSupportKnown":true,"resumeSupported":true})",
            actor, history));
        assert(history.statusCode == 200);
        for (const auto& route : {cw, rw}) {
            assert(runtime.tryHandlePost(route, list, actor, history));
            assert(history.statusCode == 200);
            assert(history.body.find("\"recording\":{\"publicRecordingId\":\"rec_0123456789abcdef0123456789abcdef\"") != std::string::npos);
            assert(history.body.find("\"recordingId\":\"history-r1\"") != std::string::npos);
            assert(runtime.tryHandlePost(route, list, "other-actor", history));
            assert(history.statusCode == 200);
            assert(history.body.find("publicRecordingId") == std::string::npos);
        }
        assert(lookups == 2);
        const auto resolve = [&recording](const std::string& backend,
            const std::string& id, VdrRecording& out) {
            if (backend != "default" || id != "rec_0123456789abcdef0123456789abcdef")
                return false;
            out = recording;
            return true;
        };
        const std::string progress = "/api/v1/browser/recording-playback-sessions/progress";
        const std::string activity = "/api/v1/browser/recording-playback-sessions/activity";
        const std::string revisionRead = R"({"operation":"revision","backendId":"default","recordingId":"rec_0123456789abcdef0123456789abcdef"})";
        assert(runtime.tryHandlePost(progress, revisionRead, actor, history, "other", resolve));
        assert(history.statusCode == 403);
        assert(runtime.tryHandlePost(progress, revisionRead, actor, history, "default", resolve));
        assert(history.statusCode == 200);
        // Legacy and Public-v1 observe the same repository revision. Read the
        // actual baseline instead of assuming the legacy initial revision.
        const auto baseline = history.body;
        assert(runtime.tryHandlePost(cw,
            R"({"operation":"revision","backendId":"default","recordingId":"history-r1"})", actor, history));
        assert(history.body == baseline);
        const std::string revision = baseline.substr(baseline.find(':') + 1,
            baseline.size() - baseline.find(':') - 2);
        const std::string write = std::string(R"({"operation":"progress","backendId":"default","recordingId":"rec_0123456789abcdef0123456789abcdef","operationId":"public-history-progress","positionSeconds":240,"resumeSupported":true,"notifyLive":true,"expectedRevision":)") + revision + "}";
        assert(runtime.tryHandlePost(progress, write, actor, history, "default", resolve));
        assert(history.statusCode == 200);
        assert(history.body.find("history-r1") == std::string::npos);
        assert(runtime.tryHandlePost(cw, list, actor, history));
        assert(history.body.find("\"resumePositionSeconds\":240") != std::string::npos);
        assert(runtime.tryHandlePost(progress,
            R"({"operation":"clear","backendId":"default","recordingId":"rec_0123456789abcdef0123456789abcdef","operationId":"stale-clear","expectedRevision":0})",
            actor, history, "default", resolve));
        assert(history.statusCode == 409);
        assert(runtime.tryHandlePost(progress,
            R"({"operation":"revision","backendId":"default","recordingId":"history-r1"})",
            actor, history, "default", resolve));
        assert(history.statusCode == 400);
        assert(runtime.tryHandlePost(activity,
            R"({"operation":"activity","backendId":"default","recordingId":"rec_0123456789abcdef0123456789abcdef","operationId":"public-history-activity","positionSeconds":240,"resumeSupportKnown":true,"resumeSupported":true})",
            actor, history, "default", resolve));
        assert(history.statusCode == 200);
        assert(runtime.tryHandlePost(rw, list, actor, history));
        assert(history.body.find("\"positionSeconds\":240") != std::string::npos);
        for (const auto& route : {progress, activity}) {
            assert(runtime.tryHandlePost(route, list, actor, history, "default", resolve));
            assert(history.statusCode == 200);
            assert(history.body.find("\"recordingId\":\"rec_0123456789abcdef0123456789abcdef\"") != std::string::npos);
            assert(history.body.find("\"preferredArtworkId\":\"0123456789abcdef0123456789abcdef\"") != std::string::npos);
            assert(history.body.find("/api/v1/recordings/rec_0123456789abcdef0123456789abcdef/artwork/0123456789abcdef0123456789abcdef?backendId=default") != std::string::npos);
            assert(history.body.find("\"backendNativeId\"") == std::string::npos);
            assert(history.body.find("history-r1") == std::string::npos);
            assert(runtime.tryHandlePost(route, list, "other-actor", history, "default", resolve));
            assert(history.statusCode == 200 && history.body == "{\"items\":[]}");
        }
        runtime.reset();
    }
    {
        RequestSecurityContext context;
        context.authenticationState = AuthenticationState::Authenticated;
        context.actor = {"home-a", ActorType::User, "Home A", true};
        context.session = SessionIdentity{"session-a"};
        context.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
        context.grants = {{"recordings.view", "default"}, {"media.recording.play", "default"}};
        HttpServerRequest request;
        request.method = "GET";
        request.path = "/api/backends/default/snapshot";
        HttpServerResponse response;
        response.body = "{}";
        vdrsuite::http::addHomeCacheScope(request, response, context, true, {});
        const auto original = response.headers.at("X-VDR-Suite-Home-Recording-Scope");
        assert(original == response.headers.at("X-VDR-Suite-Home-History-Scope"));
        assert(response.headers.at("X-VDR-Suite-Home-Backend") == "default");
        assert(response.body == "{}" && response.statusCode == 200);
        response = HttpServerResponse{};
        context.session = SessionIdentity{"session-b"};
        vdrsuite::http::addHomeCacheScope(request, response, context, true, {});
        assert(original != response.headers.at("X-VDR-Suite-Home-Recording-Scope"));
        context.session = SessionIdentity{"session-a"};
        std::reverse(context.grants.begin(), context.grants.end());
        assert(original == vdrsuite::http::browserHomeCacheScope(context, "default"));
        assert(original != vdrsuite::http::browserHomeCacheScope(context, "other"));
        context.grants.push_back({"timers.view", "default"});
        assert(original != vdrsuite::http::browserHomeCacheScope(context, "default"));
        context.grants.pop_back();
        context.session->revoked = true;
        assert(vdrsuite::http::browserHomeCacheScope(context, "default").empty());
        context.session->revoked = false;
        context.permissionGrantResolution = PermissionGrantResolutionState::Unavailable;
        assert(vdrsuite::http::browserHomeCacheScope(context, "default").empty());
        context.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
        for (const auto& query : {"backend=default&backend=other",
                "backend=default&backendId=other", "backend=default&%62ackend=other"})
            assert(vdrsuite::http::canonicalHomeCacheBackend(query).empty());
        assert(vdrsuite::http::canonicalHomeCacheBackend("backend=default&variant=home") == "default");
        response = HttpServerResponse{};
        context.actor.actorId = "home-b";
        vdrsuite::http::addHomeCacheScope(request, response, context, true, {});
        assert(original != response.headers.at("X-VDR-Suite-Home-Recording-Scope"));
        response = HttpServerResponse{};
        request.path = "/api/vdr/recordings/query?backend=default&backend=other";
        vdrsuite::http::addHomeCacheScope(request, response, context, true, {});
        assert(response.headers.empty());
        request.path = "/api/vdr/recordings/query?backend=other";
        vdrsuite::http::addHomeCacheScope(request, response, context, true, {});
        assert(response.headers.empty());
        request.path = "/api/vdr/recordings/query?backend=default";
        vdrsuite::http::addHomeCacheScope(request, response, context, false, {});
        assert(response.headers.empty());
        // Public-v1 genre pages use the same session/backend/grant proof as
        // the legacy overview, but only after the Public-v1 scope was admitted.
        context.grants = {{"recordings.view", "default"}};
        const auto genreScope = vdrsuite::http::browserHomeCacheScope(context, "default");
        for (const auto& target : {
            "/api/metadata/genres?backend=default&scope=recordings",
            "/api/v1/genres?backendId=default&locale=de&limit=30&offset=0",
            "/api/v1/genres?backendId=default&locale=de&limit=30&offset=30",
            "/api/v1/recordings?backendId=default&view=folders&limit=100&offset=0",
            "/api/v1/recordings?backendId=default&view=folders&folderId=fld1_0123456789abcdef0123456789abcdef&limit=12&offset=0"}) {
            request.path = target;
            response = HttpServerResponse{};
            response.body = "{}";
            vdrsuite::http::addHomeCacheScope(request, response, context, true, "default");
            assert(response.headers.at("X-VDR-Suite-Home-Recording-Scope") == genreScope);
            assert(response.headers.at("X-VDR-Suite-Home-Backend") == "default");
            assert(response.headers.at("Cache-Control") == "private, no-store");
            assert(response.headers.count("X-VDR-Suite-Home-History-Scope") == 0);
            assert(response.body == "{}" && response.statusCode == 200);
        }
        for (const auto& target : {
            "/api/v1/genres?backendId=default&backendId=other",
            "/api/v1/genres?backendId=default&backend=default",
            "/api/v1/genres?backend=default",
            "/api/v1/genres?%62ackendId=default",
            "/api/v1/genres?backendId=other",
            "/api/v1/genres/epg?backendId=default"}) {
            request.path = target;
            response = HttpServerResponse{};
            vdrsuite::http::addHomeCacheScope(request, response, context, true, "default");
            assert(response.headers.empty());
        }
        request.path = "/api/v1/genres?backendId=default";
        response = HttpServerResponse{};
        vdrsuite::http::addHomeCacheScope(request, response, context, true, "other");
        assert(response.headers.empty());
        vdrsuite::http::addHomeCacheScope(request, response, context, false, "default");
        assert(response.headers.empty());
        response.statusCode = 503;
        vdrsuite::http::addHomeCacheScope(request, response, context, true, "default");
        assert(response.headers.empty() && response.statusCode == 503);
        response = HttpServerResponse{};
        context.grants = {{"epg.view", "default"}};
        vdrsuite::http::addHomeCacheScope(request, response, context, true, "default");
        assert(response.headers.empty());
        context.grants = {{"recordings.view", "default"}};
        request.path = "/recording-artwork/default/0123456789abcdef0123456789abcdef?variant=home";
        response = HttpServerResponse{};
        response.statusCode = 404;
        vdrsuite::http::addDirectHomeArtworkFallbackScope(request, response, context, true);
        assert(response.statusCode == 404);
        assert(response.headers.at("X-VDR-Suite-Artwork-Scope") ==
            vdrsuite::http::browserHomeCacheScope(context, "default"));
        assert(response.headers.at("Cache-Control") == "private, no-store");
        for (const auto& target : {
            "/recording-artwork/other/0123456789abcdef0123456789abcdef?variant=home",
            "/recording-artwork/default/short?variant=home",
            "/recording-artwork/%64efault/0123456789abcdef0123456789abcdef?variant=home",
            "/recording-artwork/default/0123456789abcdef0123456789abcdef?variant=home&backend=other",
            "/recording-artwork/default/0123456789abcdef0123456789abcdef"}) {
            request.path = target;
            response = HttpServerResponse{};
            response.statusCode = 404;
            vdrsuite::http::addDirectHomeArtworkFallbackScope(request, response, context, true);
            assert(response.headers.empty());
        }
        request.path = "/recording-artwork/default/0123456789abcdef0123456789abcdef?variant=home";
        response = HttpServerResponse{};
        response.statusCode = 404;
        vdrsuite::http::addDirectHomeArtworkFallbackScope(request, response, context, false);
        assert(response.headers.empty());
        context.grants = {{"media.recording.play", "default"}};
        vdrsuite::http::addDirectHomeArtworkFallbackScope(request, response, context, true);
        assert(response.headers.empty());

        const std::string epgTarget =
            "/api/epg/cache/artwork?backend=default&channelId=C-1&eventId=42";
        context.grants = {{"recordings.view", "default"}};
        request.path = epgTarget;
        response = HttpServerResponse{};
        vdrsuite::http::addEpgArtworkScope(request, response, context, true);
        assert(response.headers.empty());
        context.grants = {{"epg.view", "default"}};
        vdrsuite::http::addEpgArtworkScope(request, response, context, true);
        const auto epgScope = vdrsuite::http::browserHomeCacheScope(context, "default");
        assert(response.headers.at("X-VDR-Suite-Artwork-Scope") == epgScope);
        response = HttpServerResponse{};
        response.statusCode = 404;
        vdrsuite::http::addEpgArtworkScope(request, response, context, true);
        assert(response.statusCode == 404 && response.headers.at("Cache-Control") == "private, no-store");
        const std::string epgMetadata = "/api/epg/cache/metadata/image?backend=default&channelId=C-1&eventId=42&kind=preferred&index=0";
        request.path = epgMetadata;
        response = HttpServerResponse{};
        vdrsuite::http::addEpgArtworkScope(request, response, context, true);
        assert(response.headers.at("X-VDR-Suite-Artwork-Scope") == epgScope);
        for (const auto& target : {epgTarget + "&eventId=43", epgTarget + "&token=bad",
                epgTarget + "&variant=full", epgMetadata + "&index=1",
                std::string("/api/epg/cache/artwork?backend=other&channelId=C-1&eventId=42"),
                std::string("/api/epg/cache/artwork?%62ackend=default&channelId=C-1&eventId=42")}) {
            request.path = target;
            response = HttpServerResponse{};
            vdrsuite::http::addEpgArtworkScope(request, response, context, true);
            assert(response.headers.empty());
        }
        request.path = epgTarget;
        response = HttpServerResponse{};
        vdrsuite::http::addEpgArtworkScope(request, response, context, false);
        assert(response.headers.empty());
        for (const auto* read : {"/api/backends/default/snapshot",
            "/api/epg/cache/now-next-artwork?backend=default&channelIds=C-1"}) {
            request.path = read;
            response = HttpServerResponse{};
            vdrsuite::http::addHomeCacheScope(request, response, context, true, {});
            assert(response.headers.at("X-VDR-Suite-Home-Epg-Scope") == epgScope);
            assert(response.headers.count("X-VDR-Suite-Home-Recording-Scope") == 0);
        }

        const std::string seriesTarget =
            "/api/backends/default/settings/series-artwork/image?seriesKey=folder%3Aseries&revision=7&variant=home";
        context.grants = {{"recordings.view", "default"}};
        request.path = seriesTarget;
        response = HttpServerResponse{};
        response.headers["X-VDR-Suite-Series-Cover-Revision"] = "7";
        vdrsuite::http::addSeriesHomeArtworkScope(request, response, context, true);
        assert(response.headers.at("X-VDR-Suite-Artwork-Scope") ==
            vdrsuite::http::browserHomeCacheScope(context, "default"));
        assert(response.headers.count("X-VDR-Suite-Series-Cover-Revision") == 0);
        for (const auto& target : {seriesTarget + "&token=bad",
            std::string("/api/backends/other/settings/series-artwork/image?seriesKey=x&revision=7&variant=home"),
            std::string("/api/backends/default/settings/series-artwork/image?seriesKey=x&revision=8&variant=home"),
            std::string("/api/backends/default/settings/series-artwork/candidate-image?seriesKey=x&revision=7&variant=home")}) {
            request.path = target;
            response = HttpServerResponse{};
            response.headers["X-VDR-Suite-Series-Cover-Revision"] = "7";
            vdrsuite::http::addSeriesHomeArtworkScope(request, response, context, true);
            assert(response.headers.empty());
        }
        request.path = seriesTarget;
        response = HttpServerResponse{};
        response.headers["X-VDR-Suite-Series-Cover-Revision"] = "7";
        vdrsuite::http::addSeriesHomeArtworkScope(request, response, context, false);
        assert(response.headers.empty());
        context.grants = {{"media.recording.play", "default"}};
        response.headers["X-VDR-Suite-Series-Cover-Revision"] = "7";
        vdrsuite::http::addSeriesHomeArtworkScope(request, response, context, true);
        assert(response.headers.empty());
        response = HttpServerResponse{};
        request.method = "POST";
        request.path = "/api/media/continue-watching";
        request.body = R"({"operation":"clear","backendId":"default"})";
        vdrsuite::http::addHomeCacheScope(request, response, context, true, "default");
        assert(response.headers.empty());
        request.body = R"({"operation":"list","backendId":"default"})";
        vdrsuite::http::addHomeCacheScope(request, response, context, true, "default");
        assert(response.headers.count("X-VDR-Suite-Home-History-Scope") == 1);
        assert(response.headers.count("X-VDR-Suite-Home-Recording-Scope") == 0);
    }
    Database db;
    if (!db.open(vdrSuiteTestDatabasePath()))
    {
        std::cerr << "database open failed" << std::endl;
        return 1;
    }

    JobRepository jobRepository(db);
    RecordingRepository recordingRepository(db);
    MetadataRepository metadataRepository(db);

    JobDashboardService jobDashboardService(jobRepository);
    RecordingDashboardService recordingDashboardService(
        recordingRepository,
        metadataRepository);
    DashboardFacade dashboardFacade(
        jobDashboardService,
        recordingDashboardService);
    DashboardJsonSerializer dashboardJsonSerializer;
    DashboardController dashboardController(
        dashboardFacade,
        dashboardJsonSerializer);

    JobsController jobsController(jobRepository);
    RecordingsController recordingsController(recordingRepository);
    MetadataController metadataController(metadataRepository);

    MockVdrAdapter adapter;
    VdrService vdrService(adapter);
    EpgQueryService epgQueryService(vdrService);
    EpgSearchService epgSearchService;
    EpgSearchResultJsonSerializer epgSearchResultJsonSerializer;
    EpgController epgController(
        epgQueryService,
        epgSearchService,
        epgSearchResultJsonSerializer);
    VdrOverviewService vdrOverviewService(vdrService);
    VdrOverviewJsonSerializer vdrJsonSerializer;
    VdrRecordingQueryService vdrRecordingQueryService(vdrService);
    VdrRecordingQueryResultJsonSerializer vdrRecordingQueryJsonSerializer;
    VdrRecordingQueryController vdrRecordingQueryController(
        vdrRecordingQueryService,
        vdrRecordingQueryJsonSerializer);

    SnapshotCache snapshotCache;
    SnapshotCacheService snapshotCacheService(snapshotCache);
    SnapshotAccessService snapshotAccessService(snapshotCacheService);
    snapshotCache.update(makeHttpServerSnapshot());

    VdrSnapshotReadService snapshotReadService(snapshotAccessService);
    VdrSnapshotReadJsonSerializer snapshotReadJsonSerializer;
    VdrController vdrController(
        vdrOverviewService,
        vdrJsonSerializer,
        snapshotReadService,
        snapshotReadJsonSerializer);

    BackendRegistry backendRegistry;
    BackendNode defaultBackend;
    defaultBackend.backendId = "default";
    defaultBackend.backendName = "Default VDR";
    defaultBackend.backendType = "vdr";
    defaultBackend.enabled = true;
    defaultBackend.online = false;
    backendRegistry.addBackend(defaultBackend);

    BackendRegistryService backendRegistryService(backendRegistry);
    BackendRegistryJsonSerializer backendRegistryJsonSerializer;
    BackendRegistryController backendRegistryController(
        backendRegistryService,
        backendRegistryJsonSerializer);

    VdrCapabilitySet capabilitySet =
        VdrCapabilitySet::snapshotReadOnly();
    CapabilityResolver capabilityResolver(capabilitySet);
    CapabilityReportBuilder capabilityReportBuilder;
    CapabilityReportService capabilityReportService(
        "default",
        capabilityResolver,
        capabilityReportBuilder);
    CapabilityReportJsonSerializer capabilityReportJsonSerializer;
    CapabilityController capabilityController(
        capabilityReportService,
        capabilityReportJsonSerializer);

    RecordingActionValidationService recordingActionValidationService;
    RecordingActionValidationResultJsonSerializer recordingActionValidationJsonSerializer;
    RecordingActionValidationRequestParser recordingActionValidationRequestParser;
    RecordingActionValidationController recordingActionValidationController(
        recordingActionValidationService,
        recordingActionValidationJsonSerializer,
        recordingActionValidationRequestParser);

    RecordingActionExecutionService recordingActionExecutionService;
    RecordingActionExecutionResultJsonSerializer recordingActionExecutionJsonSerializer;
    RecordingActionBackendExecutorAdapterRegistry recordingActionBackendExecutorAdapterRegistry;
    RecordingActionExecutionController recordingActionExecutionController(
        recordingActionExecutionService,
        recordingActionExecutionJsonSerializer,
        recordingActionBackendExecutorAdapterRegistry,
        recordingActionValidationRequestParser);

    RecordingActionRequestPreviewService
        recordingActionRequestPreviewService;

    RecordingActionRequestPreviewResultJsonSerializer
        recordingActionRequestPreviewJsonSerializer;

    RecordingActionPreviewController
        recordingActionPreviewController(
            recordingActionRequestPreviewService,
            recordingActionRequestPreviewJsonSerializer,
            recordingActionValidationRequestParser);

    VdrTimerActionExecutionService vdrTimerActionExecutionService;
    VdrTimerActionResultJsonSerializer vdrTimerActionResultJsonSerializer;
    VdrTimerActionRequestParser vdrTimerActionRequestParser;
    VdrTimerActionController vdrTimerActionController(
        vdrTimerActionExecutionService,
        vdrTimerActionResultJsonSerializer,
        vdrTimerActionRequestParser);
    VdrTimerActionExecutorAdapterRegistry vdrTimerActionExecutorAdapterRegistry;

    RuntimeDiagnosticsService runtimeDiagnosticsService;
    RuntimeDiagnosticsJsonSerializer runtimeJsonSerializer;

    RuntimeMeasurement firstMeasurement;
    firstMeasurement.component = "PollingService";
    firstMeasurement.operation = "Poll cycle";
    firstMeasurement.durationMs = 30;
    firstMeasurement.statusCode = 0;
    firstMeasurement.sizeBytes = 100;
    firstMeasurement.itemCount = 4;
    runtimeDiagnosticsService.recordMeasurement(firstMeasurement);

    RuntimeMeasurement secondMeasurement;
    secondMeasurement.component = "PollingService";
    secondMeasurement.operation = "Poll cycle";
    secondMeasurement.durationMs = 10;
    secondMeasurement.statusCode = 0;
    secondMeasurement.sizeBytes = 200;
    secondMeasurement.itemCount = 12;
    runtimeDiagnosticsService.recordMeasurement(secondMeasurement);

    RuntimeDiagnosticsController runtimeDiagnosticsController(
        runtimeDiagnosticsService,
        runtimeJsonSerializer);

    SnapshotChangeFeed snapshotChangeFeed;
    snapshotChangeFeed.addEntry(SnapshotChangeFeedEntry(3, 1, {"status"}));
    SnapshotChangeFeedJsonSerializer snapshotChangeFeedJsonSerializer;
    SnapshotChangeFeedController snapshotChangeFeedController(
        snapshotChangeFeed,
        snapshotChangeFeedJsonSerializer);

    SseLiveTransport liveTransport;
    LiveTransportController liveTransportController(liveTransport);

    SearchTimerDiscoveryStaticProvider searchTimerDiscoveryProvider;
    SearchTimerDiscoveryService searchTimerDiscoveryService(
        searchTimerDiscoveryProvider);
    SearchTimerDiscoveryJsonSerializer searchTimerDiscoveryJsonSerializer;
    SearchTimerDiscoveryController searchTimerDiscoveryController(
        searchTimerDiscoveryService,
        searchTimerDiscoveryJsonSerializer);

    ApiRouter router(
        dashboardController,
        jobsController,
        recordingsController,
        metadataController,
        vdrController,
        vdrRecordingQueryController,
        snapshotReadService,
        &epgController,
        nullptr,
        nullptr,
        backendRegistryController,
        capabilityController,
        recordingActionValidationController,
        recordingActionExecutionController,
        recordingActionPreviewController,
        vdrTimerActionController,
        vdrTimerActionExecutorAdapterRegistry,
        runtimeDiagnosticsController,
        snapshotChangeFeedController,
        nullptr,
        liveTransportController,
        nullptr,
        nullptr,
        nullptr,
        &searchTimerDiscoveryController);

    PublicApiRuntime::instance().resetOperationLookup();
    PublicApiRuntime::instance().registerOperationLookup(
        [](
            const std::string& operationId,
            const std::string& actorRef)
        {
            PublicOperationLookupResult result;

            if ((operationId != "http-op-1" &&
                 operationId != "op_12345678901234567890123456789012") ||
                actorRef.empty())
            {
                result.status =
                    PublicOperationLookupStatus::notFound;
                return result;
            }

            result.status = PublicOperationLookupStatus::ok;
            result.operation.operationId = operationId;
            result.operation.state = "dispatching";
            result.operation.backendId = "default";
            result.operation.resourceRevision = "3";
            return result;
        });

    setenv(
        "VDR_SUITE_MANAGED_BASIC_USERNAME",
        "phase62-admin",
        1);
    setenv(
        "VDR_SUITE_MANAGED_BASIC_PASSWORD_HASH",
        "$6$testsalt$qzmynZ3SU0S5D.QBAsFplf6HVa.jpeEdx88KlHvhGfddFSPHoEWMArwiVQ1PLzZDrJJ9Vs/zKBgHPMSwmFddx.",
        1);
    setenv(
        "VDR_SUITE_MANAGED_BASIC_PERMISSIONS",
        "*@*",
        1);

    const std::string nativeAccount="native-osd-test-"+std::to_string(::getpid());
    const std::string nativeActor=nativeAccount+"-actor";
    assert(::setenv("VDR_SUITE_LOCAL_OSD_ACCOUNT_ID",nativeAccount.c_str(),1)==0);
    auto eventHub = std::make_shared<DeviceLiveEventHub>();
    TestHttpServer server(router, eventHub);
    // Exercise the actual HTTP server account repository and API router,
    // not a mock native authentication callback.
    HumanAccountRepository localAccounts(db);
    SecurityIdentityProvisioningRepository localIdentities(db);
    SecurityPermissionGrantRepository localGrants(db);
    assert(db.execute("BEGIN IMMEDIATE"));
    assert(localIdentities.ensureHumanCredentialInActiveTransaction(
        nativeActor,"Native OSD fixture",nativeActor+"-credential","human-password"));
    assert(localAccounts.ensureAccountInActiveTransaction(
        nativeAccount,nativeActor,"Native OSD fixture"));
    assert(db.execute("COMMIT"));
    assert(localGrants.ensureGrant(nativeActor,"recordings.view","default"));
    assert(localGrants.ensureGrant(nativeActor,"epg.view","default"));
    assert(localGrants.ensureGrant(nativeActor,"media.recording.play","default"));
    HttpServerRequest localRequest;
    localRequest.method="GET";
    localRequest.path="/api/vdr/recordings/query?backend=default&limit=1&offset=0";
    localRequest.headers["X-Local-OSD-Peer"]="true";
    assert(server.handleRequest(localRequest).statusCode==401);
    localRequest.localOsdPeer=true;
    const auto localResponse=server.handleRequest(localRequest);
    HttpServerRequest ordinaryRequest=localRequest;
    ordinaryRequest.localOsdPeer=false;
    authorize(ordinaryRequest);
    const auto ordinaryResponse=server.handleRequest(ordinaryRequest);
    assert(localResponse.statusCode==200 && ordinaryResponse.statusCode==200);
    assert(localResponse.body==ordinaryResponse.body);
    localRequest.path="/api/vdr/recordings/query?backend=secondary&limit=1";
    assert(server.handleRequest(localRequest).statusCode==403);
    localRequest.method="POST";
    localRequest.path="/api/media/continue-watching";
    localRequest.body=R"({"operation":"list","backendId":"default"})";
    const auto history=server.handleRequest(localRequest);
    // This fixture does not install the ContinueWatching runtime; it must
    // nevertheless reach the same history owner (not an authentication error).
    assert(history.statusCode!=401 && history.statusCode!=403 && history.statusCode!=404);
    localRequest.body=R"({"operation":"clear","backendId":"default"})";
    assert(server.handleRequest(localRequest).statusCode==404);
    localRequest.method="GET";localRequest.body.clear();
    localRequest.path="/api/v1/accounts";
    assert(server.handleRequest(localRequest).statusCode==404);
    const auto existingAccount=localAccounts.findByAccountId(nativeAccount);
    assert(existingAccount.status==HumanAccountRepositoryStatus::ok);
    assert(db.execute("BEGIN IMMEDIATE"));
    assert(localAccounts.setActiveInActiveTransaction(nativeAccount,
        existingAccount.account.revision,false)==HumanAccountRepositoryStatus::ok);
    assert(db.execute("COMMIT"));
    localRequest.path="/api/vdr/recordings/query?backend=default&limit=1";
    assert(server.handleRequest(localRequest).statusCode==401);
    ::unsetenv("VDR_SUITE_LOCAL_OSD_ACCOUNT_ID");

    // Exercise real HTTP dispatch and byte-preserving response mapping with
    // an isolated callback. Provider/filesystem behavior has its own test.
    unsigned pictureReads = 0;
    const std::string jpeg("\xff\xd8\xff\0\x01",5);
    server.setImageGalleryApi({},[&](const RequestSecurityContext& actor,const std::string& path) {
        assert(actor.authenticated());
        ++pictureReads;
        if (path.find("/photo?") != std::string::npos ||
            path.find("/preview?") != std::string::npos)
            return ApiResponse{200,"image/jpeg",jpeg,{{"Cache-Control","no-store"}}};
        return ApiResponse{200,"application/json",R"({"entries":[]})",{}};
    });
    for (const std::string endpoint : {"library","browse","preview","photo"}) {
        HttpServerRequest picture;
        picture.method = "GET";
        picture.path = "/api/v1/addons/image/" + endpoint + "?backendId=default";
        if (endpoint == "library") picture.path += "&sample=1";
        else picture.path += "&sourceId=pictures&path=Urlaub.jpg";
        const auto before = pictureReads;
        assert(server.handleRequest(picture).statusCode == 401);
        assert(pictureReads == before);
        authorize(picture);
        const auto allowed = server.handleRequest(picture);
        assert(allowed.statusCode == 200 && pictureReads == before + 1);
        if (endpoint == "preview" || endpoint == "photo") {
            assert(allowed.headers.at("Content-Type") == "image/jpeg");
            assert(allowed.headers.at("Cache-Control") == "no-store");
            assert(allowed.body == jpeg);
        }
        picture.path += "&backendId=default";
        assert(server.handleRequest(picture).statusCode == 400);
        assert(pictureReads == before + 1);
    }

    // Verify the productive browser route and existing admission gate.
    HttpServerRequest unauthenticatedLive;
    unauthenticatedLive.method = "GET";
    unauthenticatedLive.path = "/api/vdr/live";
    const auto rejectedLive = server.handleRequest(unauthenticatedLive);
    assert(rejectedLive.statusCode != 200);
    assert(!rejectedLive.browserEventSubscription);

    liveTransport.publish(LiveUpdateEvent(1, 1, {"recordings"}, "default"));
    liveTransport.publish(LiveUpdateEvent(2, 2, {"timers"}, "secondary"));
    HttpServerRequest authenticatedLive;
    authenticatedLive.method = "GET";
    authenticatedLive.path = "/api/vdr/live";
    authorize(authenticatedLive);
    authenticatedLive.headers["last-event-id"] =
        liveTransport.daemonEpoch() + ":1";
    const auto replay = server.handleRequest(authenticatedLive);
    assert(replay.statusCode == 200);
    assert(replay.headers.at("Content-Type") == "text/event-stream");
    assert(replay.body.empty());
    assert(replay.browserEventSubscription);
    std::string browserFrame;
    assert(replay.browserEventSubscription->next(
        browserFrame, std::chrono::milliseconds(10)));
    assert(browserFrame.find("\"backendId\":\"secondary\"") != std::string::npos);
    assert(browserFrame.find("id: " + liveTransport.daemonEpoch() + ":1\n") ==
           std::string::npos);
    assert(browserFrame.find("id: " + liveTransport.daemonEpoch() + ":2\n") !=
           std::string::npos);
    liveTransport.publish(LiveUpdateEvent(3, 3, {"continueWatching"}, "default"));
    assert(replay.browserEventSubscription->next(
        browserFrame, std::chrono::milliseconds(10)));
    assert(browserFrame.find("\"changedDomains\":[\"continueWatching\"]") !=
           std::string::npos);
    authenticatedLive.headers["last-event-id"] = "invalid";
    const auto invalidLive = server.handleRequest(authenticatedLive);
    assert(invalidLive.browserEventSubscription);
    assert(invalidLive.browserEventSubscription->next(
        browserFrame, std::chrono::milliseconds(10)));
    assert(browserFrame.find("event: resync") != std::string::npos);
    replay.browserEventSubscription->close();
    invalidLive.browserEventSubscription->close();

    // Public-v1 M2: HTTP route must reject duplicate/encoded-alias EPG
    // identities rather than accepting RestQueryParameters' last value.
    // All examples retain the same authorized backend and valid image IDs.
    for (const std::string& target : {
        std::string("/api/v1/epg/artwork?backendId=default&channelId=S19.2E-1-100&eventId=42&channelId=other"),
        std::string("/api/v1/epg/preferred-artwork?backendId=default&channelId=S19.2E-1-100&eventId=42&eventId=43"),
        std::string("/api/v1/epg/preferred-artwork?backendId=default&channelId=S19.2E-1-100&eventId=42&%65ventId=43"),
        std::string("/api/v1/epg/artwork?backendId=default&channelId=S19.2E-1-100&eventId=42&unknown=1"),
        std::string("/api/v1/epg/artwork?backendId=default&channelId=S19.2E-1-100&eventId=42&variant=other"),
        std::string("/api/v1/epg/preferred-artwork?backendId=default&channelId=S19.2E-1-100&eventId=42&variant=home&variant=home")
    })
    {
        HttpServerRequest artworkRequest;
        artworkRequest.method = "GET";
        artworkRequest.path = target;
        authorize(artworkRequest);
        const HttpServerResponse artworkResponse =
            server.handleRequest(artworkRequest);
        assertJsonResponse(artworkResponse, 400);
        assert(artworkResponse.body.find("invalid_epg_artwork_scope") !=
            std::string::npos);
    }

    for (const auto* route : {"/api/v1/epg/artwork", "/api/v1/epg/preferred-artwork"})
    {
        HttpServerRequest artworkRequest;
        artworkRequest.method = "GET";
        artworkRequest.path = std::string(route) +
            "?backendId=default&channelId=S19.2E-1-100&eventId=42";
        authorize(artworkRequest);
        const auto original = server.handleRequest(artworkRequest);
        artworkRequest.path += "&variant=home";
        const auto previewSource = server.handleRequest(artworkRequest);
        assert(original.statusCode != 400);
        assert(previewSource.statusCode == original.statusCode);
        assert(previewSource.body == original.body);
    }

    // The Genre variant is valid only on preferred-artwork and reuses
    // the direct image owner if no portrait is materialized in the index.
    HttpServerRequest genreArtworkRequest;
    genreArtworkRequest.method = "GET";
    genreArtworkRequest.path = "/api/v1/epg/artwork?backendId=default&channelId=S19.2E-1-100&eventId=42";
    authorize(genreArtworkRequest);
    const auto directGenreFallback = server.handleRequest(genreArtworkRequest);
    genreArtworkRequest.path = "/api/v1/epg/preferred-artwork?backendId=default&channelId=S19.2E-1-100&eventId=42&variant=genre";
    const auto selectedGenreArtwork = server.handleRequest(genreArtworkRequest);
    assert(selectedGenreArtwork.statusCode == directGenreFallback.statusCode);
    assert(selectedGenreArtwork.body == directGenreFallback.body);
    genreArtworkRequest.path = "/api/v1/epg/artwork?backendId=default&channelId=S19.2E-1-100&eventId=42&variant=genre";
    assertJsonResponse(server.handleRequest(genreArtworkRequest), 400);

    HttpServerRequest anonymousFrontendRequest;
    anonymousFrontendRequest.method = "GET";
    anonymousFrontendRequest.path = "/";
    const HttpServerResponse anonymousFrontendResponse =
        server.handleRequest(anonymousFrontendRequest);
    assert(anonymousFrontendResponse.statusCode == 200);
    assert(anonymousFrontendResponse.headers.at("Content-Type") ==
        "text/html; charset=utf-8");
    assert(anonymousFrontendResponse.headers.find("WWW-Authenticate") ==
        anonymousFrontendResponse.headers.end());
    assert(anonymousFrontendResponse.body.find("<!doctype html>") !=
        std::string::npos);

    HttpServerRequest anonymousFrontendAssetRequest;
    anonymousFrontendAssetRequest.method = "GET";
    anonymousFrontendAssetRequest.path = "/frontend/style.css";
    const HttpServerResponse anonymousFrontendAssetResponse =
        server.handleRequest(anonymousFrontendAssetRequest);
    assert(anonymousFrontendAssetResponse.statusCode == 200);
    assert(anonymousFrontendAssetResponse.headers.at("Content-Type") ==
        "text/css; charset=utf-8");
    assert(anonymousFrontendAssetResponse.headers.find("WWW-Authenticate") ==
        anonymousFrontendAssetResponse.headers.end());

    assert(anonymousFrontendResponse.body.find("../frontend/manifest.webmanifest") != std::string::npos);
    assert(anonymousFrontendResponse.body.find("../frontend/pwa-register.js") != std::string::npos);
    anonymousFrontendAssetRequest.path = "/frontend/manifest.webmanifest";
    const auto manifestResponse = server.handleRequest(anonymousFrontendAssetRequest);
    assert(manifestResponse.statusCode == 200);
    assert(manifestResponse.headers.at("Content-Type") == "application/manifest+json; charset=utf-8");
    assert(manifestResponse.body.find("\"start_url\": \"./\"") != std::string::npos);
    assert(anonymousFrontendResponse.body.find("vdr-suite-pwa-install-button") != std::string::npos);
    for (const auto* iconPath : {"/frontend/pwa-icon-192.png", "/frontend/pwa-icon-512.png"})
    {
        anonymousFrontendAssetRequest.path = iconPath;
        const auto icon = server.handleRequest(anonymousFrontendAssetRequest);
        assert(icon.statusCode == 200);
        assert(icon.headers.at("Content-Type") == "image/png");
        assert(icon.body.size() > 24 && icon.body.compare(0, 8, "\x89PNG\r\n\x1a\n", 8) == 0);
        assert(manifestResponse.body.find(std::string(iconPath).substr(10)) != std::string::npos);
    }
    anonymousFrontendAssetRequest.path = "/frontend/service-worker.js";
    const auto workerResponse = server.handleRequest(anonymousFrontendAssetRequest);
    assert(workerResponse.statusCode == 200);
    assert(workerResponse.headers.at("Content-Type") == "application/javascript; charset=utf-8");
    assert(workerResponse.headers.at("Cache-Control") == "no-cache");
    assert(workerResponse.body.find("vdr-suite-pwa-shell:") != std::string::npos);
    anonymousFrontendAssetRequest.path = "/frontend/pwa-register.js";
    const auto registrationResponse = server.handleRequest(anonymousFrontendAssetRequest);
    assert(registrationResponse.statusCode == 200);
    assert(registrationResponse.body.find("serviceWorker.register") != std::string::npos);

    assert(anonymousFrontendResponse.body.find("../frontend/platform/home-persistence.js") != std::string::npos);
    anonymousFrontendAssetRequest.path = "/frontend/platform/home-persistence.js";
    const auto persistenceResponse = server.handleRequest(anonymousFrontendAssetRequest);
    assert(persistenceResponse.statusCode == 200);
    assert(persistenceResponse.headers.at("Content-Type") == "application/javascript; charset=utf-8");
    assert(persistenceResponse.body.find("VdrSuiteHomePersistence") != std::string::npos);
    anonymousFrontendAssetRequest.path = "/frontend/platform/home-cache-runtime.js";
    const auto homeCacheResponse = server.handleRequest(anonymousFrontendAssetRequest);
    assert(homeCacheResponse.statusCode == 200);
    assert(homeCacheResponse.body.find("VdrSuiteHomeCache") != std::string::npos);

    HttpServerRequest anonymousApiRequest;
    anonymousApiRequest.method = "GET";
    anonymousApiRequest.path = "/api/backends";
    const HttpServerResponse anonymousApiResponse =
        server.handleRequest(anonymousApiRequest);
    assertJsonResponse(anonymousApiResponse, 401);
    assert(anonymousApiResponse.headers.find("WWW-Authenticate") ==
        anonymousApiResponse.headers.end());
    assert(anonymousApiResponse.body.find(
        "\"code\":\"authentication_required\"") !=
        std::string::npos);

    HttpServerRequest anonymousOperationRequest;
    anonymousOperationRequest.method = "GET";
    anonymousOperationRequest.path =
        "/api/v1/operations/http-op-1";
    const HttpServerResponse anonymousOperationResponse =
        server.handleRequest(anonymousOperationRequest);
    assert(anonymousOperationResponse.statusCode == 401);
    assert(
        anonymousOperationResponse.headers.at("Content-Type") ==
        "application/problem+json");
    assert(anonymousOperationResponse.body.find(
        "\"code\":\"unauthorized\"") !=
        std::string::npos);

    HttpServerRequest retiredLegacyOperationRequest;
    retiredLegacyOperationRequest.method = "GET";
    retiredLegacyOperationRequest.path =
        "/api/v1/operations/http-op-1";
    authorizeRetiredLegacy(retiredLegacyOperationRequest);
    const HttpServerResponse retiredLegacyOperationResponse =
        server.handleRequest(retiredLegacyOperationRequest);
    assert(retiredLegacyOperationResponse.statusCode == 401);

    HttpServerRequest operationRequest;
    operationRequest.method = "GET";
    operationRequest.path =
        "/api/v1/operations/http-op-1";
    authorize(operationRequest);
    const HttpServerResponse operationResponse =
        server.handleRequest(operationRequest);
    assert(operationResponse.statusCode == 200);
    assert(
        operationResponse.headers.at("Content-Type") ==
        "application/json; charset=utf-8");
    assert(operationResponse.headers.find("ETag") !=
        operationResponse.headers.end());
    assert(operationResponse.body.find(
        "\"operationId\":\"http-op-1\"") !=
        std::string::npos);
    assert(operationResponse.body.find(
        "\"state\":\"dispatching\"") !=
        std::string::npos);

    HttpServerRequest conditionalOperationRequest =
        operationRequest;
    conditionalOperationRequest.headers["if-none-match"] =
        operationResponse.headers.at("ETag");
    const HttpServerResponse conditionalOperationResponse =
        server.handleRequest(
            conditionalOperationRequest);
    assert(conditionalOperationResponse.statusCode == 304);
    assert(conditionalOperationResponse.body.empty());
    assert(conditionalOperationResponse.headers.at("ETag") ==
        operationResponse.headers.at("ETag"));

    // SSE for an Operation is authorized via its existing actor-owned GET.
    const std::string eventPath =
        "/api/v1/operations/op_12345678901234567890123456789012/events";
    HttpServerRequest anonymousEventRequest;
    anonymousEventRequest.method = "GET";
    anonymousEventRequest.path = eventPath;
    const auto anonymousEvent = server.handleRequest(anonymousEventRequest);
    assert(anonymousEvent.statusCode == 401 &&
           !anonymousEvent.deviceEventSubscription);
    HttpServerRequest foreignEvent = operationRequest;
    foreignEvent.path = "/api/v1/operations/op_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/events";
    assert(server.handleRequest(foreignEvent).statusCode == 404);
    HttpServerRequest permittedEvent = operationRequest;
    permittedEvent.path = eventPath;
    const auto stream = server.handleRequest(permittedEvent);
    assert(stream.statusCode == 200);
    assert(stream.headers.at("Content-Type") == "text/event-stream");
    assert(stream.deviceEventSubscription);
    std::string eventFrame;
    assert(stream.deviceEventSubscription->next(
        eventFrame, std::chrono::milliseconds(10)));
    assert(eventFrame.find("event: hello") != std::string::npos);
    eventHub->publishOperation("op_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    assert(stream.deviceEventSubscription->next(
        eventFrame, std::chrono::milliseconds(10)));
    assert(eventFrame.empty());
    eventHub->publishOperation("op_12345678901234567890123456789012");
    assert(stream.deviceEventSubscription->next(
        eventFrame, std::chrono::milliseconds(10)));
    assert(eventFrame.find("event: operation") != std::string::npos);
    assert(eventFrame.find("op_12345678901234567890123456789012") !=
           std::string::npos);
    stream.deviceEventSubscription->close();

    HttpServerRequest dashboardRequest;
    dashboardRequest.method = "GET";
    dashboardRequest.path = "/api/dashboard";
    authorize(dashboardRequest);
    HttpServerResponse dashboardResponse =
        server.handleRequest(dashboardRequest);
    assertJsonResponse(dashboardResponse, 200);
    assert(dashboardResponse.body.find("\"jobs\"") != std::string::npos);
    assert(dashboardResponse.body.find("\"recordings\"") != std::string::npos);

    HttpServerRequest vdrStatusRequest;
    vdrStatusRequest.method = "GET";
    vdrStatusRequest.path = "/api/vdr/status";
    authorize(vdrStatusRequest);
    HttpServerResponse vdrStatusResponse =
        server.handleRequest(vdrStatusRequest);
    assertJsonResponse(vdrStatusResponse, 200);
    assert(vdrStatusResponse.body.find("\"mode\":\"http-server-snapshot\"") != std::string::npos);
    assert(vdrStatusResponse.body.find("\"host\":\"http-server-host\"") != std::string::npos);

    HttpServerRequest backendHealthRequest;
    backendHealthRequest.method = "GET";
    backendHealthRequest.path = "/api/backends/default/health";
    authorize(backendHealthRequest);
    HttpServerResponse backendHealthResponse =
        server.handleRequest(backendHealthRequest);
    assertJsonResponse(backendHealthResponse, 200);
    assert(backendHealthResponse.body.find("\"backendId\":\"default\"") != std::string::npos);
    assert(backendHealthResponse.body.find("\"snapshotAvailable\":true") != std::string::npos);

    HttpServerRequest runtimeSummaryRequest;
    runtimeSummaryRequest.method = "GET";
    runtimeSummaryRequest.path = "/api/runtime/summary";
    authorize(runtimeSummaryRequest);
    HttpServerResponse runtimeSummaryResponse =
        server.handleRequest(runtimeSummaryRequest);
    assertJsonResponse(runtimeSummaryResponse, 200);
    assert(runtimeSummaryResponse.body.find("\"summaries\"") != std::string::npos);
    assert(runtimeSummaryResponse.body.find("\"count\":2") != std::string::npos);

    HttpServerRequest dashboardQueryRequest;
    dashboardQueryRequest.method = "GET";
    dashboardQueryRequest.path = "/api/dashboard?ignored=true";
    authorize(dashboardQueryRequest);
    HttpServerResponse dashboardQueryResponse =
        server.handleRequest(dashboardQueryRequest);
    assertJsonResponse(dashboardQueryResponse, 200);
    assert(dashboardQueryResponse.body.find("\"jobs\"") != std::string::npos);

    HttpServerRequest epgQueryRequest;
    epgQueryRequest.method = "GET";
    epgQueryRequest.path = "/api/epg/now-next?channelId=1&from=123";
    authorize(epgQueryRequest);
    HttpServerResponse epgQueryResponse =
        server.handleRequest(epgQueryRequest);
    assertJsonResponse(epgQueryResponse, 200);
    assert(epgQueryResponse.body.find("\"events\"") != std::string::npos);

    HttpServerRequest discoveryRequest;
    discoveryRequest.method = "GET";
    discoveryRequest.path = "/api/searchtimers/discovery?backend=http-server";
    authorize(discoveryRequest);
    HttpServerResponse discoveryResponse =
        server.handleRequest(discoveryRequest);
    assertEmptyDiscoveryResponse(
        discoveryResponse,
        "http-server");

    HttpServerRequest vdrDiscoveryDefaultRequest;
    vdrDiscoveryDefaultRequest.method = "GET";
    vdrDiscoveryDefaultRequest.path = "/api/vdr/searchtimers/discovery";
    authorize(vdrDiscoveryDefaultRequest);
    HttpServerResponse vdrDiscoveryDefaultResponse =
        server.handleRequest(vdrDiscoveryDefaultRequest);
    assertEmptyDiscoveryResponse(
        vdrDiscoveryDefaultResponse,
        "default");

    HttpServerRequest vdrDiscoveryBackendRequest;
    vdrDiscoveryBackendRequest.method = "GET";
    vdrDiscoveryBackendRequest.path = "/api/vdr/searchtimers/discovery?backend=ferienhaus";
    authorize(vdrDiscoveryBackendRequest);
    HttpServerResponse vdrDiscoveryBackendResponse =
        server.handleRequest(vdrDiscoveryBackendRequest);
    assertEmptyDiscoveryResponse(
        vdrDiscoveryBackendResponse,
        "ferienhaus");

    HttpServerRequest discoveryPostRequest;
    discoveryPostRequest.method = "POST";
    discoveryPostRequest.path = "/api/searchtimers/discovery";
    discoveryPostRequest.body = "{\"ignored\":true}";
    authorize(discoveryPostRequest);
    HttpServerResponse discoveryPostResponse =
        server.handleRequest(discoveryPostRequest);
    assertJsonResponse(discoveryPostResponse, 503);
    assert(discoveryPostResponse.body.find(
        "security_policy_not_migrated") != std::string::npos);

    HttpServerRequest missingRequest;
    missingRequest.method = "GET";
    missingRequest.path = "/api/missing";
    authorize(missingRequest);
    HttpServerResponse missingResponse =
        server.handleRequest(missingRequest);
    assertJsonResponse(missingResponse, 404);
    assert(missingResponse.body == "{\"error\":\"not found\"}");

    HttpServerRequest missingLogoRequest;
    missingLogoRequest.method = "GET";
    missingLogoRequest.path = "/channel-logos/definitely-missing-logo.png";
    HttpServerResponse missingLogoResponse =
        server.handleRequest(missingLogoRequest);
    assert(missingLogoResponse.statusCode == 204);
    assert(missingLogoResponse.headers.at("Content-Type") == "image/png");
    assert(missingLogoResponse.headers.at("Cache-Control") == "public, max-age=300");
    assert(missingLogoResponse.headers.find("WWW-Authenticate") ==
        missingLogoResponse.headers.end());
    assert(missingLogoResponse.body.empty());

    // Exercise the actual anonymous asset handler and configured-root priority.
    char logoTemp[] = "/tmp/vdr-suite-channel-logos-XXXXXX";
    char* logoBase = mkdtemp(logoTemp);
    assert(logoBase != nullptr);
    const std::string configuredRoot = std::string(logoBase) + "/root";
    const std::string configuredCache = std::string(logoBase) + "/cache";
    assert(::mkdir(configuredRoot.c_str(), 0700) == 0);
    assert(::mkdir(configuredCache.c_str(), 0700) == 0);
    const std::string pngName = "/das erste hd.png";
    {
        std::ofstream png(configuredRoot + pngName, std::ios::binary);
        png << "configured-root-logo";
        assert(png.good());
    }
    {
        std::ofstream png(configuredCache + pngName, std::ios::binary);
        png << "cache-logo";
        assert(png.good());
    }
    {
        std::ofstream svg(configuredRoot + "/zdf_neo.svg");
        svg << "<svg xmlns=\"http://www.w3.org/2000/svg\"/>";
        assert(svg.good());
    }
    assert(setenv("VDR_SUITE_CHANNEL_LOGO_ROOT", configuredRoot.c_str(), 1) == 0);
    assert(setenv("VDR_SUITE_CHANNEL_LOGO_CACHE", configuredCache.c_str(), 1) == 0);

    HttpServerRequest foundLogo;
    foundLogo.method = "GET";
    foundLogo.path = "/channel-logos/das%20erste%20hd.png";
    const auto foundLogoResponse = server.handleRequest(foundLogo);
    assert(foundLogoResponse.statusCode == 200);
    assert(foundLogoResponse.body == "configured-root-logo");
    assert(foundLogoResponse.headers.at("Content-Type") == "image/png");
    assert(foundLogoResponse.headers.at("Cache-Control") == "public, max-age=3600");

    foundLogo.path = "/channel-logos/zdf_neo.svg";
    const auto svgLogoResponse = server.handleRequest(foundLogo);
    assert(svgLogoResponse.statusCode == 200);
    assert(svgLogoResponse.headers.at("Content-Type") == "image/svg+xml");
    assert(svgLogoResponse.headers.at("Cache-Control") == "public, max-age=3600");

    foundLogo.path = "/channel-logos/%2e%2e/hidden.png";
    assert(server.handleRequest(foundLogo).statusCode == 404);
    foundLogo.path = "/channel-logos/unknown-extension.jpg";
    assert(server.handleRequest(foundLogo).statusCode == 404);

    unsetenv("VDR_SUITE_CHANNEL_LOGO_ROOT");
    unsetenv("VDR_SUITE_CHANNEL_LOGO_CACHE");
    assert(std::remove((configuredRoot + pngName).c_str()) == 0);
    assert(std::remove((configuredCache + pngName).c_str()) == 0);
    assert(std::remove((configuredRoot + "/zdf_neo.svg").c_str()) == 0);
    assert(::rmdir(configuredRoot.c_str()) == 0);
    assert(::rmdir(configuredCache.c_str()) == 0);
    assert(::rmdir(logoBase) == 0);

    HttpServerRequest postRequest;
    postRequest.method = "POST";
    postRequest.path = "/api/dashboard";
    postRequest.body = "{\"ignored\":true}";
    authorize(postRequest);
    HttpServerResponse postResponse =
        server.handleRequest(postRequest);
    assertJsonResponse(postResponse, 503);
    assert(postResponse.body.find(
        "security_policy_not_migrated") != std::string::npos);

    HttpServerRequest validationRequest;
    validationRequest.method = "POST";
    validationRequest.path = "/api/recordings/actions/validate";
    validationRequest.body =
        "{"
        "\"backendId\":\"default\","
        "\"recordingId\":\"http-recording-1\","
        "\"action\":\"MOVE\","
        "\"dryRun\":true,"
        "\"targetPath\":\"/srv/vdr/video/archive\""
        "}";
    authorize(validationRequest);
    HttpServerResponse validationResponse =
        server.handleRequest(validationRequest);
    assertJsonResponse(validationResponse, 200);
    assert(validationResponse.body.find("\"valid\":true") != std::string::npos);
    assert(validationResponse.body.find("\"dryRun\":true") != std::string::npos);
    assert(validationResponse.body.find("\"wouldCreateJob\":false") != std::string::npos);
    assert(validationResponse.body.find("\"recordingId\":\"http-recording-1\"") != std::string::npos);
    assert(validationResponse.body.find("\"requiredCapabilities\":[\"recording.action.move\"]")
           != std::string::npos);
    assert(validationResponse.body.find("\"warnings\":[\"dry-run only\"]")
           != std::string::npos);

    HttpServerRequest vdrValidationRequest;
    vdrValidationRequest.method = "POST";
    vdrValidationRequest.path = "/api/vdr/recordings/actions/validate";
    vdrValidationRequest.body =
        "{"
        "\"backendId\":\"default\","
        "\"recordingId\":\"http-recording-2\","
        "\"action\":\"DELETE\","
        "\"dryRun\":true"
        "}";
    authorize(vdrValidationRequest);
    HttpServerResponse vdrValidationResponse =
        server.handleRequest(vdrValidationRequest);
    assertJsonResponse(vdrValidationResponse, 200);
    assert(vdrValidationResponse.body.find("\"valid\":true") != std::string::npos);
    assert(vdrValidationResponse.body.find("\"recordingId\":\"http-recording-2\"") != std::string::npos);
    assert(vdrValidationResponse.body.find("\"requiredCapabilities\":[\"recording.action.delete\"]")
           != std::string::npos);

    HttpServerRequest unsupportedMethodRequest;
    unsupportedMethodRequest.method = "PUT";
    unsupportedMethodRequest.path = "/api/dashboard";
    authorize(unsupportedMethodRequest);
    HttpServerResponse unsupportedMethodResponse =
        server.handleRequest(unsupportedMethodRequest);
    assertJsonResponse(unsupportedMethodResponse, 405);
    assert(unsupportedMethodResponse.body == "{\"error\":\"method not allowed\"}");

    PublicApiRuntime::instance().resetOperationLookup();

    unsetenv("VDR_SUITE_MANAGED_BASIC_USERNAME");
    unsetenv("VDR_SUITE_MANAGED_BASIC_PASSWORD_HASH");
    unsetenv("VDR_SUITE_MANAGED_BASIC_PERMISSIONS");

    db.close();

    std::cout
        << "test_test_http_server passed"
        << std::endl;

    return 0;
}
