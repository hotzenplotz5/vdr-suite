#include "BackendAgentMediaImportCatalog.h"
#include <cassert>
#include <string>
#include <vector>
int main()
{
    BackendAgentMediaImportCatalog catalog;
    RequestSecurityContext actor;
    actor.authenticationState = AuthenticationState::Authenticated;
    actor.actor = {"actor-agent", ActorType::Agent, "Agent", true};
    BackendAgentRecord live;
    live.backendId = "backend";
    live.agentId = "agent";
    live.actorId = "actor-agent";
    live.agentInstanceId = "instance";
    live.backendGeneration = 7;
    live.leaseExpiresAt = 300;
    std::vector<std::string> items;
    assert(!catalog.list(live, "backend", 100, items));
    const std::string approvedSha(64, 'a');
    assert(catalog.publish(actor, live, "backend", "instance", 7,
        {"source_00000000002", "source_00000000001"}, 100,
        "manual", "0.1.0", approvedSha));
    assert(catalog.list(live, "backend", 100, items));
    assert(items == (std::vector<std::string>{"source_00000000001",
                                             "source_00000000002"}));
    // Advisory names follow their IDs even when Agent order is reversed.
    // An invalid label must not poison the previous good observation.
    std::vector<std::string> labels;
    assert(!catalog.publish(actor, live, "backend", "instance", 7,
        {"source_00000000002", "source_00000000001"}, 100,
        "manual", "0.1.0", approvedSha, {"wrong/count"}));
    assert(!catalog.publish(actor, live, "backend", "instance", 7,
        {"source_00000000002", "source_00000000001"}, 100,
        "manual", "0.1.0", approvedSha, {"Only one"}));
    assert(catalog.publish(actor, live, "backend", "instance", 7,
        {"source_00000000002", "source_00000000001"}, 100,
        "manual", "0.1.0", approvedSha, {"Zweiter Film", "Erster Film"}));
    assert(catalog.list(live, "backend", 100, items, labels));
    assert(items == (std::vector<std::string>{"source_00000000001",
                                             "source_00000000002"}));
    assert(labels == (std::vector<std::string>{"Erster Film", "Zweiter Film"}));
    // UX3: advisory root-relative folders follow sorted source IDs, never
    // act as source authority, and invalid paths keep the old observation.
    std::vector<std::string> folders;
    assert(catalog.publish(actor, live, "backend", "instance", 7,
        {"source_00000000002", "source_00000000001"}, 100,
        "manual", "0.1.0", approvedSha,
        {"Zweiter Film", "Erster Film"}, false,
        {"Serien/Andor", ""}));
    assert(catalog.list(live, "backend", 100, items, labels,
                        nullptr, &folders));
    assert(folders == (std::vector<std::string>{"", "Serien/Andor"}));
    assert(!catalog.publish(actor, live, "backend", "instance", 7,
        {"source_00000000002", "source_00000000001"}, 100,
        "manual", "0.1.0", approvedSha,
        {"Zweiter Film", "Erster Film"}, false,
        {"../outside", ""}));
    assert(!catalog.publish(actor, live, "backend", "instance", 7,
        {"source_00000000002", "source_00000000001"}, 100,
        "manual", "0.1.0", approvedSha,
        {"Zweiter Film", "Erster Film"}, false,
        {"/etc", ""}));
    assert(catalog.list(live, "backend", 100, items, labels,
                        nullptr, &folders));
    assert(folders == (std::vector<std::string>{"", "Serien/Andor"}));
    // Backend-scoped Agent observation can carry more than the old 32
    // sources while retaining the 100-film maximum.
    std::vector<std::string> bulkIds, bulkLabels;
    for (int i = 0; i < 70; ++i) {
        bulkIds.push_back("source_" + std::to_string(10000000000ULL + i));
        bulkLabels.push_back("Bulk Film " + std::to_string(i));
    }
    assert(catalog.publish(actor, live, "backend", "instance", 7,
        bulkIds, 100, "manual", "0.1.0", approvedSha, bulkLabels));
    assert(catalog.list(live, "backend", 100, items, labels));
    assert(items.size() == 70 && labels.size() == 70);
    assert(items.back() == bulkIds.back());
    bool partial = false;
    assert(catalog.list(live, "backend", 100, items, labels, &partial));
    assert(partial); // conservative default for older Agent observations
    assert(catalog.publish(actor, live, "backend", "instance", 7,
        std::vector<std::string>(bulkIds.begin(), bulkIds.end()), 100,
        "manual", "0.1.0", approvedSha, bulkLabels, false));
    assert(catalog.list(live, "backend", 100, items, labels, &partial));
    assert(!partial && items.size() == 70);
    std::vector<std::string> exact100 = bulkIds;
    for (int i = 70; i < 100; ++i)
        exact100.push_back("source_" + std::to_string(10000000000ULL + i));
    assert(catalog.publish(actor, live, "backend", "instance", 7,
        exact100, 100, "manual", "0.1.0", approvedSha));
    assert(catalog.list(live, "backend", 100, items) && items.size() == 100);
    bulkIds.resize(101, "source_oversized_0000000001");
    assert(!catalog.publish(actor, live, "backend", "instance", 7,
        bulkIds, 100, "manual", "0.1.0", approvedSha));
    assert(catalog.list(live, "backend", 100, items) && items.size() == 100);
    // UX4.2: only a selected, verified opaque source is requested.
    // The same Agent lease must deliver the result. No paths or operations.
    BackendAgentMediaImportCatalog::Preview observed;
    const std::string selected = exact100.front();
    assert(catalog.requestPreview(live, "backend", 100, selected, observed) ==
        BackendAgentMediaImportCatalog::PreviewState::pending);
    assert(catalog.nextPreviewRequest(live, "backend", 100) == selected);
    assert(catalog.nextPreviewRequest(live, "other-backend", 100).empty());
    // Filmwechsel while an earlier Agent FFprobe is in flight.
    const auto newer = exact100[1];
    assert(catalog.requestPreview(live, "backend", 100, newer, observed) ==
        BackendAgentMediaImportCatalog::PreviewState::pending);
    assert(catalog.nextPreviewRequest(live, "backend", 100) == newer);
    BackendAgentMediaImportCatalog::Preview stale{
        selected, "ready", "Alter Film", "Alt/Film", "h264", 1000,
        8000000000ULL, 5000000000ULL, false};
    assert(!catalog.acceptPreview(live, "backend", 100, stale));
    assert(catalog.nextPreviewRequest(live, "backend", 100) == newer);
    assert(catalog.requestPreview(live, "backend", 100, selected, observed) ==
        BackendAgentMediaImportCatalog::PreviewState::pending);
    BackendAgentMediaImportCatalog::Preview report;
    report.sourceId = selected;
    report.status = "ready";
    report.title = "Film";
    report.targetFolder = "Serien/Andor/Film";
    report.videoCodec = "h264";
    report.sourceBytes = 1000;
    report.nfoOutline = "Eine kurze NFO";
    report.nfoPlot = "Handlung & Details";
    report.nfoYear = "2024";
    report.nfoGenre = "Drama";
    report.durationSeconds = 3120;
    report.posterDataUri = "data:image/jpeg;base64,/9j/2Q==";
    report.fanartDataUri = "data:image/png;base64,iVBORw0KGgo=";
    report.availableBytes = 8000000000ULL;
    report.requiredBytes = 5000000000ULL;
    assert(!catalog.acceptPreview(live, "backend", 100,
        BackendAgentMediaImportCatalog::Preview{
          selected, "ready", "Film", "../outside", "h264", 1000, 8000000000ULL,
          5000000000ULL, false}));
    assert(!catalog.acceptPreview(live, "backend", 100,
        BackendAgentMediaImportCatalog::Preview{
          "source_00000000002", "ready", "Film", "Film", "h264",
          1000, 8000000000ULL, 5000000000ULL, false}));
    auto invalidArtwork = report;
    invalidArtwork.posterDataUri = "data:text/html;base64,PHNjcmlwdD4=";
    assert(!catalog.acceptPreview(live, "backend", 100, invalidArtwork));
    invalidArtwork = report;
    invalidArtwork.fanartDataUri = "data:image/png;base64,%%%";
    assert(!catalog.acceptPreview(live, "backend", 100, invalidArtwork));
    assert(catalog.acceptPreview(live, "backend", 100, report));
    assert(catalog.nextPreviewRequest(live, "backend", 100).empty());
    assert(catalog.requestPreview(live, "backend", 100, selected, observed) ==
        BackendAgentMediaImportCatalog::PreviewState::ready);
    assert(observed.targetFolder == "Serien/Andor/Film" &&
           observed.videoCodec == "h264" && observed.sourceBytes == 1000);
    assert(observed.posterDataUri == "data:image/jpeg;base64,/9j/2Q==" &&
           observed.fanartDataUri == "data:image/png;base64,iVBORw0KGgo=");
    assert(observed.nfoPlot == "Handlung & Details" &&
           observed.nfoYear == "2024" && observed.nfoGenre == "Drama" &&
           observed.durationSeconds == 3120);
    auto invalidYear = report;
    invalidYear.nfoYear = "20xx";
    assert(!catalog.acceptPreview(live, "backend", 100, invalidYear));
    assert(catalog.requestPreview(
        live, "backend", 100, "source_no_such_123456", observed) ==
        BackendAgentMediaImportCatalog::PreviewState::denied);
    assert(catalog.requestPreview(live, "backend", 135, selected, observed) ==
        BackendAgentMediaImportCatalog::PreviewState::pending);
    assert(catalog.nextPreviewRequest(live, "backend", 135) == selected);
    // The same pending request cannot outlive the authenticated Agent lease.
    live.leaseExpiresAt = 135;
    assert(catalog.requestPreview(live, "backend", 136, selected, observed) ==
        BackendAgentMediaImportCatalog::PreviewState::denied);
    assert(catalog.nextPreviewRequest(live, "backend", 136).empty());
    assert(!catalog.acceptPreview(live, "backend", 136, report));
    live.leaseExpiresAt = 300;
    // Repeated reads keep the pending ID; the reply never starts a job.
    assert(catalog.requestPreview(live, "backend", 137, selected, observed) ==
        BackendAgentMediaImportCatalog::PreviewState::pending);
    report.status = "insufficient";
    report.videoCodec = "vp9";
    report.videoReencodeRequired = true;
    assert(catalog.acceptPreview(live, "backend", 138, report));
    assert(catalog.requestPreview(live, "backend", 138, selected, observed) ==
        BackendAgentMediaImportCatalog::PreviewState::ready);
    assert(observed.status == "insufficient" && observed.videoCodec == "vp9" &&
           observed.videoReencodeRequired);
    // A confirmed VDR recording can be classified without probing the
    // original codec. Core/Agent still validates the marker before execution.
    assert(catalog.requestPreview(live, "backend", 139, newer, observed) ==
        BackendAgentMediaImportCatalog::PreviewState::pending);
    const BackendAgentMediaImportCatalog::Preview duplicate{
        newer, "duplicate", "Film", "Serien/Andor/Film", "",
        1000, 0, 0, false};
    assert(catalog.acceptPreview(live, "backend", 139, duplicate));
    assert(catalog.requestPreview(live, "backend", 139, newer, observed) ==
        BackendAgentMediaImportCatalog::PreviewState::ready);
    assert(observed.status == "duplicate" && observed.videoCodec.empty());
    const auto diagnosticSource = exact100[2];
    assert(catalog.requestPreview(live, "backend", 140, diagnosticSource, observed) ==
        BackendAgentMediaImportCatalog::PreviewState::pending);
    auto unavailable = report;
    unavailable.sourceId = diagnosticSource;
    unavailable.status = "unavailable";
    unavailable.videoCodec.clear();
    unavailable.failureCode = "ffprobe_failed";
    assert(catalog.acceptPreview(live, "backend", 140, unavailable));
    assert(catalog.requestPreview(live, "backend", 140, diagnosticSource, observed) ==
        BackendAgentMediaImportCatalog::PreviewState::ready);
    assert(observed.status == "unavailable" &&
           observed.failureCode == "ffprobe_failed");
    std::string origin, version, sha256;
    assert(catalog.observedInstallation(
        live, "backend", 100, origin, version, sha256));
    assert(origin == "manual" && version == "0.1.0" && sha256 == approvedSha);
    using namespace vdrsuite::addons;
    RootApprovalResult coreApproval{
        RootApprovalStatus::approved, InstallOrigin::manual, "0.1.0", approvedSha};
    assert(catalog.matchesIndependentCoreApproval(
        live, "backend", 100, coreApproval));
    coreApproval.status = RootApprovalStatus::revoked;
    assert(!catalog.matchesIndependentCoreApproval(
        live, "backend", 100, coreApproval));
    coreApproval.status = RootApprovalStatus::approved;
    coreApproval.sha256 = std::string(64, 'b');
    assert(!catalog.matchesIndependentCoreApproval(
        live, "backend", 100, coreApproval));
    coreApproval.sha256 = approvedSha;
    coreApproval.origin = InstallOrigin::debian;
    assert(!catalog.matchesIndependentCoreApproval(
        live, "backend", 100, coreApproval));
    coreApproval.origin = InstallOrigin::manual;
    coreApproval.version = "0.2.0";
    assert(!catalog.matchesIndependentCoreApproval(
        live, "backend", 100, coreApproval));
    coreApproval.version = "0.1.0";
    assert(!catalog.matchesIndependentCoreApproval(
        live, "other-backend", 100, coreApproval));
    assert(!catalog.matchesIndependentCoreApproval(
        live, "backend", 191, coreApproval));
    assert(!catalog.observedInstallation(
        live, "backend", 191, origin, version, sha256));
    assert(!catalog.publish(actor, live, "backend", "instance", 7,
        {}, 120, "manual", "0.1.0", "invalid-sha256"));
    assert(!catalog.publish(actor, live, "backend", "instance", 7,
        {}, 120, "other", "0.1.0", approvedSha));
    assert(!catalog.list(live, "backend", 191, items));
    assert(items.empty());
    assert(!catalog.publish(actor, live, "backend", "instance", 7,
        {"source_00000000001", "source_00000000001"}, 120));
    assert(!catalog.publish(actor, live, "backend", "instance", 7,
        {"/etc/passwd"}, 120));
    actor.actor.actorId = "spoofed";
    assert(!catalog.publish(actor, live, "backend", "instance", 7,
        {"source_00000000001"}, 120));
    actor.actor.actorId = "actor-agent";
    live.agentInstanceId = "restarted";
    assert(!catalog.list(live, "backend", 140, items));
    assert(!catalog.matchesIndependentCoreApproval(
        live, "backend", 140, coreApproval));
    assert(catalog.publish(actor, live, "backend", "restarted", 7,
        {"source_00000000003"}, 140));
    live.backendGeneration = 8;
    assert(!catalog.list(live, "backend", 145, items));
    assert(!catalog.matchesIndependentCoreApproval(
        live, "backend", 145, coreApproval));
    assert(!catalog.publish(actor, live, "backend", "restarted", 7,
        {"source_00000000003"}, 145));
    assert(catalog.publish(actor, live, "backend", "restarted", 8,
        {}, 145));
    assert(catalog.list(live, "backend", 145, items) && items.empty());
    live.revoked = true;
    assert(!catalog.list(live, "backend", 147, items));
    live.revoked = false;
    live.leaseExpiresAt = 146;
    assert(!catalog.list(live, "backend", 147, items));
    return 0;
}
