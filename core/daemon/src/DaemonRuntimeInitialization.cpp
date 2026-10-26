#include "DaemonRuntime.h"

#include "DaemonRuntimeRecordingCut.h"
#include "EpgTimerIntentAdmissionRepository.h"
#include "EpgTimerIntentAtomicAdmissionService.h"
#include "EpgNativeTimerClockAttestation.h"
#include "EpgNativeTimerPreviewAttestation.h"
#include "SuiteBridgeNativeTimerCreateTransport.h"
#include "TimerIntentApplicationService.h"

#include "BackendAgentNativeTimerDeleteFingerprint.h"
#include "NativeTimerDeleteDispatchService.h"
#include "NativeTimerModifyDispatchService.h"
#include "NativeTimerModifyReadbackVerificationService.h"
#include "NativeTimerModifyOperationCompletionService.h"
#include "BackendAgentNativeTimerModifyAssignment.h"
#include "NativeTimerDeleteOperationPreparationService.h"
#include "NativeTimerModifyOperationPreparationService.h"
#include "NativeTimerDeletePublicPreflight.h"
#include "NativeTimerPublicDetailRevision.h"
#include "MutationOperationIdentity.h"
#include "NativeTimerAbsenceReadbackVerificationService.h"
#include "NativeTimerDeleteOperationCompletionService.h"
#include "BackendAgentNativeTimerDeleteAssignment.h"
#include "EpgSearchNativeFuzzyStartupRestoreDiagnostics.h"
#include "ManualRecordingMetadataApiRuntime.h"
#include "RecordingArtworkHttpServer.h"
#include "VdrRecordingArtworkIdentity.h"
#include "PublicResourcePreconditions.h"
#include "PublicRecordingCollectionProjection.h"
#include "PublicRecordingMetadataDetailProjection.h"
#include "PublicRecordingPlaybackTargetResolver.h"
#include "PublicRecordingActionReadback.h"
#include "RestfulApiRecordingMapper.h"
#include "HttpRequest.h"
#include "PublicRecordingDisplayName.h"
#include "PublicSearchProjection.h"
#include "GenreBrowserApiRuntime.h"
#include "GlobalSearchApiRuntime.h"
#include "PublicChannelNeighbor.h"
#include "RecordingSeriesHierarchyApiRuntime.h"
#include "RestfulApiRecordingActionBackendExecutorAdapter.h"
#include "SuiteBridgeSvdrpTransport.h"
#include "SuiteBridgeVideoDirectoryDiscovery.h"
#include "RestfulApiSearchTimerDiscoveryProvider.h"
#include "SearchTimerPreviewService.h"
#include "SearchTimerCreateAdmissionService.h"
#include "SearchTimerCreateDispatchService.h"
#include "SearchTimerNativeCasAdmissionService.h"
#include "SearchTimerNativeCasDispatchService.h"
#include "SearchTimerNativeCasLocalProvider.h"
#include "SearchTimerCreateDeviceAdmissionService.h"
#include "SearchTimerDiscoveryStaticProvider.h"
#include "SimpleHttpListener.h"
#include "TestHttpServer.h"
#include "VdrRecordingCacheRepository.h"
#include "VdrRecordingNativePersonSearchService.h"
#include "NativeTimerPublicDetailRevision.h"
#include "NativeTimerBindingRepository.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <csignal>
#include <iostream>
#include <iterator>
#include <utility>
#include <vector>

namespace
{
bool searchCasReplayMatches(
    const std::string& canonical,
    const std::string& action,
    const std::string& backendId,
    const std::string& publicId,
    const std::string& revision,
    const std::string& query)
{
    constexpr const char* prefix = "searchtimer-native-cas-evidence/1|";
    std::size_t pos = std::char_traits<char>::length(prefix);
    if (canonical.compare(0U, pos, prefix) != 0) return false;
    std::array<std::string, 7> fields;
    for (auto& field : fields)
    {
        if (pos >= canonical.size()) return false;
        std::size_t size = 0U;
        const auto start = pos;
        while (pos < canonical.size() && canonical[pos] >= '0' &&
               canonical[pos] <= '9')
        {
            const std::size_t digit =
                static_cast<std::size_t>(canonical[pos++] - '0');
            if (size > (16384U - digit) / 10U) return false;
            size = size * 10U + digit;
        }
        if (pos == start || pos >= canonical.size() ||
            canonical[pos++] != ':' || size >= canonical.size() - pos)
            return false;
        field = canonical.substr(pos, size);
        pos += size;
        if (canonical[pos++] != '|') return false;
    }
    if (pos != canonical.size() || fields[0] != action ||
        fields[1] != backendId || fields[2] != publicId ||
        fields[5] != revision)
        return false;
    if (action != "update") return fields[6].empty() && query.empty();
    const auto first = fields[6].find(':');
    if (first == std::string::npos || fields[6].substr(0U, first) != fields[3])
        return false;
    const auto second = fields[6].find(':', first + 1U);
    return second != std::string::npos &&
        fields[6].substr(first + 1U, second - first - 1U) == query;
}

class SearchTimerCurrentBackendGeneration final :
    public ISearchTimerBackendGenerationAuthority
{
public:
    explicit SearchTimerCurrentBackendGeneration(
        BackendAgentRepository& agents)
        : agents_(agents) {}

    std::optional<std::uint64_t> currentGeneration(
        const std::string& backendId) const override
    {
        // A slow inventory/readback can outlive the lease at HTTP admission.
        // Recheck current wall time at EVERY dispatch generation probe.
        const auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        const auto agent = agents_.findAgentForBackend(backendId);
        if (!agent || agent->revoked || agent->incompatible ||
            agent->backendGeneration == 0U ||
            agent->leaseExpiresAt <= now)
            return std::nullopt;
        return agent->backendGeneration;
    }
private:
    BackendAgentRepository& agents_;
};
} // namespace

bool DaemonRuntime::initialize()
{
    std::cout << "vdr-suite-daemon runtime initializing" << std::endl;
    std::cout << "database path: " << config_.databasePath() << std::endl;

    if (!database_.open(config_.databasePath())) {
        std::cerr << "failed to open database" << std::endl;
        return false;
    }

    std::cout << "database opened" << std::endl;

    mutationOperationRepository_ =
        std::make_unique<vdrsuite::operations::MutationOperationRepository>(
            database_);
    if (!mutationOperationRepository_->ensureSchema())
    {
        std::cerr
            << "failed to initialize mutation operation schema"
            << std::endl;
        return false;
    }
    mutationOperationReadService_ =
        std::make_unique<vdrsuite::operations::MutationOperationReadService>(
            *mutationOperationRepository_);

    std::cout
        << "mutation operation read runtime initialized"
        << std::endl;

    timerIntentRepository_ =
        std::make_unique<vdrsuite::timers::TimerIntentRepository>(
            database_);
    if (!timerIntentRepository_->ensureSchema())
    {
        std::cerr
            << "failed to initialize TimerIntent schema"
            << std::endl;
        return false;
    }

    timerAssignmentRepository_ =
        std::make_unique<vdrsuite::timers::TimerAssignmentRepository>(
            database_);
    if (!timerAssignmentRepository_->ensureSchema())
    {
        std::cerr
            << "failed to initialize TimerAssignment schema"
            << std::endl;
        return false;
    }
    nativeTimerBindingRepository_ =
        std::make_unique<vdrsuite::timers::NativeTimerBindingRepository>(
            database_);
    if (!nativeTimerBindingRepository_->ensureSchema())
    {
        std::cerr
            << "failed to initialize NativeTimerBinding schema"
            << std::endl;
        return false;
    }

    timerAssignmentReadService_ =
        std::make_unique<vdrsuite::timers::TimerAssignmentReadService>(
            *timerAssignmentRepository_);
    timerAssignmentFulfillmentService_ =
        std::make_unique<vdrsuite::timers::TimerAssignmentFulfillmentService>(
            *timerAssignmentRepository_,
            *nativeTimerBindingRepository_);

    nativeTimerCreateOperationPreparationService_ =
        std::make_unique<
            vdrsuite::timers::NativeTimerCreateOperationPreparationService>(
                *timerIntentRepository_,
                *timerAssignmentRepository_,
                *mutationOperationRepository_);
    nativeTimerCreateAdmissionService_ =
        std::make_unique<
            vdrsuite::timers::NativeTimerCreateAdmissionService>(
                database_,
                *timerAssignmentRepository_,
                *mutationOperationRepository_,
                *timerAssignmentFulfillmentService_,
                *nativeTimerCreateOperationPreparationService_);
    nativeTimerCreateDispatchService_ =
        std::make_unique<vdrsuite::timers::NativeTimerCreateDispatchService>(
            *mutationOperationRepository_);
    nativeTimerDeleteDispatchService_ =
        std::make_unique<vdrsuite::timers::NativeTimerDeleteDispatchService>(
            *mutationOperationRepository_, *nativeTimerBindingRepository_);
    nativeTimerModifyDispatchService_ =
        std::make_unique<vdrsuite::timers::NativeTimerModifyDispatchService>(
            *mutationOperationRepository_, *nativeTimerBindingRepository_);
    nativeTimerModifyReadbackVerificationService_ =
        std::make_unique<vdrsuite::timers::NativeTimerModifyReadbackVerificationService>(
            *nativeTimerBindingRepository_);
    nativeTimerModifyOperationCompletionService_ =
        std::make_unique<vdrsuite::timers::NativeTimerModifyOperationCompletionService>(
            *mutationOperationRepository_, *nativeTimerBindingRepository_);
    nativeTimerAbsenceReadbackVerificationService_ =
        std::make_unique<vdrsuite::timers::NativeTimerAbsenceReadbackVerificationService>(
            *nativeTimerBindingRepository_);
    nativeTimerDeleteOperationCompletionService_ =
        std::make_unique<vdrsuite::timers::NativeTimerDeleteOperationCompletionService>(
            *mutationOperationRepository_, *nativeTimerBindingRepository_);
    nativeTimerCreateReadbackVerificationService_ =
        std::make_unique<
            vdrsuite::timers::NativeTimerCreateReadbackVerificationService>(
                *nativeTimerBindingRepository_);
    nativeTimerCreateOperationCompletionService_ =
        std::make_unique<
            vdrsuite::timers::NativeTimerCreateOperationCompletionService>(
                *mutationOperationRepository_,
                *timerAssignmentRepository_,
                *nativeTimerBindingRepository_);

    std::cout
        << "TimerAssignment read, fulfillment, atomic native Timer CREATE admission, preparation and reconciliation runtime initialized"
        << std::endl;

    if (!RecordingSeriesHierarchyApiRuntime::instance().configured() &&
        !RecordingSeriesHierarchyApiRuntime::instance().configure(database_))
    {
        std::cerr
            << "failed to initialize recording Series hierarchy override runtime"
            << std::endl;
        return false;
    }

    std::cout
        << "recording Series hierarchy override runtime initialized"
        << std::endl;


    jobRepository_ = std::make_unique<JobRepository>(database_);
    recordingRepository_ = std::make_unique<RecordingRepository>(database_);
    publicRecordingIdentities_ =
        std::make_unique<PublicRecordingIdentityRepository>(database_);
    if (!publicRecordingIdentities_->ensureSchema())
    {
        std::cerr << "failed to initialize public Recording identities"
                  << std::endl;
        return false;
    }
    publicSearchTimerIdentities_ =
        std::make_unique<PublicSearchTimerIdentityRepository>(database_);
    if (!publicSearchTimerIdentities_->ensureSchema())
    {
        std::cerr << "failed to initialize public SearchTimer identities"
                  << std::endl;
        return false;
    }
    metadataRepository_ = std::make_unique<MetadataRepository>(database_);
    jobDashboardService_ = std::make_unique<JobDashboardService>(*jobRepository_);
    recordingDashboardService_ = std::make_unique<RecordingDashboardService>(*recordingRepository_, *metadataRepository_);
    dashboardFacade_ = std::make_unique<DashboardFacade>(*jobDashboardService_, *recordingDashboardService_);
    dashboardJsonSerializer_ = std::make_unique<DashboardJsonSerializer>();
    dashboardController_ = std::make_unique<DashboardController>(*dashboardFacade_, *dashboardJsonSerializer_);
    jobsController_ = std::make_unique<JobsController>(*jobRepository_);
    recordingsController_ = std::make_unique<RecordingsController>(*recordingRepository_);
    metadataController_ = std::make_unique<MetadataController>(*metadataRepository_);

    std::cout << "REST controller runtime initialized" << std::endl;

    BackendNode defaultBackend;
    defaultBackend.backendId = "default";
    defaultBackend.backendName = "Default VDR";
    defaultBackend.backendType = "restfulapi";
    defaultBackend.connection.enabled = true;
    defaultBackend.connection.mode = config_.vdrMode();
    defaultBackend.connection.host = config_.vdrHost();
    defaultBackend.connection.port = config_.vdrPort();
    defaultBackend.enabled = true;
    defaultBackend.online = true;
    defaultBackend.capabilities = VdrCapabilitySet::snapshotReadOnly();

    backendRegistry_.addBackend(defaultBackend);
    backendRegistryService_ = std::make_unique<BackendRegistryService>(backendRegistry_);
    backendAccessPolicy_ = std::make_unique<BackendAccessPolicy>();
    backendRegistryJsonSerializer_ = std::make_unique<BackendRegistryJsonSerializer>();
    backendRegistryController_ = std::make_unique<BackendRegistryController>(*backendRegistryService_, *backendRegistryJsonSerializer_);

    backendAgentIdentityRepository_ =
        std::make_unique<SecurityIdentityRepository>(database_);
    backendAgentProvisioningRepository_ =
        std::make_unique<SecurityIdentityProvisioningRepository>(database_);
    backendAgentCredentialVerifierRepository_ =
        std::make_unique<CredentialVerifierRepository>(database_);
    backendAgentAccountabilityRepository_ =
        std::make_unique<AccountabilityEventRepository>(database_);
    backendAgentRepository_ =
        std::make_unique<BackendAgentRepository>(database_);
    backendAgentCommandRepository_ =
        std::make_unique<BackendAgentCommandRepository>(database_);
    backendAgentCommandReservationRepository_ =
        std::make_unique<BackendAgentCommandReservationRepository>(database_);

    if (!backendAgentIdentityRepository_->ensureSchema() ||
        !backendAgentCredentialVerifierRepository_->ensureSchema() ||
        !backendAgentAccountabilityRepository_->ensureSchema() ||
        !backendAgentRepository_->ensureSchema() ||
        !backendAgentCommandRepository_->ensureSchema() ||
        !backendAgentCommandReservationRepository_->ensureSchema()) {
        std::cerr << "failed to initialize Backend Agent control-plane schema" << std::endl;
        return false;
    }

    humanAccountRepository_ =
        std::make_unique<HumanAccountRepository>(database_);
    if (!humanAccountRepository_->ensureSchema())
    {
        std::cerr
            << "failed to initialize Human Account schema"
            << std::endl;
        return false;
    }
    firstAdminBootstrapRepository_ =
        std::make_unique<FirstAdminBootstrapRepository>(database_);
    if (!firstAdminBootstrapRepository_->ensureSchema())
    {
        std::cerr
            << "failed to initialize first-admin bootstrap schema"
            << std::endl;
        return false;
    }

    humanAccountReadService_ =
        std::make_unique<HumanAccountReadService>(
            *humanAccountRepository_);

    std::cout
        << "Human Account read and first-admin bootstrap persistence "
           "runtime initialized"
        << std::endl;

    backendAgentNativeTimerDeleteAssignmentService_ =
        std::make_unique<vdrsuite::agent::BackendAgentNativeTimerDeleteAssignmentService>(
            *backendAgentCommandRepository_, *backendAgentRepository_);
    backendAgentNativeTimerModifyAssignmentService_ =
        std::make_unique<vdrsuite::agent::BackendAgentNativeTimerModifyAssignmentService>(
            *backendAgentCommandRepository_, *backendAgentRepository_);
    backendAgentNativeTimerCreateReservationService_ =
        std::make_unique<
            vdrsuite::agent::BackendAgentNativeTimerCreateReservationService>(
                *backendAgentCommandRepository_,
                *backendAgentCommandReservationRepository_,
                *backendAgentRepository_);
    backendAgentNativeTimerCreateActivationService_ =
        std::make_unique<
            vdrsuite::agent::BackendAgentNativeTimerCreateActivationService>(
                *mutationOperationRepository_,
                *backendAgentCommandReservationRepository_,
                *backendAgentCommandRepository_);

    backendAgentLifecycleService_ =
        std::make_unique<BackendAgentLifecycleService>(
            database_,
            *backendAgentRepository_,
            *backendRegistryService_,
            *backendAgentProvisioningRepository_,
            *backendAgentIdentityRepository_,
            *backendAgentCredentialVerifierRepository_,
            *backendAgentAccountabilityRepository_);
    embeddedBackendLifecycleService_ =
        std::make_unique<EmbeddedBackendLifecycleService>(database_);
    if (!embeddedBackendLifecycleService_->ensureSchema()) {
        std::cerr << "failed to initialize embedded backend lifecycle schema"
                  << std::endl;
        return false;
    }
    backendAgentCommandDeliveryService_ =
        std::make_unique<BackendAgentCommandDeliveryService>(
            *backendAgentCommandRepository_,
            *backendAgentRepository_,
            *backendAgentAccountabilityRepository_);
    std::cout << "Backend Agent enrollment, lease and command delivery runtime initialized" << std::endl;

    epgSearchNativeFuzzyCapabilityRepository_ =
        std::make_unique<EpgSearchNativeFuzzyCapabilityRepository>(
            database_);
    epgSearchNativeFuzzyCapabilityDetector_ =
        std::make_unique<EpgSearchNativeFuzzyCapabilityDetector>();
    epgSearchNativeFuzzyCapabilityFreshnessPolicy_ =
        std::make_unique<EpgSearchNativeFuzzyCapabilityFreshnessPolicy>();
    epgSearchNativeFuzzyStartupRestoreService_ =
        std::make_unique<EpgSearchNativeFuzzyStartupRestoreService>(
            *epgSearchNativeFuzzyCapabilityRepository_,
            *epgSearchNativeFuzzyCapabilityDetector_,
            *backendRegistryService_,
            *epgSearchNativeFuzzyCapabilityFreshnessPolicy_);

    const auto nativeFuzzyStartupRestoreSummary =
        epgSearchNativeFuzzyStartupRestoreService_->restoreAllBackends();
    const auto nativeFuzzyStartupRestoreDiagnostics =
        EpgSearchNativeFuzzyStartupRestoreDiagnostics::fromSummary(
            nativeFuzzyStartupRestoreSummary);

    runtimeDiagnosticsService_.recordMeasurement(
        nativeFuzzyStartupRestoreDiagnostics.toRuntimeMeasurement());

    if (nativeFuzzyStartupRestoreDiagnostics.schemaReady) {
        std::cout
            << "EPGSearch native fuzzy persisted capability restore: "
            << "status=" << nativeFuzzyStartupRestoreDiagnostics.status()
            << ", reason=\"" << nativeFuzzyStartupRestoreDiagnostics.reason() << "\""
            << ", backends=" << nativeFuzzyStartupRestoreDiagnostics.backendsSeen
            << ", persisted=" << nativeFuzzyStartupRestoreDiagnostics.persistedResultsFound
            << ", updated=" << nativeFuzzyStartupRestoreDiagnostics.backendsUpdated
            << ", nativeAvailable=" << nativeFuzzyStartupRestoreDiagnostics.nativeFuzzyAvailable
            << ", nativeUnavailable=" << nativeFuzzyStartupRestoreDiagnostics.nativeFuzzyUnavailable
            << ", staleIgnored=" << nativeFuzzyStartupRestoreDiagnostics.staleResultsIgnored
            << std::endl;
    }
    else {
        std::cerr
            << "EPGSearch native fuzzy persisted capability restore skipped: "
            << nativeFuzzyStartupRestoreDiagnostics.reason()
            << std::endl;
    }
    const auto runtimeBackends =
        backendRegistry_.listBackends();

    if (runtimeBackends.empty()) {
        std::cout << "no VDR backends configured" << std::endl;
    }

    snapshotCache_ = std::make_unique<SnapshotCache>();
    snapshotCacheService_ = std::make_unique<SnapshotCacheService>(*snapshotCache_);
    snapshotAccessService_ = std::make_unique<SnapshotAccessService>(*snapshotCacheService_);
    vdrSnapshotReadService_ = std::make_unique<VdrSnapshotReadService>(*snapshotAccessService_);
    vdrSnapshotReadJsonSerializer_ = std::make_unique<VdrSnapshotReadJsonSerializer>();

    backendPollingCoordinator_ = std::make_unique<BackendPollingCoordinator>();
    vdrTimerActionExecutorAdapterRegistry_ = std::make_unique<VdrTimerActionExecutorAdapterRegistry>();
    searchTimerPreviewEpgCache_ = std::make_unique<SearchTimerPreviewEpgCache>();
    searchTimerPreviewEpgCacheRefreshServiceRegistry_ =
        std::make_unique<SearchTimerPreviewEpgCacheRefreshServiceRegistry>();
    epgEventRepository_ = std::make_unique<EpgEventRepository>(database_);

    if (!epgEventRepository_->ensureSchema()) {
        std::cerr << "failed to initialize EPG cache repository schema" << std::endl;
        return false;
    }

    epgArtworkRepository_ = std::make_unique<EpgArtworkRepository>(database_);

    if (!epgArtworkRepository_->ensureSchema()) {
        std::cerr << "failed to initialize EPG artwork repository schema" << std::endl;
        return false;
    }

    epgArtworkPublicJsonSerializer_ =
        std::make_unique<EpgArtworkPublicJsonSerializer>();

    vdrRecordingCacheRepository_ = std::make_unique<VdrRecordingCacheRepository>(database_);

    if (!vdrRecordingCacheRepository_->ensureSchema()) {
        std::cerr << "failed to initialize VDR recording cache repository schema" << std::endl;
        return false;
    }

    epgCacheServiceRegistry_ = std::make_unique<EpgCacheServiceRegistry>();

    for (const BackendNode& runtimeBackend : runtimeBackends) {
        auto backendRuntimeContext =
            createBackendRuntimeContext(runtimeBackend);

        if (backendRuntimeContext->searchTimerPreviewEpgCacheRefreshService) {
            searchTimerPreviewEpgCacheRefreshServiceRegistry_->registerService(
                backendRuntimeContext->backendId,
                *backendRuntimeContext->searchTimerPreviewEpgCacheRefreshService);
        }

        if (backendRuntimeContext->epgCacheService) {
            epgCacheServiceRegistry_->registerService(
                backendRuntimeContext->backendId,
                *backendRuntimeContext->epgCacheService);
        }

        backendPollingCoordinator_->addPollingService(
            backendRuntimeContext->backendId,
            *backendRuntimeContext->pollingService);

        backendRuntimeContexts_.push_back(
            std::move(backendRuntimeContext));
    }

    snapshotChangeFeed_ = std::make_unique<SnapshotChangeFeed>();
    snapshotChangeFeedService_ = std::make_unique<SnapshotChangeFeedService>();
    snapshotChangeFeedJsonSerializer_ = std::make_unique<SnapshotChangeFeedJsonSerializer>();

    if (!backendRuntimeContexts_.empty()) {
        pollVdrAndUpdateChangeFeed();
    }

    std::cout << "VDR snapshot runtime initialized" << std::endl;

    vdrOverviewService_ = std::make_unique<VdrOverviewService>(*snapshotAccessService_);
    vdrOverviewJsonSerializer_ = std::make_unique<VdrOverviewJsonSerializer>();
    vdrController_ = std::make_unique<VdrController>(
        *vdrOverviewService_,
        *vdrOverviewJsonSerializer_,
        *vdrSnapshotReadService_,
        *vdrSnapshotReadJsonSerializer_,
        backendRuntimeContexts_.empty()
            ? nullptr
            : backendRuntimeContexts_.front()->service.get());

    if (!backendRuntimeContexts_.empty()) {
        vdrRecordingQueryService_ = std::make_unique<VdrRecordingQueryService>(
            *backendRuntimeContexts_.front()->service,
            vdrRecordingCacheRepository_.get(),
            backendRuntimeContexts_.front()->backendId);
    }
    else {
        std::cerr << "failed to initialize VDR recording query controller: no VDR backend configured" << std::endl;
        return false;
    }

    vdrRecordingQueryResultJsonSerializer_ = std::make_unique<VdrRecordingQueryResultJsonSerializer>();
    vdrRecordingQueryController_ = std::make_unique<VdrRecordingQueryController>(
        *vdrRecordingQueryService_,
        *vdrRecordingQueryResultJsonSerializer_,
        [this](const std::string& backendId)
        {
            return metadataController_
                ->findManualRecordingMetadataForBackend(
                    backendId);
        });
    vdrRecordingFolderController_ =
        std::make_unique<VdrRecordingFolderController>(
            *vdrRecordingCacheRepository_,
            [this](
                const std::string& backendId,
                const std::string& backendNativeId)
            {
                const std::string normalizedBackendId =
                    backendId.empty()
                        ? "default"
                        : backendId;

                for (const auto& backendRuntimeContext :
                     backendRuntimeContexts_)
                {
                    if (!backendRuntimeContext ||
                        backendRuntimeContext->backendId !=
                            normalizedBackendId ||
                        !backendRuntimeContext
                             ->recordingMetadataRepository)
                    {
                        continue;
                    }

                    return backendRuntimeContext
                        ->recordingMetadataRepository
                        ->findByBackendNativeId(
                            normalizedBackendId,
                            backendNativeId);
                }

                return VdrRecordingNativeMetadataRecord{};
            });

    capabilityResolver_ =
        std::make_unique<BackendRegistryCapabilityResolver>(
            *backendRegistryService_,
            "default");
    capabilityReportBuilder_ = std::make_unique<CapabilityReportBuilder>();
    capabilityReportService_ = std::make_unique<CapabilityReportService>(
        "default",
        *capabilityResolver_,
        *capabilityReportBuilder_);
    capabilityReportJsonSerializer_ = std::make_unique<CapabilityReportJsonSerializer>();
    capabilityController_ = std::make_unique<CapabilityController>(
        *capabilityReportService_,
        *capabilityReportJsonSerializer_);

    std::cout << "capability controller runtime initialized" << std::endl;

    if (!backendRuntimeContexts_.empty()) {
        epgQueryService_ = std::make_unique<EpgQueryService>(
            *backendRuntimeContexts_.front()->service);
        epgSearchService_ = std::make_unique<EpgSearchService>();
        epgSearchResultJsonSerializer_ = std::make_unique<EpgSearchResultJsonSerializer>();
        epgController_ = std::make_unique<EpgController>(
            *epgQueryService_,
            *epgSearchService_,
            *epgSearchResultJsonSerializer_);

        std::cout << "EPG controller runtime initialized" << std::endl;
    }
    else {
        std::cout << "EPG controller runtime skipped: no VDR backend configured" << std::endl;
    }

    std::cout << "VDR controller runtime initialized" << std::endl;

    searchTimerService_ = std::make_unique<SearchTimerService>();
    searchTimerResultJsonSerializer_ = std::make_unique<SearchTimerResultJsonSerializer>();

    if (!backendRuntimeContexts_.empty()
        && backendRuntimeContexts_.front()->searchTimerAdapter) {
        searchTimerCreateService_ =
            std::make_unique<SearchTimerCreateService>();
        searchTimerCreateResultJsonSerializer_ =
            std::make_unique<SearchTimerCreateResultJsonSerializer>();
        searchTimerCreateRequestParser_ =
            std::make_unique<SearchTimerCreateRequestParser>();
        searchTimerUpdateService_ =
            std::make_unique<SearchTimerUpdateService>();
        searchTimerUpdateResultJsonSerializer_ =
            std::make_unique<SearchTimerUpdateResultJsonSerializer>();
        searchTimerUpdateRequestParser_ =
            std::make_unique<SearchTimerUpdateRequestParser>();
        searchTimerDeleteService_ =
            std::make_unique<SearchTimerDeleteService>();
        searchTimerDeleteResultJsonSerializer_ =
            std::make_unique<SearchTimerDeleteResultJsonSerializer>();
        searchTimerDeleteRequestParser_ =
            std::make_unique<SearchTimerDeleteRequestParser>();

        searchTimerCommandExecutor_ =
            std::make_unique<RestfulApiSearchTimerCommandExecutor>(
                *backendRuntimeContexts_.front()->httpClient);
        searchTimerRuntimeMutationPolicyExecutor_ =
            std::make_unique<SearchTimerRuntimeMutationPolicyExecutor>(
                *searchTimerCommandExecutor_,
                false);

        searchTimerController_ = std::make_unique<SearchTimerController>(
            *searchTimerService_,
            *searchTimerResultJsonSerializer_,
            *backendRuntimeContexts_.front()->searchTimerAdapter,
            *searchTimerCreateService_,
            *searchTimerCreateResultJsonSerializer_,
            *searchTimerCreateRequestParser_,
            searchTimerUpdateService_.get(),
            searchTimerUpdateResultJsonSerializer_.get(),
            searchTimerUpdateRequestParser_.get(),
            searchTimerDeleteService_.get(),
            searchTimerDeleteResultJsonSerializer_.get(),
            searchTimerDeleteRequestParser_.get());

        std::cout << "SearchTimer controller runtime initialized" << std::endl;
        std::cout << "SearchTimer command executor runtime initialized" << std::endl;
        std::cout << "SearchTimer runtime mutation policy executor initialized closed" << std::endl;
    }
    else {
        std::cout << "SearchTimer controller runtime skipped: no VDR backend configured" << std::endl;
    }

    if (!backendRuntimeContexts_.empty()
        && backendRuntimeContexts_.front()->httpClient) {
        searchTimerDiscoveryProvider_ =
            std::make_unique<RestfulApiSearchTimerDiscoveryProvider>(
                backendRuntimeContexts_.front()->backendId,
                *backendRuntimeContexts_.front()->httpClient,
                "");
        std::cout << "SearchTimer discovery provider runtime initialized: restfulapi" << std::endl;
    }
    else {
        searchTimerDiscoveryProvider_ =
            std::make_unique<SearchTimerDiscoveryStaticProvider>();
        std::cout << "SearchTimer discovery provider runtime initialized: static fallback" << std::endl;
    }

    searchTimerDiscoveryService_ =
        std::make_unique<SearchTimerDiscoveryService>(
            *searchTimerDiscoveryProvider_);
    searchTimerDiscoveryJsonSerializer_ =
        std::make_unique<SearchTimerDiscoveryJsonSerializer>();
    searchTimerDiscoveryController_ =
        std::make_unique<SearchTimerDiscoveryController>(
            *searchTimerDiscoveryService_,
            *searchTimerDiscoveryJsonSerializer_);

    std::cout << "SearchTimer discovery controller runtime initialized" << std::endl;

    searchTimerAutomationReadOnlyService_ =
        std::make_unique<SearchTimerAutomationReadOnlyService>();
    searchTimerAutomationDryRunResultJsonSerializer_ =
        std::make_unique<SearchTimerAutomationDryRunResultJsonSerializer>();
    searchTimerAutomationPreviewController_ =
        std::make_unique<SearchTimerAutomationPreviewController>(
            *searchTimerAutomationReadOnlyService_,
            *searchTimerAutomationDryRunResultJsonSerializer_);

    std::cout << "SearchTimer automation preview controller runtime initialized" << std::endl;

    searchTimerPreviewEpgCacheRefreshController_ =
        std::make_unique<SearchTimerPreviewEpgCacheRefreshController>(
            *searchTimerPreviewEpgCacheRefreshServiceRegistry_);

    std::cout << "SearchTimer preview EPG cache refresh controller runtime initialized" << std::endl;

    epgCacheController_ = std::make_unique<EpgCacheController>(
        *epgCacheServiceRegistry_,
        *epgArtworkRepository_,
        *epgArtworkPublicJsonSerializer_);

    for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
        if (backendRuntimeContext &&
            backendRuntimeContext->epgScraperMetadataResolver) {
            epgCacheController_->registerScraperMetadataResolver(
                backendRuntimeContext->backendId,
                *backendRuntimeContext->epgScraperMetadataResolver);
        }
    }

    std::cout << "EPG cache controller runtime initialized" << std::endl;

    personResolutionJsonSerializer_ = std::make_unique<PersonResolutionJsonSerializer>();
    personSearchService_ = std::make_unique<PersonSearchService>();
    personQueryResultJsonSerializer_ = std::make_unique<PersonQueryResultJsonSerializer>();
    personController_ = std::make_unique<PersonController>(
        *personResolutionJsonSerializer_,
        *personSearchService_,
        *personQueryResultJsonSerializer_);

    std::cout << "person controller runtime initialized" << std::endl;

    recordingPersonSearchService_ =
        std::make_unique<RecordingPersonSearchService>();

    recordingPersonSearchResultJsonSerializer_ =
        std::make_unique<
            RecordingPersonSearchResultJsonSerializer>();

    std::vector<std::pair<
        std::string,
        VdrRecordingNativeMetadataRepository*>>
        recordingPersonMetadataRepositories;

    for (const auto& backendRuntimeContext :
         backendRuntimeContexts_)
    {
        if (!backendRuntimeContext ||
            !backendRuntimeContext
                 ->recordingMetadataRepository)
        {
            continue;
        }

        recordingPersonMetadataRepositories.emplace_back(
            backendRuntimeContext->backendId,
            backendRuntimeContext
                ->recordingMetadataRepository
                .get());
    }

    RecordingPersonSearchController::PersistentSearch
        persistentRecordingPersonSearch;

    if (!recordingPersonMetadataRepositories.empty())
    {
        VdrRecordingCacheRepository*
            recordingCacheRepository =
                vdrRecordingCacheRepository_.get();

        persistentRecordingPersonSearch =
            [
                recordingPersonMetadataRepositories =
                    std::move(
                        recordingPersonMetadataRepositories),
                recordingCacheRepository
            ](
                const std::string& backendId,
                const PersonQuery& query,
                int limit,
                int offset)
            {
                const std::string normalizedBackendId =
                    backendId.empty()
                        ? "default"
                        : backendId;

                for (const auto& repositoryEntry :
                     recordingPersonMetadataRepositories)
                {
                    if (repositoryEntry.first !=
                        normalizedBackendId)
                    {
                        continue;
                    }

                    VdrRecordingNativePersonSearchService
                        nativeSearchService(
                            *repositoryEntry.second,
                            *recordingCacheRepository);
                    return nativeSearchService.search(
                        normalizedBackendId,
                        query,
                        limit,
                        offset);
                }

                return RecordingPersonSearchResult::empty(
                    limit,
                    offset);
            };
    }

    const bool usesPersistentRecordingPersonSearch =
        static_cast<bool>(
            persistentRecordingPersonSearch);

    recordingPersonSearchController_ =
        std::make_unique<
            RecordingPersonSearchController>(
                *recordingPersonSearchService_,
                *recordingPersonSearchResultJsonSerializer_,
                std::move(
                    persistentRecordingPersonSearch));

    std::cout
        << "recording person search controller runtime initialized: "
        << "source="
        << (
            usesPersistentRecordingPersonSearch
                ? "native-persistent-index"
                : "snapshot-fallback"
        )
        << std::endl;

    recordingActionValidationService_ = std::make_unique<RecordingActionValidationService>();

    RecordingActionValidationService::RequestGuard
        recordingActionRuntimeGuard =
            [this](const RecordingActionRequest& request) -> std::string {
                if (request.type != RecordingActionType::Delete ||
                    !vdrRecordingCacheRepository_)
                {
                    return std::string();
                }

                return daemonRecordingCutDeleteBlockReason(
                    *vdrRecordingCacheRepository_,
                    backendRuntimeContexts_,
                    request.backendId,
                    request.recordingId);
            };

    recordingActionValidationService_->setRequestGuard(
        recordingActionRuntimeGuard);

    recordingActionValidationResultJsonSerializer_ = std::make_unique<RecordingActionValidationResultJsonSerializer>();
    recordingActionValidationRequestParser_ = std::make_unique<RecordingActionValidationRequestParser>();
    recordingActionValidationController_ = std::make_unique<RecordingActionValidationController>(
        *recordingActionValidationService_,
        *recordingActionValidationResultJsonSerializer_,
        *recordingActionValidationRequestParser_);

    recordingActionExecutionService_ = std::make_unique<RecordingActionExecutionService>();
    recordingActionExecutionService_->setValidationRequestGuard(
        recordingActionRuntimeGuard);
    recordingActionExecutionResultJsonSerializer_ = std::make_unique<RecordingActionExecutionResultJsonSerializer>();
    recordingActionBackendExecutorAdapterRegistry_ = std::make_unique<RecordingActionBackendExecutorAdapterRegistry>();

    for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
        RestfulApiRecordingActionBackendConfig recordingActionConfig;
        recordingActionConfig.backendId = backendRuntimeContext->backendId;
        recordingActionConfig.basePath = "";
        // The packaged yaVDR fallback remains for existing installations.
        // A declared backend-root map is authoritative: missing/invalid
        // entries must disable native recording writes, not silently write
        // to another distribution's recording directory.
        const auto& recordingRoots = config_.recordingVideoRoots();
        const auto rootIt = recordingRoots.find(backendRuntimeContext->backendId);
        const bool explicitlyConfigured = config_.recordingVideoRootsSpecified();
        const bool configuredRootAvailable = rootIt != recordingRoots.end();
        // VDR owns the authoritative root. Query only the local SuiteBridge
        // associated with this backend; never reuse its path for another host.
        // Explicit operator policy always takes precedence and needs no probe.
        vdrsuite::agent::SuiteBridgeVideoDirectoryDiscoveryResult nativeRoot;
        const auto& bridge = config_.suiteBridge();
        if (!explicitlyConfigured && bridge.enabled &&
            backendRuntimeContext->backendId == bridge.backendId)
        {
            vdrsuite::agent::SuiteBridgeSvdrpTransportConfig transportConfig;
            transportConfig.host = bridge.host;
            transportConfig.port = bridge.port;
            transportConfig.connectTimeout =
                std::chrono::milliseconds(bridge.connectTimeoutMs);
            transportConfig.ioTimeout =
                std::chrono::milliseconds(bridge.ioTimeoutMs);
            transportConfig.operationTimeout =
                std::chrono::milliseconds(bridge.operationTimeoutMs);
            vdrsuite::agent::SuiteBridgeSvdrpTransport transport(transportConfig);
            nativeRoot =
                vdrsuite::agent::SuiteBridgeVideoDirectoryDiscovery::discover(transport);
        }

        const bool nativeRootAvailable = !nativeRoot.videoDirectory.empty();
        // A malformed success response is not a signal to fall back to an
        // unrelated disk. An older plugin's unsupported VPATH remains legacy.
        const bool invalidNativeRoot =
            nativeRoot.authoritativeReply && !nativeRootAvailable;
        const bool rootAvailable = configuredRootAvailable || nativeRootAvailable;
        recordingActionConfig.videoDirectory = configuredRootAvailable
            ? rootIt->second
            : (nativeRootAvailable ? nativeRoot.videoDirectory
                : (explicitlyConfigured || invalidNativeRoot
                    ? std::string() : "/srv/vdr/video"));
        recordingActionConfig.videoDirectoryExplicit = rootAvailable;
        recordingActionConfig.apiMode =
            RestfulApiRecordingActionApiMode::SafeMutation;
        recordingActionConfig.allowExecution =
            (!explicitlyConfigured || configuredRootAvailable) &&
            !invalidNativeRoot;
        recordingActionConfig.readOnly = !recordingActionConfig.allowExecution;
        // Local startup evidence for read-only acceptance: the operator can
        // confirm which root won without invoking any recording mutation.
        const char* rootSource = configuredRootAvailable ? "configured"
            : nativeRootAvailable ? "suitebridge-vpath"
            : invalidNativeRoot ? "invalid-vpath"
            : explicitlyConfigured ? "invalid-config" : "legacy";
        std::cout << "recording-root backend="
                  << backendRuntimeContext->backendId
                  << " source=" << rootSource
                  << " path=" << (recordingActionConfig.videoDirectory.empty()
                        ? "(none)" : recordingActionConfig.videoDirectory)
                  << " writes=" << (recordingActionConfig.readOnly
                        ? "blocked" : "policy-eligible") << std::endl;

        recordingActionBackendExecutorAdapterRegistry_->registerAdapter(
            std::make_shared<RestfulApiRecordingActionBackendExecutorAdapter>(
                recordingActionConfig,
                *backendRuntimeContext->httpClient));
    }

    recordingActionExecutionController_ = std::make_unique<RecordingActionExecutionController>(
        *recordingActionExecutionService_,
        *recordingActionExecutionResultJsonSerializer_,
        *recordingActionBackendExecutorAdapterRegistry_,
        backendRegistry_,
        *recordingActionValidationRequestParser_,
        *vdrSnapshotReadService_);
    recordingActionExecutionController_->setAfterSuccessfulExecutionCallback(
        [this](const RecordingActionRequest& request) -> bool {
            const std::string backendId =
                request.backendId.empty()
                    ? "default"
                    : request.backendId;

            // This branch applies only to Public-v1 writes. Legacy route
            // behavior remains unchanged. A provider 2xx is not sufficient:
            // verify effect with an independent provider inventory before
            // retiring/rebinding the only persistent rec_ identity authority.
            if (request.parameters.count("publicV1Action") != 0U) {
                if (!publicRecordingIdentities_ ||
                    !vdrRecordingCacheRepository_ ||
                    !snapshotCacheService_)
                    return false;

                const auto native = request.parameters.find("backendNativeId");
                VdrRecording before;
                if (native == request.parameters.end() ||
                    !vdrRecordingCacheRepository_->findByBackendNativeId(
                        backendId, native->second, before))
                    return false;

                for (const auto& context : backendRuntimeContexts_) {
                    if (!context || context->backendId != backendId ||
                        !context->httpClient)
                        continue;
                    HttpRequest read;
                    read.method = "GET";
                    read.url = "/recordings.json";
                    read.headers["Accept"] = "application/json";
                    const HttpResponse upstream =
                        context->httpClient->execute(read);
                    if (upstream.statusCode != 200 ||
                        upstream.body.size() > 8U * 1024U * 1024U)
                        return false;

                    const auto first =
                        upstream.body.find_first_not_of(" \t\r\n");
                    const auto field =
                        upstream.body.find("\"recordings\"");
                    // A malformed/unreachable backend must never make the
                    // final recording look deleted due to an empty vector.
                    if (first == std::string::npos ||
                        !(upstream.body[first] == '[' ||
                          (upstream.body[first] == '{' &&
                           field != std::string::npos &&
                           upstream.body.find('[', field) !=
                               std::string::npos)))
                        return false;

                    std::vector<VdrRecording> fresh =
                        RestfulApiRecordingMapper::parseRecordings(upstream.body);
                    for (VdrRecording& item : fresh) {
                        item.backendId = backendId;
                        if (item.backendNativeId.empty())
                            return false;
                    }

                    if (request.type == RecordingActionType::Delete) {
                        if (!PublicRecordingActionReadback::deletedNativeId(
                                before, fresh) ||
                            !publicRecordingIdentities_->removeAfterVerifiedDeletion(
                                backendId, before.backendNativeId))
                            return false;
                    } else {
                        const auto relocated =
                            PublicRecordingActionReadback::relocatedNativeId(
                                before, fresh);
                        if (!relocated ||
                            !publicRecordingIdentities_->rebindAfterVerifiedMove(
                                backendId, before.backendNativeId, *relocated))
                            return false;
                    }

                    if (!vdrRecordingCacheRepository_->
                            replaceRecordingsForBackend(backendId, fresh))
                        return false;
                    vdrRecordingCacheRepository_->markRefreshFinished(
                        backendId, static_cast<int>(fresh.size()));
                    snapshotCacheService_->updateRecordingsForBackend(
                        backendId, fresh);
                    recordingPresentationChangeQueue_.request(backendId);
                    recordingCacheRefreshQueue_.request(backendId);
                    return true;
                }
                return false;
            }

            // A successful DELETE already has authoritative mutation evidence.
            // Project that one Recording out of the Suite caches immediately,
            // publish one backend-scoped presentation invalidation, and let one
            // asynchronous provider read reconcile the authoritative inventory.
            // Never turn one mutation (or its dry-run preview) into a burst of
            // complete Recording inventory rebuilds.
            if (request.type == RecordingActionType::Delete) {
                const auto nativeId =
                    request.parameters.find("backendNativeId");
                const std::string backendNativeId =
                    nativeId == request.parameters.end()
                        ? std::string()
                        : nativeId->second;

                if (!backendNativeId.empty() &&
                    vdrRecordingCacheRepository_) {
                    vdrRecordingCacheRepository_->removeByBackendNativeId(
                        backendId,
                        backendNativeId);
                }

                if (!backendNativeId.empty() && snapshotCacheService_) {
                    snapshotCacheService_->removeRecordingForBackend(
                        backendId,
                        backendNativeId);
                }

                // Retire the opaque public ID only after a successful real
                // DELETE. A later recording at the same native path must get
                // a fresh public ID instead of inheriting the deleted one.
                if (!backendNativeId.empty() && publicRecordingIdentities_) {
                    publicRecordingIdentities_->removeAfterVerifiedDeletion(
                        backendId,
                        backendNativeId);
                }

                // The mutation is authoritative even if a local cache row was
                // already absent. Home still needs one immediate invalidation.
                recordingPresentationChangeQueue_.request(backendId);
                recordingCacheRefreshQueue_.request(backendId);
                return false;
            }

            VdrRecording previous;
            bool hasPrevious = false;
            const auto oldNative = request.parameters.find("backendNativeId");
            if (oldNative != request.parameters.end() &&
                vdrRecordingCacheRepository_ && publicRecordingIdentities_) {
                hasPrevious = vdrRecordingCacheRepository_->findByBackendNativeId(
                    backendId, oldNative->second, previous);
            }

            // Rename and move change backend-native identity. Keep their
            // single synchronous refresh so an immediate follow-up action resolves
            // the new path rather than a stale pre-mutation snapshot.
            for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
                if (!backendRuntimeContext ||
                    backendRuntimeContext->backendId != backendId ||
                    !backendRuntimeContext->snapshotBuilder) {
                    continue;
                }

                const std::vector<VdrRecording> recordings =
                    backendRuntimeContext->snapshotBuilder->buildRecordings();

                // Preserve existing public identity across a legacy rename
                // or move only when the new provider inventory proves a
                // unique equivalent source and no old native path remains.
                if (hasPrevious && publicRecordingIdentities_ &&
                    (request.type == RecordingActionType::Rename ||
                     request.type == RecordingActionType::Move)) {
                    const auto next =
                        PublicRecordingActionReadback::relocatedNativeId(
                            previous, recordings);
                    if (next)
                        (void) publicRecordingIdentities_->
                            rebindAfterVerifiedMove(backendId,
                                previous.backendNativeId, *next);
                }

                snapshotCacheService_->updateRecordingsForBackend(
                    backendRuntimeContext->backendId,
                    recordings);

                if (vdrRecordingCacheRepository_) {
                    vdrRecordingCacheRepository_->replaceRecordingsForBackend(
                        backendRuntimeContext->backendId,
                        recordings);
                    vdrRecordingCacheRepository_->markRefreshFinished(
                        backendRuntimeContext->backendId,
                        static_cast<int>(recordings.size()));
                }

                recordingPresentationChangeQueue_.request(backendId);
                return true;
            }

            return false;
        });

    recordingActionRequestPreviewService_ =
        std::make_unique<RecordingActionRequestPreviewService>();
    recordingActionRequestPreviewResultJsonSerializer_ =
        std::make_unique<RecordingActionRequestPreviewResultJsonSerializer>();
    recordingActionPreviewController_ =
        std::make_unique<RecordingActionPreviewController>(
            *recordingActionRequestPreviewService_,
            *recordingActionRequestPreviewResultJsonSerializer_,
            *recordingActionValidationRequestParser_);

    vdrTimerActionService_ = std::make_unique<VdrTimerActionService>();
    vdrTimerActionExecutionService_ = std::make_unique<VdrTimerActionExecutionService>();
    vdrTimerActionResultJsonSerializer_ = std::make_unique<VdrTimerActionResultJsonSerializer>();
    vdrTimerActionRequestParser_ = std::make_unique<VdrTimerActionRequestParser>();
    vdrTimerActionController_ = std::make_unique<VdrTimerActionController>(
        *vdrTimerActionExecutionService_,
        *vdrTimerActionResultJsonSerializer_,
        *vdrTimerActionRequestParser_,
        *backendRegistryService_,
        *backendAccessPolicy_);

    vdrChannelMoveExecutionService_ =
        std::make_unique<VdrChannelMoveExecutionService>();
    vdrChannelMoveResultJsonSerializer_ =
        std::make_unique<VdrChannelMoveResultJsonSerializer>();
    vdrChannelMoveRequestParser_ =
        std::make_unique<VdrChannelMoveRequestParser>();
    vdrChannelMoveExecutorAdapterRegistry_ =
        std::make_unique<VdrChannelMoveExecutorAdapterRegistry>();

    for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
        if (backendRuntimeContext &&
            backendRuntimeContext->backendId == "default") {
            vdrChannelMoveExecutorAdapterRegistry_->registerAdapter(
                std::make_shared<VdrChannelMoveExecutorAdapter>(
                    backendRuntimeContext->backendId,
                    std::make_shared<SvdrpChannelMoveExecutor>()));
        }
    }

    vdrChannelMoveController_ =
        std::make_unique<VdrChannelMoveController>(
            *vdrChannelMoveExecutionService_,
            *vdrChannelMoveResultJsonSerializer_,
            *vdrChannelMoveRequestParser_,
            *vdrChannelMoveExecutorAdapterRegistry_,
            *backendRegistryService_,
            *backendAccessPolicy_);

    vdrChannelMoveController_->setAfterSuccessfulMoveCallback(
        [this](const std::string& backendId) {
            for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
                if (backendRuntimeContext &&
                    backendRuntimeContext->backendId == backendId &&
                    backendRuntimeContext->snapshotBuilder) {
                    snapshotCacheService_->updateChannelsForBackend(
                        backendId,
                        backendRuntimeContext->snapshotBuilder->buildChannels());
                    externalVdrChangeHint_.store(true);
                    break;
                }
            }
        });

    std::cout << "recording action controller runtime initialized" << std::endl;
    std::cout << "VDR timer action controller runtime initialized" << std::endl;

    epgSearchNativeFuzzyStaleProbeAdministrationService_ =
        std::make_unique<EpgSearchNativeFuzzyStaleProbeAdministrationService>(
            *epgSearchNativeFuzzyCapabilityRepository_,
            *epgSearchNativeFuzzyCapabilityFreshnessPolicy_);
    epgSearchNativeFuzzyStaleProbeAdministrationController_ =
        std::make_unique<EpgSearchNativeFuzzyStaleProbeAdministrationController>(
            *epgSearchNativeFuzzyStaleProbeAdministrationService_);

    if (searchTimerCommandExecutor_ != nullptr
        && !backendRuntimeContexts_.empty()
        && backendRuntimeContexts_.front()->searchTimerAdapter)
    {
        epgSearchNativeFuzzyOperatorRefreshService_ =
            std::make_unique<EpgSearchNativeFuzzyOperatorRefreshService>(
                *searchTimerCommandExecutor_,
                *backendRuntimeContexts_.front()->searchTimerAdapter,
                *epgSearchNativeFuzzyCapabilityRepository_,
                *epgSearchNativeFuzzyCapabilityDetector_,
                *backendRegistryService_);
        epgSearchNativeFuzzyOperatorRefreshController_ =
            std::make_unique<EpgSearchNativeFuzzyOperatorRefreshController>(
                *epgSearchNativeFuzzyOperatorRefreshService_);
        std::cout << "EPGSearch native fuzzy operator refresh API runtime initialized" << std::endl;
    }
    else
    {
        std::cout << "EPGSearch native fuzzy operator refresh API runtime skipped: SearchTimer backend unavailable" << std::endl;
    }

    runtimeDiagnosticsJsonSerializer_ = std::make_unique<RuntimeDiagnosticsJsonSerializer>();
    runtimeDiagnosticsController_ = std::make_unique<RuntimeDiagnosticsController>(runtimeDiagnosticsService_, *runtimeDiagnosticsJsonSerializer_);
    snapshotChangeFeedController_ = std::make_unique<SnapshotChangeFeedController>(*snapshotChangeFeed_, *snapshotChangeFeedJsonSerializer_);

    deviceLiveEventHub_ = std::make_shared<DeviceLiveEventHub>();
    liveTransport_ = std::make_unique<SseLiveTransport>();
    liveTransport_->setDeviceLiveEventHub(deviceLiveEventHub_);
    liveTransportService_ = std::make_unique<LiveTransportService>(*liveTransport_);
    liveTransportController_ = std::make_unique<LiveTransportController>(*liveTransport_);

    ManualRecordingMetadataApiRuntime::instance()
        .registerRecordingPresentationChangedCallback(
            [this](const std::string& backendId)
            {
                recordingPresentationChangeQueue_.request(backendId);
            });

    std::cout << "runtime diagnostics controller initialized" << std::endl;
    std::cout << "snapshot change feed controller initialized" << std::endl;
    std::cout << "live transport service initialized" << std::endl;
    std::cout << "live transport controller initialized" << std::endl;

    PublicApiRuntime::instance().registerOperationLookup(
        [this](
            const std::string& operationId,
            const std::string& actorRef)
        {
            PublicOperationLookupResult result;
            const auto found =
                mutationOperationReadService_->findForActor(
                    operationId,
                    actorRef);

            switch (found.status)
            {
                case vdrsuite::operations::MutationOperationReadStatus::ok:
                    result.status = PublicOperationLookupStatus::ok;
                    result.operation.operationId =
                        found.operation.operationId;
                    result.operation.state =
                        vdrsuite::operations::mutationOperationStateName(
                            found.operation.state);
                    result.operation.backendId =
                        found.operation.backendId;
                    result.operation.resourceRevision =
                        found.operation.operationRevision;
                    return result;

                case vdrsuite::operations::MutationOperationReadStatus::invalid:
                    result.status =
                        PublicOperationLookupStatus::invalid;
                    return result;

                case vdrsuite::operations::MutationOperationReadStatus::notFound:
                    result.status =
                        PublicOperationLookupStatus::notFound;
                    return result;

                case vdrsuite::operations::MutationOperationReadStatus::storageError:
                    result.status =
                        PublicOperationLookupStatus::unavailable;
                    return result;
            }

            return result;
        });

    PublicApiRuntime::instance().registerTimerAssignmentLookup(
        [this](
            const std::string& timerAssignmentId,
            const std::string& backendId)
        {
            PublicTimerAssignmentLookupResult result;
            const auto found =
                timerAssignmentReadService_->findForBackend(
                    timerAssignmentId,
                    backendId);

            switch (found.status)
            {
                case vdrsuite::timers::TimerAssignmentReadStatus::ok:
                    result.status =
                        PublicTimerAssignmentLookupStatus::ok;
                    result.assignment.timerAssignmentId =
                        found.assignment.timerAssignmentId;
                    result.assignment.backendId =
                        found.assignment.backendId;
                    result.assignment.resourceRevision =
                        found.assignment.assignmentRevision;
                    return result;

                case vdrsuite::timers::TimerAssignmentReadStatus::invalid:
                    result.status =
                        PublicTimerAssignmentLookupStatus::invalid;
                    return result;

                case vdrsuite::timers::TimerAssignmentReadStatus::notFound:
                    result.status =
                        PublicTimerAssignmentLookupStatus::notFound;
                    return result;

                case vdrsuite::timers::TimerAssignmentReadStatus::storageError:
                    result.status =
                        PublicTimerAssignmentLookupStatus::unavailable;
                    return result;
            }

            return result;
        });

    PublicApiRuntime::instance().registerTimerAssignmentCollectionLookup(
        [this](
            const PublicTimerAssignmentCollectionRequest& request)
        {
            PublicTimerAssignmentCollectionResult result;
            const auto page =
                timerAssignmentReadService_->listForBackend(
                    request.backendId,
                    request.afterTimerAssignmentId,
                    request.limit);

            switch (page.status)
            {
                case vdrsuite::timers::TimerAssignmentCollectionReadStatus::ok:
                    result.status =
                        PublicTimerAssignmentCollectionStatus::ok;
                    result.hasMore = page.hasMore;
                    for (const auto& assignment : page.assignments)
                    {
                        PublicTimerAssignmentCollectionItem item;
                        item.timerAssignmentId =
                            assignment.timerAssignmentId;
                        item.backendId = assignment.backendId;
                        // Only project the persisted, valid Suite-owned
                        // desired specification; no native timer observation,
                        // provider channel ID, recording path, or binding ID.
                        if (assignment.desiredNativeTimerSpecificationPresent &&
                            vdrsuite::timers::nativeTimerSpecificationValid(
                                assignment.desiredNativeTimerSpecification))
                        {
                            const auto& desired =
                                assignment.desiredNativeTimerSpecification;
                            item.plannedPresent = true;
                            item.plannedTitle = desired.title;
                            item.plannedDay = desired.day;
                            item.plannedWeekdays = desired.weekdays;
                            item.plannedStartTime = desired.startTime;
                            item.plannedEndTime = desired.endTime;
                        }
                        result.assignments.push_back(
                            std::move(item));
                    }
                    return result;

                case vdrsuite::timers::TimerAssignmentCollectionReadStatus::invalid:
                    result.status =
                        PublicTimerAssignmentCollectionStatus::invalid;
                    return result;

                case vdrsuite::timers::TimerAssignmentCollectionReadStatus::storageError:
                    result.status =
                        PublicTimerAssignmentCollectionStatus::unavailable;
                    return result;
            }
            return result;
        });

    PublicApiRuntime::instance().registerSearchTimerNativeVersionLookup(
        [this](const PublicSearchTimerNativeVersionRequest& request)
        {
            PublicSearchTimerNativeVersionResult result;
            if (!config_.suiteBridge().enabled ||
                request.backendId != config_.suiteBridge().backendId ||
                !publicSearchTimerIdentities_ || !backendRegistryService_)
                return result;

            const auto backend =
                backendRegistryService_->getBackend(request.backendId);
            if (!backend || !backend->online) return result;

            const auto native = publicSearchTimerIdentities_->
                resolveActiveNativeId(
                    request.backendId, request.publicSearchTimerId);
            if (!native)
            {
                result.status = PublicSearchTimerNativeVersionStatus::notFound;
                return result;
            }
            int numericId = -1;
            try
            {
                const unsigned long raw = std::stoul(*native);
                if (raw > static_cast<unsigned long>(2147483647L) ||
                    std::to_string(raw) != *native)
                    return result;
                numericId = static_cast<int>(raw);
            }
            catch (...) { return result; }
            for (const auto& candidate : backendRuntimeContexts_)
            {
                if (!candidate || candidate->backendId != request.backendId ||
                    !candidate->providerLocalControlTransport ||
                    !candidate->suiteBridgeAgentRuntime)
                    continue;
                SearchTimerNativeCasLocalProvider provider(
                    candidate->backendId,
                    *candidate->providerLocalControlTransport);
                const auto seen = provider.observeRule(
                    request.backendId, numericId);
                if (!seen.available) return result;
                if (!seen.found)
                {
                    result.status =
                        PublicSearchTimerNativeVersionStatus::notFound;
                    return result;
                }
                if (!seen.durable) return result;
                result.status = PublicSearchTimerNativeVersionStatus::ok;
                result.revision = seen.revision;
                return result;
            }
            return result;
        });

    PublicApiRuntime::instance().registerSearchTimerNativeMutation(
        [this](const PublicSearchTimerNativeMutationRequest& request)
        {
            PublicSearchTimerNativeMutationResult result;
            if (request.actorRef.empty() || request.backendId.empty() ||
                request.publicSearchTimerId.empty() || !request.confirmed ||
                request.ifMatch.size() != 35U ||
                request.ifMatch.compare(0U, 3U, "sr_") != 0 ||
                !config_.suiteBridge().enabled ||
                request.backendId != config_.suiteBridge().backendId ||
                !mutationOperationRepository_ || !publicSearchTimerIdentities_ ||
                !backendRegistryService_ || !backendAccessPolicy_ ||
                !backendAgentRepository_)
                return result;

            const auto backend =
                backendRegistryService_->getBackend(request.backendId);
            const auto access = backendAccessPolicy_->canWriteToBackend(
                *backendRegistryService_, request.backendId);
            if (!access.allowed || !backend || !backend->online)
                return result;

            // Idempotent replay MUST be evaluated against the ORIGINAL
            // immutable request before observing a potentially changed
            // native revision. Never claim a second CAS because the first
            // request rotated sr_ or deleted its public identity.
            std::string family = "searchtimer.native-cas.";
            switch (request.action)
            {
                case PublicSearchTimerNativeAction::update:
                    family += "update"; break;
                case PublicSearchTimerNativeAction::activate:
                    family += "activate"; break;
                case PublicSearchTimerNativeAction::deactivate:
                    family += "deactivate"; break;
                case PublicSearchTimerNativeAction::remove:
                    family += "delete"; break;
            }
            const auto existing =
                mutationOperationRepository_->findByIdempotencyScope(
                    request.actorRef, request.backendId, "SearchTimer",
                    request.publicSearchTimerId, family,
                    request.idempotencyKey);
            if (existing.status ==
                    vdrsuite::operations::MutationOperationRepositoryStatus::ok)
            {
                const auto payload =
                    mutationOperationRepository_->findPayloadByOperationId(
                        existing.operation.operationId);
                const bool identical =
                    existing.operation.expectedRevision == request.ifMatch &&
                    existing.operation.actionFamily == family &&
                    existing.operation.actorId == request.actorRef &&
                    existing.operation.backendId == request.backendId &&
                    payload.ok() &&
                    payload.payload.payloadType == "searchtimer.native-cas" &&
                    payload.payload.payloadVersion == 1U &&
                    payload.payload.payloadFingerprint ==
                        existing.operation.requestFingerprint &&
                    searchCasReplayMatches(
                        payload.payload.payload,
                        family.substr(std::string(
                            "searchtimer.native-cas.").size()),
                        request.backendId, request.publicSearchTimerId,
                        request.ifMatch, request.query);
                if (!identical)
                {
                    result.status =
                        PublicSearchTimerNativeMutationStatus::conflict;
                    return result;
                }
                result.status = PublicSearchTimerNativeMutationStatus::replayed;
                result.operation.operationId =
                    existing.operation.operationId;
                result.operation.state =
                    vdrsuite::operations::mutationOperationStateName(
                        existing.operation.state);
                result.operation.backendId =
                    existing.operation.backendId;
                result.operation.resourceRevision =
                    existing.operation.operationRevision;
                return result;
            }
            if (existing.status !=
                    vdrsuite::operations::MutationOperationRepositoryStatus::notFound)
                return result;

            const std::int64_t now =
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
            SearchTimerCurrentBackendGeneration generations(
                *backendAgentRepository_);
            const auto liveGeneration =
                generations.currentGeneration(request.backendId);
            if (!liveGeneration.has_value()) return result;

            const auto nativeId = publicSearchTimerIdentities_->
                resolveActiveNativeId(
                    request.backendId, request.publicSearchTimerId);
            if (!nativeId)
            {
                result.status = PublicSearchTimerNativeMutationStatus::notFound;
                return result;
            }
            int numericId = -1;
            try
            {
                const unsigned long value = std::stoul(*nativeId);
                if (value > static_cast<unsigned long>(2147483647L) ||
                    std::to_string(value) != *nativeId)
                    return result;
                numericId = static_cast<int>(value);
            }
            catch (...) { return result; }

            BackendRuntimeContext* runtime = nullptr;
            for (const auto& candidate : backendRuntimeContexts_)
            {
                if (candidate && candidate->backendId == request.backendId &&
                    candidate->providerLocalControlTransport &&
                    candidate->suiteBridgeAgentRuntime)
                {
                    runtime = candidate.get();
                    break;
                }
            }
            if (!runtime) return result;

            SearchTimerNativeCasLocalProvider provider(
                runtime->backendId, *runtime->providerLocalControlTransport);
            const auto seen = provider.observeRule(
                request.backendId, numericId);
            if (!seen.available) return result;
            if (!seen.found)
            {
                result.status = PublicSearchTimerNativeMutationStatus::notFound;
                return result;
            }
            if (!seen.durable) return result;
            if (seen.revision != request.ifMatch)
            {
                result.status = PublicSearchTimerNativeMutationStatus::conflict;
                return result;
            }

            SearchTimerNativeCasAdmissionRequest admissionInput;
            admissionInput.actorId = request.actorRef;
            admissionInput.backendGeneration = *liveGeneration;
            admissionInput.idempotencyKey = request.idempotencyKey;
            admissionInput.requestedAt = now;
            admissionInput.native.backendId = request.backendId;
            admissionInput.native.publicSearchTimerId =
                request.publicSearchTimerId;
            admissionInput.native.nativeId = *nativeId;
            admissionInput.native.expectedGeneration = seen.generation;
            admissionInput.native.expectedRevision = seen.revision;
            switch (request.action)
            {
                case PublicSearchTimerNativeAction::update:
                    admissionInput.native.action =
                        SearchTimerNativeCasAction::update;
                    if (!SearchTimerNativeCasLocalProvider::
                        buildInactiveQueryUpdate(seen, numericId,
                            request.query, admissionInput.native.changePayload))
                    {
                        result.status =
                            PublicSearchTimerNativeMutationStatus::invalid;
                        return result;
                    }
                    break;
                case PublicSearchTimerNativeAction::activate:
                    admissionInput.native.action =
                        SearchTimerNativeCasAction::activate;
                    break;
                case PublicSearchTimerNativeAction::deactivate:
                    admissionInput.native.action =
                        SearchTimerNativeCasAction::deactivate;
                    break;
                case PublicSearchTimerNativeAction::remove:
                    admissionInput.native.action =
                        SearchTimerNativeCasAction::remove;
                    break;
            }

            SearchTimerNativeCasAdmissionService admission(
                *mutationOperationRepository_);
            const auto admitted = admission.admit(admissionInput);
            switch (admitted.status)
            {
                case SearchTimerNativeCasAdmissionStatus::invalid:
                    result.status =
                        PublicSearchTimerNativeMutationStatus::invalid;
                    return result;
                case SearchTimerNativeCasAdmissionStatus::idempotencyConflict:
                case SearchTimerNativeCasAdmissionStatus::operationConflict:
                    result.status =
                        PublicSearchTimerNativeMutationStatus::conflict;
                    return result;
                case SearchTimerNativeCasAdmissionStatus::prepared:
                case SearchTimerNativeCasAdmissionStatus::replayed:
                    break;
                default:
                    return result;
            }

            auto operation = admitted.operation;
            if (operation.state ==
                    vdrsuite::operations::MutationOperationState::accepted &&
                operation.backendGeneration == *liveGeneration)
            {
                SearchTimerNativeCasDispatchService dispatch(
                    *mutationOperationRepository_, generations);
                SearchTimerNativeCasDispatchRequest command;
                command.operationId = operation.operationId;
                command.actorId = request.actorRef;
                command.backendGeneration = operation.backendGeneration;
                command.now = now;
                command.backendWriteAuthorized = access.allowed;
                command.operatorConfirmed = request.confirmed;
                command.native = admissionInput.native;
                const auto invoked = dispatch.dispatchOnce(command, provider);
                if (!invoked.operation.operationId.empty())
                    operation = invoked.operation;
            }
            result.status = admitted.status ==
                    SearchTimerNativeCasAdmissionStatus::prepared
                ? PublicSearchTimerNativeMutationStatus::accepted
                : PublicSearchTimerNativeMutationStatus::replayed;
            result.operation.operationId = operation.operationId;
            result.operation.state =
                vdrsuite::operations::mutationOperationStateName(
                    operation.state);
            result.operation.backendId = operation.backendId;
            result.operation.resourceRevision = operation.operationRevision;
            return result;
        });

    PublicApiRuntime::instance().registerSearchTimerCreate(
        [this](const PublicSearchTimerCreateRequest& request)
        {
            PublicSearchTimerCreateResult result;
            // Defense in depth: never admit an active SearchTimer even if a
            // caller bypasses the public JSON parser. Activation needs its
            // own generation-fenced workflow and explicit recording preview.
            if (request.active)
            {
                result.status = PublicSearchTimerCreateStatus::invalid;
                return result;
            }
            if (!mutationOperationRepository_ || !backendRegistryService_ ||
                !backendAccessPolicy_ || !backendAgentRepository_)
                return result;
            const BackendAccessDecision access =
                backendAccessPolicy_->canWriteToBackend(
                    *backendRegistryService_, request.backendId);
            const auto backend =
                backendRegistryService_->getBackend(request.backendId);
            if (!access.allowed || !backend || !backend->online)
            {
                result.status =
                    PublicSearchTimerCreateStatus::backendUnavailable;
                return result;
            }
            const std::int64_t now =
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
            const auto agent = backendAgentRepository_->findAgentForBackend(
                request.backendId);
            if (!agent || agent->revoked || agent->incompatible ||
                agent->backendGeneration == 0U ||
                agent->leaseExpiresAt <= now)
            {
                result.status =
                    PublicSearchTimerCreateStatus::backendUnavailable;
                return result;
            }
            BackendRuntimeContext* runtime = nullptr;
            for (const auto& candidate : backendRuntimeContexts_)
            {
                if (candidate && candidate->backendId == request.backendId &&
                    candidate->searchTimerAdapter && candidate->httpClient)
                {
                    runtime = candidate.get();
                    break;
                }
            }
            if (!runtime)
            {
                result.status =
                    PublicSearchTimerCreateStatus::backendUnavailable;
                return result;
            }

            SearchTimerCreateAdmissionService admission(
                *mutationOperationRepository_);
            SearchTimerCreateAdmissionRequest input;
            input.actorId = request.actorRef;
            input.backendId = request.backendId;
            input.backendGeneration = agent->backendGeneration;
            input.idempotencyKey = request.idempotencyKey;
            input.requestedAt = now;
            input.payload.name = request.name;
            input.payload.query = request.query;
            input.payload.active = request.active;
            input.payload.matchMode = request.mode;
            input.payload.matchTolerance = request.tolerance;
            input.payload.compareTitle = request.compareTitle;
            input.payload.compareSubtitle = request.compareSubtitle;
            input.payload.compareSummary = request.compareSummary;
            const auto admitted = admission.admit(input);
            if (admitted.status == SearchTimerCreateAdmissionStatus::invalid)
            {
                result.status = PublicSearchTimerCreateStatus::invalid;
                return result;
            }
            if (admitted.status ==
                    SearchTimerCreateAdmissionStatus::idempotencyConflict ||
                admitted.status ==
                    SearchTimerCreateAdmissionStatus::operationConflict)
            {
                result.status = PublicSearchTimerCreateStatus::conflict;
                return result;
            }
            if (admitted.status != SearchTimerCreateAdmissionStatus::prepared &&
                admitted.status != SearchTimerCreateAdmissionStatus::replayed)
                return result;

            auto operation = admitted.operation;
            // CAS protects the only provider write. A retry of a dispatched
            // or unknown result never calls the executor again.
            if (operation.state ==
                    vdrsuite::operations::MutationOperationState::accepted &&
                operation.backendGeneration == agent->backendGeneration)
            {
                SearchTimerCurrentBackendGeneration generations(
                *backendAgentRepository_);
                SearchTimerCreateDispatchService dispatch(
                    *mutationOperationRepository_, generations);
                SearchTimerCreateDispatchRequest command;
                command.operationId = operation.operationId;
                command.actorId = request.actorRef;
                command.backendId = request.backendId;
                command.backendGeneration = operation.backendGeneration;
                command.now = now;
                command.operatorConfirmed = true;
                command.backendWriteAllowed = access.allowed;
                command.executorOptIn = true;
                RestfulApiSearchTimerCommandExecutor executor(
                    *runtime->httpClient);
                const auto outcome = dispatch.dispatchOnce(
                    command, executor, *runtime->searchTimerAdapter);
                if (!outcome.operation.operationId.empty())
                    operation = outcome.operation;
            }
            result.status = admitted.status ==
                SearchTimerCreateAdmissionStatus::prepared
                ? PublicSearchTimerCreateStatus::accepted
                : PublicSearchTimerCreateStatus::replayed;
            result.operation.operationId = operation.operationId;
            result.operation.state =
                vdrsuite::operations::mutationOperationStateName(
                    operation.state);
            result.operation.backendId = operation.backendId;
            result.operation.resourceRevision = operation.operationRevision;
            return result;
        });

    PublicApiRuntime::instance().registerSearchTimerCollectionLookup(
        [this](const PublicSearchTimerCollectionRequest& request)
        {
            PublicSearchTimerCollectionResult result;
            if (!backendRegistryService_ || !publicSearchTimerIdentities_ ||
                request.backendId.empty() ||
                request.limit == 0U || request.limit > 30U)
            {
                result.status = PublicSearchTimerCollectionStatus::invalid;
                return result;
            }
            const auto backend = backendRegistryService_->getBackend(
                request.backendId);
            if (!backend.has_value() || !backend->enabled || !backend->online)
                return result;

            // Select exactly one authorized backend. Legacy Web's default
            // backend cannot be used to satisfy an arbitrary Device scope.
            const RestfulApiSearchTimerAdapter* adapter = nullptr;
            for (const auto& context : backendRuntimeContexts_)
            {
                if (context && context->backendId == request.backendId &&
                    context->searchTimerAdapter)
                {
                    adapter = context->searchTimerAdapter.get();
                    break;
                }
            }
            if (!adapter) return result;

            // RESTfulAPI returns the catalogue in one read; no retry or
            // cross-backend fanout. Only bounded safe projections leave v1.
            const SearchTimerResult source = adapter->list(
                SearchTimerQuery::byBackend(request.backendId));
            if (!source.available() ||
                source.totalCount() != source.returnedCount()) return result;
            // The provider catalogue is complete and validated before this
            // atomic identity reconciliation. A real empty snapshot retires
            // stale mappings; an outage or malformed catalogue never does.
            const auto publicIds = publicSearchTimerIdentities_
                ->reconcileAuthoritativeSnapshot(request.backendId,
                    source.items());
            if (!publicIds || publicIds->size() != source.items().size())
                return result;

            std::vector<PublicSearchTimerCollectionItem> candidates;
            std::size_t identityIndex = 0U;
            for (const SearchTimer& timer : source.items())
            {
                if (timer.backendId() != request.backendId ||
                    timer.backendNativeId().empty() ||
                    timer.name().empty() || timer.query().empty() ||
                    timer.name().size() > 1024U ||
                    timer.query().size() > 1024U)
                    return result;
                PublicSearchTimerCollectionItem item;
                item.searchTimerId = (*publicIds)[identityIndex++];
                item.backendId = request.backendId;
                item.name = timer.name();
                item.query = timer.query();
                item.state = timer.state() == SearchTimerState::Active
                    ? "active"
                    : (timer.state() == SearchTimerState::Inactive
                        ? "inactive" : "unknown");
                candidates.push_back(std::move(item));
            }
            std::sort(candidates.begin(), candidates.end(),
                [](const PublicSearchTimerCollectionItem& left,
                   const PublicSearchTimerCollectionItem& right)
                {
                    return left.searchTimerId < right.searchTimerId;
                });
            for (std::size_t i = 1U; i < candidates.size(); ++i)
                if (candidates[i - 1U].searchTimerId ==
                    candidates[i].searchTimerId)
                    return result;
            // Reuse this one authoritative, identity-reconciled snapshot
            // for a single saved SearchTimer lookup. A missing ID is not an
            // empty catalogue error, but must never resolve another rule.
            if (!request.searchTimerId.empty())
            {
                if (!request.afterSearchTimerId.empty() ||
                    request.limit != 1U)
                {
                    result.status = PublicSearchTimerCollectionStatus::invalid;
                    return result;
                }
                const auto match = std::lower_bound(
                    candidates.begin(), candidates.end(),
                    request.searchTimerId,
                    [](const PublicSearchTimerCollectionItem& item,
                       const std::string& id)
                    {
                        return item.searchTimerId < id;
                    });
                if (match != candidates.end() &&
                    match->searchTimerId == request.searchTimerId)
                    result.items.push_back(*match);
                result.status = PublicSearchTimerCollectionStatus::ok;
                return result;
            }
            const auto begin = std::upper_bound(
                candidates.begin(), candidates.end(),
                request.afterSearchTimerId,
                [](const std::string& id,
                   const PublicSearchTimerCollectionItem& item)
                {
                    return id < item.searchTimerId;
                });
            const std::size_t remaining =
                static_cast<std::size_t>(candidates.end() - begin);
            result.hasMore = remaining > request.limit;
            const std::size_t count = std::min(remaining, request.limit);
            for (std::size_t index = 0; index < count; ++index)
                result.items.push_back(*(begin + index));
            result.status = PublicSearchTimerCollectionStatus::ok;
            return result;
        });

    PublicApiRuntime::instance().registerSearchTimerPreviewLookup(
        [this](const PublicSearchTimerPreviewRequest& request)
        {
            PublicSearchTimerPreviewResult result;
            if (!backendRegistryService_ || !publicSearchTimerIdentities_ ||
                !searchTimerPreviewEpgCache_ || request.limit == 0U ||
                request.limit > 30U)
                return result;
            const auto backend =
                backendRegistryService_->getBackend(request.backendId);
            if (!backend.has_value() || !backend->enabled || !backend->online)
                return result;

            // Same engine and ready backend-scoped EPG cache as the Web
            // preview; do not substitute raw EPG text search.
            const auto events =
                searchTimerPreviewEpgCache_->sharedEventsForBackend(request.backendId);
            if (!events) return result;

            if (request.draft)
            {
                // A non-persisted SearchTimer goes through the exact same
                // domain preview engine as Web/saved rules. The dummy native
                // identity is internal only, never included in public output.
                SearchTimer timer = SearchTimer::create(
                    SearchTimerId::fromBackendNativeId(
                        request.backendId, "draft-preview"),
                    request.query, request.query, SearchTimerState::Active);
                timer.comparisonOptions().setCompareTitle(request.compareTitle);
                timer.comparisonOptions().setCompareSubtitle(request.compareSubtitle);
                timer.comparisonOptions().setCompareSummary(request.compareSummary);
                timer.matchOptions().setMode(request.mode);
                timer.matchOptions().setTolerance(request.tolerance);
                SearchTimerPreviewService service;
                const auto preview = service.preview(
                    timer, *events, static_cast<int>(request.limit), 0);
                result.backendId = request.backendId;
                result.totalCount = preview.totalCount();
                for (const auto& match : preview.searchResult().matches())
                {
                    const auto& event = match.event();
                    PublicSearchTimerPreviewEvent item;
                    item.title = event.title;
                    item.subtitle = event.subtitle;
                    item.channelId = event.channelId;
                    item.startTime = event.startTime;
                    item.endTime = event.endTime;
                    result.items.push_back(std::move(item));
                }
                result.status = PublicSearchTimerPreviewStatus::ok;
                return result;
            }

            const RestfulApiSearchTimerAdapter* adapter = nullptr;
            for (const auto& context : backendRuntimeContexts_)
                if (context && context->backendId == request.backendId &&
                    context->searchTimerAdapter)
                {
                    adapter = context->searchTimerAdapter.get();
                    break;
                }
            if (!adapter) return result;
            const auto catalogue = adapter->list(
                SearchTimerQuery::byBackend(request.backendId));
            if (!catalogue.available() ||
                catalogue.totalCount() != catalogue.returnedCount())
                return result;
            const auto publicIds = publicSearchTimerIdentities_
                ->reconcileAuthoritativeSnapshot(request.backendId,
                    catalogue.items());
            if (!publicIds || publicIds->size() != catalogue.items().size())
                return result;

            std::size_t identityIndex = 0U;
            for (const auto& timer : catalogue.items())
            {
                if (timer.backendId() != request.backendId ||
                    timer.backendNativeId().empty() ||
                    timer.name().empty() || timer.query().empty() ||
                    timer.name().size() > 1024U ||
                    timer.query().size() > 1024U)
                    return result;
                const std::string& publicId = (*publicIds)[identityIndex++];
                if (publicId != request.searchTimerId) continue;

                SearchTimerPreviewService service;
                const auto preview = service.preview(
                    timer, *events, static_cast<int>(request.limit), 0);
                result.searchTimerId = publicId;
                result.backendId = request.backendId;
                result.totalCount = preview.totalCount();
                for (const auto& match : preview.searchResult().matches())
                {
                    const auto& event = match.event();
                    PublicSearchTimerPreviewEvent item;
                    item.title = event.title;
                    item.subtitle = event.subtitle;
                    item.channelId = event.channelId;
                    item.startTime = event.startTime;
                    item.endTime = event.endTime;
                    result.items.push_back(std::move(item));
                }
                result.status = PublicSearchTimerPreviewStatus::ok;
                return result;
            }
            result.status = PublicSearchTimerPreviewStatus::notFound;
            return result;
        });

    PublicApiRuntime::instance().registerAccountLookup(
        [this](const std::string& accountId)
        {
            PublicAccountLookupResult result;
            if (!humanAccountReadService_)
            {
                result.status =
                    PublicAccountLookupStatus::unavailable;
                return result;
            }

            const HumanAccountLookupResult found =
                humanAccountReadService_->find(accountId);

            switch (found.status)
            {
                case HumanAccountRepositoryStatus::ok:
                    result.account.accountId =
                        found.account.accountId;
                    result.account.actorId =
                        found.account.actorId;
                    result.account.displayName =
                        found.account.displayName;
                    result.account.active =
                        found.account.active;
                    result.account.resourceRevision =
                        "account:" +
                        std::to_string(found.account.revision);
                    result.status =
                        PublicAccountLookupStatus::ok;
                    return result;

                case HumanAccountRepositoryStatus::invalid:
                    result.status =
                        PublicAccountLookupStatus::invalid;
                    return result;

                case HumanAccountRepositoryStatus::notFound:
                    result.status =
                        PublicAccountLookupStatus::notFound;
                    return result;

                case HumanAccountRepositoryStatus::revisionConflict:
                case HumanAccountRepositoryStatus::storageError:
                    result.status =
                        PublicAccountLookupStatus::unavailable;
                    return result;
            }

            result.status =
                PublicAccountLookupStatus::unavailable;
            return result;
        });

    PublicApiRuntime::instance().registerAccountCollectionLookup(
        [this](const PublicAccountCollectionRequest& request)
        {
            PublicAccountCollectionResult result;
            if (!humanAccountReadService_ ||
                request.limit == 0U ||
                request.limit > 100U)
            {
                result.status =
                    PublicAccountCollectionStatus::unavailable;
                return result;
            }

            const HumanAccountListResult listed =
                humanAccountReadService_->list();
            if (listed.status != HumanAccountRepositoryStatus::ok)
            {
                result.status =
                    PublicAccountCollectionStatus::unavailable;
                return result;
            }

            std::vector<PublicAccountCollectionItem> eligible;
            for (const HumanAccountRecord& account : listed.accounts)
            {
                if (!request.afterAccountId.empty() &&
                    account.accountId <= request.afterAccountId)
                {
                    continue;
                }

                PublicAccountCollectionItem item;
                item.accountId = account.accountId;
                item.actorId = account.actorId;
                item.displayName = account.displayName;
                item.active = account.active;
                eligible.push_back(std::move(item));
            }

            result.hasMore = eligible.size() > request.limit;
            if (result.hasMore)
                eligible.resize(request.limit);
            result.accounts = std::move(eligible);
            result.status = PublicAccountCollectionStatus::ok;
            return result;
        });

    PublicApiRuntime::instance().registerBackendCollectionLookup(
        [this](const PublicBackendCollectionRequest& request)
        {
            PublicBackendCollectionResult result;
            if (!backendRegistryService_ ||
                request.limit == 0U ||
                request.limit > 100U)
            {
                result.status =
                    PublicBackendCollectionStatus::unavailable;
                return result;
            }

            std::vector<BackendNode> registered =
                backendRegistryService_->listBackends();
            std::sort(
                registered.begin(),
                registered.end(),
                [](const BackendNode& lhs, const BackendNode& rhs)
                {
                    return lhs.backendId < rhs.backendId;
                });

            const bool wildcard =
                std::find(
                    request.authorizedBackendIds.begin(),
                    request.authorizedBackendIds.end(),
                    std::string("*")) !=
                request.authorizedBackendIds.end();

            std::vector<PublicBackendCollectionItem> eligible;
            for (const BackendNode& backend : registered)
            {
                const bool authorized =
                    wildcard ||
                    std::binary_search(
                        request.authorizedBackendIds.begin(),
                        request.authorizedBackendIds.end(),
                        backend.backendId);
                const bool afterCursor =
                    request.afterBackendId.empty() ||
                    backend.backendId > request.afterBackendId;
                if (!authorized || !afterCursor)
                    continue;

                PublicBackendCollectionItem item;
                item.backendId = backend.backendId;
                item.name = backend.backendName;
                item.enabled = backend.enabled;
                item.online = backend.online;
                eligible.push_back(std::move(item));
            }

            result.hasMore =
                eligible.size() > request.limit;
            if (result.hasMore)
                eligible.resize(request.limit);
            result.backends = std::move(eligible);
            result.status =
                PublicBackendCollectionStatus::ok;
            return result;
        });

    // Public-v1 is an authorized projection of the exact Web search result,
    // not another search or a blanket-fail per-result consistency gate.
    PublicApiRuntime::instance().registerSearchLookup(
        [this](const PublicSearchRequest& request)
        {
            PublicSearchResult unavailable;
            if (!backendRegistryService_ || !publicRecordingIdentities_ ||
                !vdrSnapshotReadService_ || request.backendId.empty() ||
                request.limit == 0U || request.limit > 30U)
            {
                std::cerr << "Public-v1 search unavailable: runtime dependency"
                          << std::endl;
                return unavailable;
            }
            const auto backend =
                backendRegistryService_->getBackend(request.backendId);
            if (!backend.has_value() || !backend->enabled || !backend->online)
            {
                std::cerr << "Public-v1 search unavailable: backend offline"
                          << std::endl;
                return unavailable;
            }
            // Recording identities require a current VDR snapshot to prevent
            // obsolete index entries from exposing deleted recordings.
            // EPG search reads the shared search index, not the snapshot.
            if (!request.epg &&
                !vdrSnapshotReadService_->hasSnapshotForBackend(request.backendId))
            {
                std::cerr << "Public-v1 search unavailable: snapshot missing"
                          << std::endl;
                return unavailable;
            }

            const auto now = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            const auto searched = GlobalSearchApiRuntime::instance().searchPublic(
                request.backendId, request.query,
                now - 6 * 3600, now + 14 * 24 * 3600,
                static_cast<int>(request.limit),
                static_cast<int>(request.offset));
            if (!searched || searched->status != "ready")
            {
                std::cerr << "Public-v1 search unavailable: shared search"
                          << std::endl;
                return unavailable;
            }

            const std::vector<VdrRecording> snapshot = request.epg
                ? std::vector<VdrRecording>{}
                : vdrSnapshotReadService_->getRecordingsForBackend(
                    request.backendId);
            PublicSearchResult response = PublicSearchProjection::project(
                request, *searched, snapshot,
                [this](const std::string& backendId,
                       const std::string& nativeId)
                {
                    return publicRecordingIdentities_->resolveOrCreate(
                        backendId, nativeId);
                });

            const std::size_t original = request.epg
                ? searched->epg.size() : searched->recordings.size();
            if (response.status != PublicSearchStatus::ok)
                std::cerr << "Public-v1 search unavailable: projection contract"
                          << std::endl;
            else if (response.items.size() != original)
                std::cerr << "Public-v1 search: filtered "
                          << (original - response.items.size())
                          << " stale or invalid entries" << std::endl;
            return response;
        });

    PublicApiRuntime::instance().registerRecordingCollectionLookup(
        [this](const PublicRecordingCollectionRequest& request)
        {
            PublicRecordingCollectionResult result;
            if (!backendRegistryService_ || !vdrSnapshotReadService_ ||
                !publicRecordingIdentities_ ||
                (request.browseFolders && !vdrRecordingCacheRepository_) ||
                request.backendId.empty() ||
                request.limit == 0U || request.limit > 100U)
            {
                result.status = PublicRecordingCollectionStatus::invalid;
                return result;
            }

            const auto backend =
                backendRegistryService_->getBackend(request.backendId);
            if (!backend.has_value() || !backend->enabled ||
                !backend->online ||
                !vdrSnapshotReadService_->hasSnapshotForBackend(
                    request.backendId))
            {
                std::cerr << "Public-v1 Recording browse unavailable: "
                             "backend/snapshot precondition failed" << std::endl;
                result.status = PublicRecordingCollectionStatus::unavailable;
                return result;
            }

            if (request.browseFolders)
            {
                // Reuse the Home cache hierarchy; only opaque folder IDs,
                // safe labels and Public-v1 recording identities leave here.
                VdrRecordingFolderPage initial;
                if (!vdrRecordingCacheRepository_->folderPageForBackendByPublicId(
                        request.backendId, request.folderId, 1, 0, initial))
                {
                    result.status = PublicRecordingCollectionStatus::invalid;
                    return result;
                }
                const std::size_t folderCount = initial.folders.size();
                const std::size_t recordingCount =
                    static_cast<std::size_t>(std::max(0, initial.recordingCount));
                result.totalEntries = folderCount + recordingCount;
                const std::size_t begin =
                    std::min(request.offset, folderCount);
                const std::size_t end =
                    std::min(folderCount, request.offset + request.limit);
                for (std::size_t index = begin; index < end; ++index)
                {
                    const auto& child = initial.folders[index];
                    PublicRecordingFolderItem entry;
                    entry.folderId =
                        VdrRecordingCacheRepository::publicFolderId(
                            request.backendId, child.path);
                    entry.name = child.name;
                    entry.recordingCount = child.recordingCount;
                    if (child.singleRecordingLeaf)
                    {
                        // A VDR title directory with exactly one recording is
                        // presented as a recording, not as another TV folder.
                        PublicRecordingCollectionProjection projector(
                            *publicRecordingIdentities_);
                        const auto visible = projector.project(
                            request.backendId, {child.singleRecording});
                        if (!visible.valid || visible.items.size() != 1U)
                        {
                            std::cerr << "Public-v1 Recording browse unavailable: "
                                         "single-recording leaf projection failed"
                                      << std::endl;
                            result.status =
                                PublicRecordingCollectionStatus::unavailable;
                            return result;
                        }
                        const auto& item = visible.items.front();
                        entry.singleRecordingLeaf = true;
                        entry.singleRecording.recordingId = item.recordingId;
                        entry.singleRecording.backendId = item.backendId;
                        entry.singleRecording.title = item.title;
                        entry.singleRecording.recordedAt = item.recordedAt;
                        entry.singleRecording.durationSeconds =
                            item.durationSeconds;
                        entry.singleRecording.durationKnown =
                            item.durationKnown;
                        entry.singleRecording.edited = item.edited;
                        entry.singleRecording.description = item.description;
                        entry.singleRecording.sizeMb = item.sizeMb;
                    }
                    result.folders.push_back(std::move(entry));
                }
                const std::size_t used = result.folders.size();
                if (used < request.limit &&
                    request.offset + used >= folderCount &&
                    request.offset + used < result.totalEntries)
                {
                    const std::size_t recordingOffset =
                        request.offset + used - folderCount;
                    VdrRecordingFolderPage direct;
                    if (!vdrRecordingCacheRepository_->folderPageForBackendByPublicId(
                            request.backendId, request.folderId,
                            static_cast<int>(request.limit - used),
                            static_cast<int>(recordingOffset), direct))
                    {
                        result.status = PublicRecordingCollectionStatus::unavailable;
                        return result;
                    }
                    PublicRecordingCollectionProjection projector(
                        *publicRecordingIdentities_);
                    const auto visible = projector.project(
                        request.backendId, direct.recordings);
                    if (!visible.valid)
                    {
                        std::cerr << "Public-v1 Recording browse unavailable: "
                                     "direct-recording projection failed"
                                  << std::endl;
                        result.status = PublicRecordingCollectionStatus::unavailable;
                        return result;
                    }
                    for (const auto& item : visible.items)
                    {
                        PublicRecordingCollectionItem entry;
                        entry.recordingId = item.recordingId;
                        entry.backendId = item.backendId;
                        entry.title = item.title;
                        entry.recordedAt = item.recordedAt;
                        entry.durationSeconds = item.durationSeconds;
                        entry.durationKnown = item.durationKnown;
                        entry.edited = item.edited;
                        entry.description = item.description;
                        entry.sizeMb = item.sizeMb;
                        result.recordings.push_back(std::move(entry));
                    }
                }
                result.hasMore = request.offset +
                    result.folders.size() + result.recordings.size() <
                        result.totalEntries;
                result.status = PublicRecordingCollectionStatus::ok;
                return result;
            }

            // ADR-0035 intentionally skips recordings in the startup snapshot.
            // Use the same warmed backend cache as Web/Home folder browsing.
            if (!vdrRecordingCacheRepository_)
            {
                result.status = PublicRecordingCollectionStatus::unavailable;
                return result;
            }
            const auto cacheStatus =
                vdrRecordingCacheRepository_->statusForBackend(request.backendId);
            if (cacheStatus.state != "ready")
            {
                result.status = PublicRecordingCollectionStatus::unavailable;
                return result;
            }
            const auto cached =
                vdrRecordingCacheRepository_->findAllForBackend(request.backendId);
            if (cached.empty() && cacheStatus.totalCount > 0)
            {
                result.status = PublicRecordingCollectionStatus::unavailable;
                return result;
            }
            PublicRecordingCollectionProjection projection(
                *publicRecordingIdentities_);
            const auto page = projection.project(request.backendId, cached);
            if (!page.valid)
            {
                result.status = PublicRecordingCollectionStatus::unavailable;
                return result;
            }

            std::vector<PublicRecordingReadItem> records = page.items;
            std::sort(records.begin(), records.end(),
                [](const PublicRecordingReadItem& lhs,
                   const PublicRecordingReadItem& rhs) {
                    return lhs.recordingId < rhs.recordingId;
                });
            for (const auto& item : records)
            {
                if (!request.afterRecordingId.empty() &&
                    item.recordingId <= request.afterRecordingId)
                    continue;

                if (result.recordings.size() >= request.limit)
                {
                    result.hasMore = true;
                    break;
                }

                PublicRecordingCollectionItem view;
                view.recordingId = item.recordingId;
                view.backendId = item.backendId;
                view.title = item.title;
                view.recordedAt = item.recordedAt;
                view.durationSeconds = item.durationSeconds;
                view.durationKnown = item.durationKnown;
                view.edited = item.edited;
                view.description = item.description;
                view.sizeMb = item.sizeMb;
                result.recordings.push_back(std::move(view));
            }
            result.status = PublicRecordingCollectionStatus::ok;
            return result;
        });

    // M1: the same warmed backend cache + persistent rec_ mapping as the
    // Public-v1 Recording collection. Never select by a user-supplied VDR
    // filename or route through a private metadata endpoint.
    PublicApiRuntime::instance().registerRecordingDetailLookup(
        [this](const PublicRecordingDetailRequest& request)
        {
            PublicRecordingDetailResult result;
            if (request.backendId.empty() || request.backendId == "*" ||
                request.recordingId.size() != 36U)
            {
                result.status = PublicRecordingDetailStatus::invalid;
                return result;
            }

            if (!backendRegistryService_ || !vdrSnapshotReadService_ ||
                !vdrRecordingCacheRepository_ || !publicRecordingIdentities_)
                return result;

            const auto backend =
                backendRegistryService_->getBackend(request.backendId);
            if (!backend.has_value() || !backend->enabled ||
                !backend->online ||
                !vdrSnapshotReadService_->hasSnapshotForBackend(
                    request.backendId) ||
                vdrRecordingCacheRepository_->statusForBackend(
                    request.backendId).state != "ready")
                return result;

            PublicRecordingPlaybackTargetResolver resolver(
                *publicRecordingIdentities_,
                *vdrRecordingCacheRepository_);
            const auto found = resolver.resolveForRead(
                request.backendId, request.recordingId);
            if (found.status == PublicRecordingPlaybackTargetStatus::invalidRequest)
            {
                result.status = PublicRecordingDetailStatus::invalid;
                return result;
            }
            if (found.status != PublicRecordingPlaybackTargetStatus::ready)
            {
                result.status = PublicRecordingDetailStatus::notFound;
                return result;
            }

            // Reuse the existing Recording presentation, including path-
            // derived display name, edited flag and capped synopsis.
            PublicRecordingCollectionProjection projector(
                *publicRecordingIdentities_);
            const auto displayed = projector.project(
                request.backendId, {found.recording});
            if (!displayed.valid || displayed.items.size() != 1U ||
                displayed.items.front().recordingId != request.recordingId)
                return result;

            auto detail = PublicRecordingMetadataDetailProjection::project(
                request.backendId, request.recordingId,
                found.recording, displayed.items.front());
            if (!detail || !metadataRepository_)
                return result;

            // Reuse the Web manual assignment and canonical cache-key
            // resolution. This lookup performs no metadata schema creation,
            // scraper fetch, mutation or portrait materialization.
            const auto manual =
                metadataRepository_->getManualRecordingMetadataReadOnly(
                    request.backendId, found.recording.backendNativeId);
            if (!PublicRecordingMetadataDetailProjection::applyManualAssignment(
                    request.backendId, manual, *detail))
                return result;

            // The Web's canonical, separately assigned Genre belongs to the
            // same backend-native Recording, whether or not manual metadata
            // is selected. Read without creating/reconciling the Genre index.
            const auto manualGenreId =
                metadataRepository_->getManualRecordingGenreReadOnly(
                    request.backendId, found.recording.backendNativeId);
            if (manualGenreId.size() > 128U ||
                std::any_of(manualGenreId.begin(), manualGenreId.end(),
                    [](unsigned char c) {
                        return c < 0x20U || c == 0x7fU;
                    }))
                return result;

            // Identity can be retired/rebound during multiple readonly DB
            // lookups; never expose stale manual metadata across rebinding.
            const auto bound = publicRecordingIdentities_->find(
                request.backendId, found.recording.backendNativeId);
            if (!bound || *bound != request.recordingId)
            {
                result.status = PublicRecordingDetailStatus::notFound;
                return result;
            }

            const auto& source = *detail;
            auto& out = result.item;
            out.recording.recordingId = source.recording.recordingId;
            out.recording.backendId = source.recording.backendId;
            out.recording.title = source.recording.title;
            out.recording.recordedAt = source.recording.recordedAt;
            out.recording.durationSeconds = source.recording.durationSeconds;
            out.recording.durationKnown = source.recording.durationKnown;
            out.recording.edited = source.recording.edited;
            out.recording.description = source.recording.description;
            out.recording.sizeMb = source.recording.sizeMb;
            out.metadataState = source.metadataState;
            out.metadataSource = source.metadataSource;
            out.contentKind = source.contentKind;
            out.epgTitle = source.epgTitle;
            out.subtitle = source.subtitle;
            out.epgDescription = source.epgDescription;
            out.providerTitle = source.providerTitle;
            out.overview = source.overview;
            out.originalTitle = source.originalTitle;
            out.tagline = source.tagline;
            out.seriesTitle = source.seriesTitle;
            out.episodeTitle = source.episodeTitle;
            out.genreText = source.genreText;
            out.manualGenreId = manualGenreId;
            out.releaseDate = source.releaseDate;
            out.seasonNumber = source.seasonNumber;
            out.episodeNumber = source.episodeNumber;
            out.providerRuntimeMinutes = source.providerRuntimeMinutes;
            out.manualAssignmentId = source.manualAssignmentId;
            out.metadataEntityId = source.metadataEntityId;
            out.manualRevision = source.manualRevision;
            result.status = PublicRecordingDetailStatus::ok;
            return result;
        });

    // Same Suite Genre index as the Web frontend; Public-v1 only projects
    // authorized, paginated and opaque-ID read models.
    PublicApiRuntime::instance().registerGenreCollectionLookup(
        [this](const PublicGenreCollectionRequest& request)
        {
            PublicGenreCollectionResult result;
            if (!backendRegistryService_ || !vdrSnapshotReadService_ ||
                !publicRecordingIdentities_ || request.backendId.empty() ||
                request.limit == 0U || request.limit > 30U ||
                request.offset > 1000000U)
            {
                result.status = PublicGenreCollectionStatus::invalid;
                return result;
            }
            const auto backend =
                backendRegistryService_->getBackend(request.backendId);
            if (!backend.has_value() || !backend->enabled ||
                !backend->online ||
                !vdrSnapshotReadService_->hasSnapshotForBackend(
                    request.backendId))
                return result;

            auto& genres = GenreBrowserApiRuntime::instance();
            if (request.recordings)
            {
                GenreRecordingPage page;
                if (!genres.recordingPage(request.backendId,
                    request.genreId, static_cast<int>(request.limit),
                    static_cast<int>(request.offset), page) ||
                    page.totalCount < 0 ||
                    page.recordings.size() > request.limit)
                    return result;

                // The fast startup snapshot deliberately omits recordings.
                // Genre membership is indexed from the persisted VDR recording
                // cache, so resolve each member against that same backend-
                // scoped, current inventory instead of the empty snapshot.
                if (!vdrRecordingCacheRepository_)
                    return result;
                for (const auto& item : page.recordings)
                {
                    if (item.backendId != request.backendId ||
                        item.backendNativeId.empty())
                        return PublicGenreCollectionResult{};
                    VdrRecording found;
                    if (!vdrRecordingCacheRepository_->findByBackendNativeId(
                            request.backendId, item.backendNativeId, found) ||
                        found.backendId != request.backendId ||
                        found.backendNativeId != item.backendNativeId ||
                        found.title.empty() ||
                        found.durationSeconds < 0)
                        return PublicGenreCollectionResult{};
                    const auto publicId =
                        publicRecordingIdentities_->resolveOrCreate(
                            request.backendId, found.backendNativeId);
                    if (!publicId.has_value())
                        return PublicGenreCollectionResult{};

                    PublicGenreRecordingItem view;
                    view.recordingId = *publicId;
                    view.backendId = request.backendId;
                    const auto display = PublicRecordingDisplayName::project(found);
                    view.title = display.title;
                    view.edited = display.edited;
                    view.recordedAt = found.startTime;
                    view.durationSeconds = found.durationSeconds;
                    view.durationKnown = found.recordingDurationKnown;
                    result.recordings.push_back(std::move(view));
                }
                result.totalCount = static_cast<std::size_t>(page.totalCount);
            }
            else
            {
                GenreOverview overview;
                if (!genres.recordingOverview(
                    request.backendId, request.locale, overview))
                    return result;
                std::sort(overview.genres.begin(), overview.genres.end(),
                    [](const GenreOverviewEntry& a,
                       const GenreOverviewEntry& b) {
                        return a.genreId < b.genreId;
                    });
                result.totalCount = overview.genres.size();
                const std::size_t end =
                    std::min(result.totalCount, request.offset + request.limit);
                for (std::size_t index = request.offset; index < end; ++index)
                {
                    const auto& item = overview.genres[index];
                    if (item.genreId.empty() || item.label.empty() ||
                        item.itemCount < 0)
                        return PublicGenreCollectionResult{};
                    PublicGenreItem view;
                    view.genreId = item.genreId;
                    view.label = item.label;
                    view.labelDe = item.labelDe;
                    view.labelEn = item.labelEn;
                    view.count = static_cast<std::size_t>(item.itemCount);
                    result.genres.push_back(std::move(view));
                }
            }
            result.status = PublicGenreCollectionStatus::ok;
            return result;
        });

    // Project exactly the established Web EPG Browse genre taxonomy.
    // Default window matches the Web GenreBrowserController (48 hours).
    PublicApiRuntime::instance().registerEpgGenreLookup(
        [this](const PublicEpgGenreRequest& request)
        {
            PublicEpgGenreResult result;
            if (!backendRegistryService_ || request.backendId.empty() ||
                request.limit == 0U || request.limit > 30U ||
                request.offset > 1000000U)
            {
                result.status = PublicEpgGenreStatus::invalid;
                return result;
            }
            const auto backend = backendRegistryService_->getBackend(
                request.backendId);
            if (!backend.has_value() || !backend->enabled || !backend->online)
                return result;
            const auto now = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            const std::int64_t from = now;
            const std::int64_t until = now + 48 * 60 * 60;
            auto& genres = GenreBrowserApiRuntime::instance();
            if (request.events)
            {
                GenreEpgPage page;
                if (!genres.epgPage(request.backendId, request.contentClass,
                    request.genreId, from, until,
                    static_cast<int>(request.limit),
                    static_cast<int>(request.offset), page) ||
                    page.totalCount < 0 ||
                    page.events.size() > request.limit)
                    return result;
                result.totalCount = static_cast<std::size_t>(page.totalCount);
                for (const auto& event : page.events)
                {
                    if (event.backendId != request.backendId ||
                        event.channelId.empty() || event.eventId.empty() ||
                        event.title.empty() || event.endTime <= event.startTime)
                        return PublicEpgGenreResult{};
                    PublicEpgGenreEvent item;
                    item.backendId = request.backendId;
                    item.channelId = event.channelId;
                    item.eventId = event.eventId;
                    item.title = event.title;
                    item.subtitle = event.subtitle;
                    item.startTime = std::to_string(event.startTime);
                    item.endTime = std::to_string(event.endTime);
                    result.events.push_back(std::move(item));
                }
            }
            else
            {
                GenreEpgBrowseOverview overview;
                if (!genres.epgOverview(
                    request.backendId, request.locale, from, until, overview))
                    return result;
                std::vector<PublicEpgGenreItem> flat;
                for (const auto& category : overview.categories)
                {
                    if (category.id.empty() || category.label.empty() ||
                        category.itemCount < 0)
                        return PublicEpgGenreResult{};
                    PublicEpgGenreItem parent;
                    parent.genreId = category.id;
                    parent.contentClass = category.id;
                    parent.label = category.label;
                    parent.labelDe = category.labelDe;
                    parent.labelEn = category.labelEn;
                    parent.count = static_cast<std::size_t>(category.itemCount);
                    flat.push_back(std::move(parent));
                    for (const auto& child : category.children)
                    {
                        if (child.id.empty() || child.label.empty() ||
                            child.itemCount < 0)
                            return PublicEpgGenreResult{};
                        PublicEpgGenreItem item;
                        item.genreId = child.id;
                        item.contentClass = category.id;
                        item.label = category.label + " › " + child.label;
                        item.labelDe = category.labelDe + " › " + child.labelDe;
                        item.labelEn = category.labelEn + " › " + child.labelEn;
                        item.count = static_cast<std::size_t>(child.itemCount);
                        flat.push_back(std::move(item));
                    }
                }
                result.totalCount = flat.size();
                const std::size_t end = std::min(
                    result.totalCount, request.offset + request.limit);
                for (std::size_t index = request.offset; index < end; ++index)
                    result.genres.push_back(std::move(flat[index]));
            }
            result.status = PublicEpgGenreStatus::ok;
            return result;
        });

    PublicApiRuntime::instance().registerEpgNowNextLookup(
        [this](const PublicEpgNowNextRequest& request)
        {
            PublicEpgNowNextResult result;
            if (!backendRegistryService_ || !vdrSnapshotReadService_ ||
                !epgCacheServiceRegistry_ ||
                request.backendId.empty() ||
                request.channelId.empty() ||
                request.limit == 0U || request.limit > 2U)
            {
                result.status = PublicEpgNowNextStatus::invalid;
                return result;
            }
            const auto backend =
                backendRegistryService_->getBackend(request.backendId);
            if (!backend.has_value() || !backend->enabled ||
                !backend->online ||
                !vdrSnapshotReadService_
                    ->hasSnapshotForBackend(request.backendId))
            {
                return result;
            }
            const auto channels =
                vdrSnapshotReadService_
                    ->getChannelsForBackend(request.backendId);
            const auto channel = std::find_if(
                channels.begin(), channels.end(),
                [&request](const VdrChannel& item) {
                    return item.id == request.channelId;
                });
            if (channel == channels.end())
            {
                result.status = PublicEpgNowNextStatus::notFound;
                return result;
            }
            EpgCacheService* service =
                epgCacheServiceRegistry_->findService(request.backendId);
            if (service == nullptr) return result;
            const auto events = service->findNowNextPerChannelForBackend(
                request.backendId,
                request.channelId,
                request.fromTime,
                static_cast<int>(request.limit));
            if (events.size() > request.limit) return result;
            for (const VdrEvent& event : events)
            {
                if (event.channelId != request.channelId ||
                    event.title.empty() ||
                    event.startTime.empty() ||
                    event.endTime.empty() ||
                    event.durationSeconds < 0)
                    return PublicEpgNowNextResult{};
                PublicEpgNowNextItem item;
                item.channelId = event.channelId;
                item.title = event.title;
                item.subtitle = event.subtitle;
                item.startTime = event.startTime;
                item.endTime = event.endTime;
                item.durationSeconds = event.durationSeconds;
                result.events.push_back(std::move(item));
            }
            result.status = PublicEpgNowNextStatus::ok;
            return result;
        });

    PublicApiRuntime::instance().registerNativeTimersLookup(
        [this](const PublicNativeTimersRequest& request)
        {
            PublicNativeTimersResult result;
            if (!backendRegistryService_ || !vdrSnapshotReadService_ ||
                request.backendId.empty() || request.limit == 0U ||
                request.limit > 30U || request.offset > 100000U)
            {
                result.status = PublicNativeTimersStatus::invalid;
                return result;
            }
            const auto backend =
                backendRegistryService_->getBackend(request.backendId);
            if (!backend.has_value() || !backend->enabled ||
                !backend->online ||
                !vdrSnapshotReadService_
                    ->hasSnapshotForBackend(request.backendId))
                return result;

            auto timers = vdrSnapshotReadService_->getTimersForBackend(
                request.backendId);
            std::sort(timers.begin(), timers.end(),
                [](const VdrTimer& first, const VdrTimer& second) {
                    return first.id < second.id;
                });
            const std::size_t count = timers.size();
            const std::size_t begin = std::min(count, request.offset);
            const std::size_t end =
                std::min(count, begin + request.limit);
            result.hasMore = end < count;
            for (std::size_t index = begin; index < end; ++index)
            {
                const auto& timer = timers[index];
                if (timer.id.empty() || timer.id.size() > 512U)
                    return PublicNativeTimersResult{};
                PublicNativeTimerItem item;
                item.id = timer.id;
                item.channelId = timer.channelId;
                item.channelName = timer.channelName;
                item.title = timer.title;
                item.day = timer.day;
                item.weekdays = timer.weekdays;
                item.startTime = timer.startTime;
                item.endTime = timer.endTime;
                item.enabled = timer.enabled;
                item.recording = timer.recording;
                item.pending = timer.pending;
                result.timers.push_back(std::move(item));
            }
            result.status = PublicNativeTimersStatus::ok;
            return result;
        });

    // Public Device DELETE admission only reserves a durable ADR-0042
    // operation. Native side effects remain in the agent/productive runner.
    PublicApiRuntime::instance().registerNativeTimerModifyAdmission(
        [this](const PublicNativeTimerModifyRequest& request)
        {
            using namespace vdrsuite::timers;
            using namespace vdrsuite::operations;
            using S = PublicNativeTimerModifyStatus;
            PublicNativeTimerModifyResult response;
            if (!backendRegistryService_ || !backendAccessPolicy_ ||
                !backendAgentRepository_ || !nativeTimerBindingRepository_ ||
                !timerAssignmentRepository_ || !timerIntentRepository_ ||
                !mutationOperationRepository_ || !vdrSnapshotReadService_ ||
                request.actorRef.empty() || request.backendId.empty() ||
                request.timerId.empty())
                return response;

            const auto access = backendAccessPolicy_->canWriteToBackend(
                *backendRegistryService_, request.backendId);
            if (!access.allowed)
            {
                response.status = S::backendUnavailable;
                return response;
            }
            const auto found = nativeTimerBindingRepository_
                ->findByBackendNativeTimer(request.backendId, request.timerId);
            if (found.status == NativeTimerBindingRepositoryStatus::notFound)
            {
                response.status = S::notFound;
                return response;
            }
            if (!found.ok()) return response;
            const auto& binding = found.binding;

            const std::string expectedRevision =
                "native-timer-binding/1:" + binding.nativeTimerBindingId +
                ":" + std::to_string(binding.backendGeneration) +
                ":" + binding.bindingRevision;
            if (request.expectedResourceRevision != expectedRevision)
            {
                response.status = S::revisionConflict;
                return response;
            }
            if (binding.ownership == NativeTimerBindingOwnership::external)
            {
                response.status = S::adoptionRequired;
                return response;
            }
            if (binding.ownership != NativeTimerBindingOwnership::managed &&
                binding.ownership != NativeTimerBindingOwnership::adopted)
            {
                response.status = S::stateConflict;
                return response;
            }

            const auto assigned = timerAssignmentRepository_
                ->findById(binding.timerAssignmentId);
            if (!assigned.ok() ||
                assigned.assignment.nativeTimerBindingId != binding.nativeTimerBindingId ||
                assigned.assignment.backendId != request.backendId ||
                assigned.assignment.backendGeneration != binding.backendGeneration ||
                assigned.assignment.state != TimerAssignmentState::bound)
            {
                response.status = S::stateConflict;
                return response;
            }
            const auto intent = timerIntentRepository_
                ->findById(assigned.assignment.timerIntentId);
            if (!intent.ok())
            {
                response.status = S::stateConflict;
                return response;
            }

            NativeTimerSpecification specification;
            specification.channelId = request.specification.channelId;
            specification.title = request.specification.title;
            specification.directory = request.specification.directory;
            specification.day = request.specification.day;
            specification.weekdays = request.specification.weekdays;
            specification.startTime = request.specification.startTime;
            specification.endTime = request.specification.endTime;
            specification.priority = request.specification.priority;
            specification.lifetime = request.specification.lifetime;
            specification.enabled = request.specification.enabled;
            specification.vps = request.specification.vps;
            if (request.kind == PublicNativeTimerChangeKind::toggle)
            {
                const auto& current = binding.observedState;
                specification.channelId = current.channelId;
                specification.title = current.title;
                specification.directory = current.directory;
                specification.day = current.day;
                specification.weekdays = current.weekdays;
                specification.startTime = current.startTime;
                specification.endTime = current.endTime;
                specification.priority = current.priority;
                specification.lifetime = current.lifetime;
                specification.vps = current.vps;
            }
            if (!nativeTimerSpecificationValid(specification))
            {
                response.status = S::invalid;
                return response;
            }
            if (nativeTimerObservationMatchesSpecification(
                specification, binding.observedState))
            {
                response.status = S::stateConflict;
                return response;
            }
            const std::string specFingerprint =
                nativeTimerSpecificationFingerprint(specification);
            const std::string specDigest =
                vdrsuite::agent::backendAgentNativeTimerDeleteFingerprintToken(
                    specFingerprint);
            if (specFingerprint.empty() || specDigest.empty())
                return response;
            const std::string action = request.kind == PublicNativeTimerChangeKind::toggle
                ? "timer.toggle" : "timer.update";
            const std::string requestFingerprint =
                "native-modify/1:" + binding.nativeTimerBindingId + ":" +
                binding.bindingRevision + ":" + action + ":" + specDigest;
            if (requestFingerprint.size() > 512U)
            {
                response.status = S::invalid;
                return response;
            }

            // Replays never generate another native operation. A key can be
            // re-used only with the exact actor/resource/revision/body.
            const auto existing = mutationOperationRepository_
                ->findByIdempotencyScope(
                    request.actorRef, request.backendId,
                    "NativeTimerBinding", binding.nativeTimerBindingId,
                    action, request.idempotencyKey);
            if (existing.ok())
            {
                const auto& operation = existing.operation;
                if (operation.actorId != request.actorRef ||
                    operation.backendId != request.backendId ||
                    operation.backendGeneration != binding.backendGeneration ||
                    operation.expectedRevision != binding.bindingRevision ||
                    operation.expectedResourceFingerprint != binding.observedFingerprint ||
                    operation.requestFingerprint != requestFingerprint)
                {
                    response.status = S::idempotencyConflict;
                    return response;
                }
                response.operation = {
                    operation.operationId,
                    mutationOperationStateName(operation.state),
                    operation.backendId, operation.operationRevision};
                response.status = S::replayed;
                return response;
            }
            if (existing.status != MutationOperationRepositoryStatus::notFound)
                return response;

            const auto agent = backendAgentRepository_
                ->findAgentForBackend(request.backendId);
            const auto now = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            if (!agent || agent->revoked || agent->incompatible ||
                agent->backendGeneration != binding.backendGeneration ||
                agent->leaseExpiresAt < now ||
                !vdrSnapshotReadService_->hasSnapshotForBackend(request.backendId))
            {
                response.status = S::backendUnavailable;
                return response;
            }
            const auto timers = vdrSnapshotReadService_
                ->getTimersForBackend(request.backendId);
            const auto same = [&request](const VdrTimer& timer) {
                return timer.id == request.timerId;
            };
            const auto current = std::find_if(timers.begin(), timers.end(), same);
            if (current == timers.end() ||
                std::count_if(timers.begin(), timers.end(), same) != 1)
            {
                response.status = S::stateConflict;
                return response;
            }
            const auto verification = nativeTimerPublicDetailRevision(
                request.backendId, agent->backendGeneration,
                *current, &binding);
            if ((verification.status !=
                    NativeTimerPublicDetailRevisionStatus::verifiedManaged &&
                 verification.status !=
                    NativeTimerPublicDetailRevisionStatus::verifiedAdopted) ||
                verification.resourceRevision != expectedRevision ||
                binding.lastObservedAt < now - 300 ||
                binding.observedState.recording ||
                binding.observedState.pending)
            {
                response.status = S::stateConflict;
                return response;
            }

            NativeTimerModifyOperationPreparationRequest candidate;
            candidate.operationId = generateMutationOperationId();
            candidate.idempotencyKey = request.idempotencyKey;
            candidate.actorId = request.actorRef;
            candidate.requestFingerprint = requestFingerprint;
            candidate.kind =
                request.kind == PublicNativeTimerChangeKind::toggle
                    ? NativeTimerModifyKind::toggle
                    : NativeTimerModifyKind::update;
            candidate.timerAssignmentId = assigned.assignment.timerAssignmentId;
            candidate.expectedAssignmentRevision =
                assigned.assignment.assignmentRevision;
            candidate.expectedIntentRevision = intent.intent.intentRevision;
            candidate.expectedAssignmentEpoch =
                assigned.assignment.assignmentEpoch;
            candidate.nativeTimerBindingId = binding.nativeTimerBindingId;
            candidate.expectedBindingRevision = binding.bindingRevision;
            candidate.expectedBackendId = request.backendId;
            candidate.expectedBackendGeneration = binding.backendGeneration;
            candidate.backendNativeTimerId = binding.backendNativeTimerId;
            candidate.expectedCurrentFingerprint = binding.observedFingerprint;
            candidate.expectedSpecification = specification;
            candidate.requestedAt = now;
            candidate.deadline = now + 3600;

            NativeTimerModifyOperationPreparationService preparation(
                *timerIntentRepository_, *timerAssignmentRepository_,
                *nativeTimerBindingRepository_, *mutationOperationRepository_);
            const auto stored = preparation.prepare(candidate);
            switch (stored.status)
            {
                case NativeTimerModifyOperationPreparationStatus::prepared:
                case NativeTimerModifyOperationPreparationStatus::alreadyPrepared:
                    response.operation = {
                        stored.operation.operationId,
                        mutationOperationStateName(stored.operation.state),
                        stored.operation.backendId,
                        stored.operation.operationRevision};
                    response.status = S::accepted;
                    return response;
                case NativeTimerModifyOperationPreparationStatus::intentNotFound:
                case NativeTimerModifyOperationPreparationStatus::assignmentNotFound:
                case NativeTimerModifyOperationPreparationStatus::bindingNotFound:
                    response.status = S::notFound; return response;
                case NativeTimerModifyOperationPreparationStatus::bindingRevisionConflict:
                case NativeTimerModifyOperationPreparationStatus::intentRevisionConflict:
                case NativeTimerModifyOperationPreparationStatus::assignmentRevisionConflict:
                    response.status = S::revisionConflict; return response;
                case NativeTimerModifyOperationPreparationStatus::ownershipConflict:
                    response.status = S::adoptionRequired; return response;
                case NativeTimerModifyOperationPreparationStatus::idempotencyConflict:
                    response.status = S::idempotencyConflict; return response;
                case NativeTimerModifyOperationPreparationStatus::invalid:
                    response.status = S::invalid; return response;
                case NativeTimerModifyOperationPreparationStatus::repositoryError:
                    return response;
                default:
                    response.status = S::stateConflict; return response;
            }
        });

    PublicApiRuntime::instance().registerNativeTimerDeleteAdmission(
        [this](const PublicNativeTimerDeleteRequest& request)
        {
            using namespace vdrsuite::timers;
            using namespace vdrsuite::operations;
            using S = PublicNativeTimerDeleteStatus;
            PublicNativeTimerDeleteResult result;
            if (!backendRegistryService_ || !backendAccessPolicy_ ||
                !backendAgentRepository_ || !nativeTimerBindingRepository_ ||
                !timerAssignmentRepository_ || !mutationOperationRepository_ ||
                !vdrSnapshotReadService_ ||
                request.actorRef.empty() || request.backendId.empty() ||
                request.timerId.empty())
                return result;

            const auto access = backendAccessPolicy_->canWriteToBackend(
                *backendRegistryService_, request.backendId);
            if (!access.allowed)
            {
                result.status = S::backendUnavailable;
                return result;
            }

            const auto binding = nativeTimerBindingRepository_
                ->findByBackendNativeTimer(request.backendId, request.timerId);
            if (binding.status == NativeTimerBindingRepositoryStatus::notFound)
            {
                result.status = S::notFound;
                return result;
            }
            if (!binding.ok())
                return result;
            const auto& durable = binding.binding;

            // The resource ETag must come from this durable binding and
            // generation, not from a mutable VDR list index.
            const std::string revision =
                "native-timer-binding/1:" + durable.nativeTimerBindingId +
                ":" + std::to_string(durable.backendGeneration) +
                ":" + durable.bindingRevision;
            if (request.expectedResourceRevision != revision)
            {
                result.status = S::revisionConflict;
                return result;
            }

            // Idempotent replay does not issue another delete even after a
            // verified absence or a process restart. Never reuse a key with
            // mismatched native resource, actor, revision or generation.
            const auto existing = mutationOperationRepository_
                ->findByIdempotencyScope(
                    request.actorRef, request.backendId,
                    "NativeTimerBinding", durable.nativeTimerBindingId,
                    "timer.delete", request.idempotencyKey);
            if (existing.status == MutationOperationRepositoryStatus::ok)
            {
                const auto& op = existing.operation;
                if (op.actorId != request.actorRef ||
                    op.backendId != request.backendId ||
                    op.backendGeneration != durable.backendGeneration ||
                    op.expectedRevision != durable.bindingRevision ||
                    op.resourceId != durable.nativeTimerBindingId ||
                    op.expectedResourceFingerprint != durable.observedFingerprint)
                {
                    result.status = S::idempotencyConflict;
                    return result;
                }
                result.operation = {
                    op.operationId, mutationOperationStateName(op.state),
                    op.backendId, op.operationRevision};
                result.status = S::replayed;
                return result;
            }
            if (existing.status != MutationOperationRepositoryStatus::notFound)
                return result;

            if (durable.ownership == NativeTimerBindingOwnership::external)
            {
                result.status = S::adoptionRequired;
                return result;
            }
            if (durable.ownership != NativeTimerBindingOwnership::managed &&
                durable.ownership != NativeTimerBindingOwnership::adopted)
            {
                result.status = S::stateConflict;
                return result;
            }

            const auto ownership = timerAssignmentRepository_
                ->findById(durable.timerAssignmentId);
            if (!ownership.ok() ||
                ownership.assignment.timerAssignmentId != durable.timerAssignmentId ||
                ownership.assignment.nativeTimerBindingId != durable.nativeTimerBindingId ||
                ownership.assignment.backendId != request.backendId ||
                ownership.assignment.backendGeneration != durable.backendGeneration ||
                ownership.assignment.state != TimerAssignmentState::bound)
            {
                result.status = S::stateConflict;
                return result;
            }
            const auto agent = backendAgentRepository_
                ->findAgentForBackend(request.backendId);
            const auto now = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            if (!agent || agent->revoked || agent->incompatible ||
                agent->backendGeneration != durable.backendGeneration ||
                agent->leaseExpiresAt < now ||
                !vdrSnapshotReadService_->hasSnapshotForBackend(request.backendId))
            {
                result.status = S::backendUnavailable;
                return result;
            }

            const auto timers =
                vdrSnapshotReadService_->getTimersForBackend(request.backendId);
            const auto same = [&request](const VdrTimer& timer) {
                return timer.id == request.timerId;
            };
            const auto timer = std::find_if(timers.begin(), timers.end(), same);
            if (timer == timers.end() ||
                std::count_if(timers.begin(), timers.end(), same) != 1)
            {
                result.status = S::stateConflict;
                return result;
            }
            const auto verified = nativeTimerPublicDetailRevision(
                request.backendId, agent->backendGeneration, *timer, &durable);
            if ((verified.status !=
                    NativeTimerPublicDetailRevisionStatus::verifiedManaged &&
                 verified.status !=
                    NativeTimerPublicDetailRevisionStatus::verifiedAdopted) ||
                verified.resourceRevision != request.expectedResourceRevision)
            {
                result.status = S::stateConflict;
                return result;
            }

            PublicNativeTimerDeletePreflightRequest preflight;
            preflight.nativeTimerBindingId = durable.nativeTimerBindingId;
            preflight.backendId = request.backendId;
            preflight.expectedBackendGeneration = durable.backendGeneration;
            preflight.backendNativeTimerId = request.timerId;
            preflight.expectedBindingRevision = durable.bindingRevision;
            preflight.expectedObservedFingerprint = durable.observedFingerprint;
            preflight.observedNotBefore = std::max<std::int64_t>(1, now - 300);
            if (publicNativeTimerDeletePreflight(preflight, durable) !=
                PublicNativeTimerDeletePreflightStatus::eligible)
            {
                result.status = S::stateConflict;
                return result;
            }

            const std::string nativeDigest =
                vdrsuite::agent::backendAgentNativeTimerDeleteFingerprintToken(
                    durable.observedFingerprint);
            if (nativeDigest.empty())
                return result;

            NativeTimerDeleteOperationPreparationRequest candidate;
            candidate.operationId = generateMutationOperationId();
            candidate.idempotencyKey = request.idempotencyKey;
            candidate.actorId = request.actorRef;
            candidate.requestFingerprint =
                "native-delete/1:" + durable.nativeTimerBindingId +
                ":" + durable.bindingRevision + ":" + nativeDigest;
            candidate.nativeTimerBindingId = durable.nativeTimerBindingId;
            candidate.expectedBindingRevision = durable.bindingRevision;
            candidate.expectedBackendGeneration = durable.backendGeneration;
            candidate.requestedAt = now;
            candidate.deadline = now + 3600;

            NativeTimerDeleteOperationPreparationService prepare(
                *mutationOperationRepository_, *nativeTimerBindingRepository_);
            const auto stored = prepare.prepare(candidate);
            switch (stored.status)
            {
                case NativeTimerDeleteOperationPreparationStatus::prepared:
                case NativeTimerDeleteOperationPreparationStatus::alreadyPrepared:
                    result.operation = {
                        stored.operation.operationId,
                        mutationOperationStateName(stored.operation.state),
                        stored.operation.backendId,
                        stored.operation.operationRevision};
                    result.status = S::accepted;
                    return result;
                case NativeTimerDeleteOperationPreparationStatus::idempotencyConflict:
                {
                    const auto raced = mutationOperationRepository_
                        ->findByIdempotencyScope(
                            request.actorRef, request.backendId,
                            "NativeTimerBinding", durable.nativeTimerBindingId,
                            "timer.delete", request.idempotencyKey);
                    if (raced.ok() &&
                        raced.operation.expectedRevision == durable.bindingRevision &&
                        raced.operation.expectedResourceFingerprint == durable.observedFingerprint &&
                        raced.operation.backendGeneration == durable.backendGeneration &&
                        raced.operation.actorId == request.actorRef)
                    {
                        result.operation = {
                            raced.operation.operationId,
                            mutationOperationStateName(raced.operation.state),
                            raced.operation.backendId,
                            raced.operation.operationRevision};
                        result.status = S::replayed;
                    }
                    else
                        result.status = S::idempotencyConflict;
                    return result;
                }
                case NativeTimerDeleteOperationPreparationStatus::bindingNotFound:
                    result.status = S::notFound; return result;
                case NativeTimerDeleteOperationPreparationStatus::bindingRevisionConflict:
                    result.status = S::revisionConflict; return result;
                case NativeTimerDeleteOperationPreparationStatus::ownershipConflict:
                    result.status = S::adoptionRequired; return result;
                case NativeTimerDeleteOperationPreparationStatus::bindingMissing:
                case NativeTimerDeleteOperationPreparationStatus::driftConflict:
                case NativeTimerDeleteOperationPreparationStatus::generationConflict:
                case NativeTimerDeleteOperationPreparationStatus::operationConflict:
                case NativeTimerDeleteOperationPreparationStatus::operationStateConflict:
                    result.status = S::stateConflict; return result;
                case NativeTimerDeleteOperationPreparationStatus::invalid:
                    result.status = S::invalid; return result;
                case NativeTimerDeleteOperationPreparationStatus::operationRepositoryError:
                case NativeTimerDeleteOperationPreparationStatus::bindingRepositoryError:
                    return result;
            }
            return result;
        });

    PublicApiRuntime::instance().registerNativeTimerDetailLookup(
        [this](const PublicNativeTimerDetailRequest& request)
        {
            using S = PublicNativeTimerDetailStatus;
            using P = vdrsuite::timers::NativeTimerPublicDetailRevisionStatus;
            PublicNativeTimerDetailResult result;
            if (!backendRegistryService_ || !vdrSnapshotReadService_ ||
                !nativeTimerBindingRepository_ || !backendAgentRepository_)
                return result;

            const auto backend =
                backendRegistryService_->getBackend(request.backendId);
            if (!backend || !backend->enabled || !backend->online ||
                !vdrSnapshotReadService_->hasSnapshotForBackend(request.backendId))
                return result;
            const auto agent =
                backendAgentRepository_->findAgentForBackend(request.backendId);
            if (!agent || agent->revoked || agent->incompatible ||
                agent->backendGeneration == 0U)
                return result;

            const auto timers =
                vdrSnapshotReadService_->getTimersForBackend(request.backendId);
            const auto sameId = [&request](const VdrTimer& timer) {
                return timer.id == request.timerId;
            };
            const auto timer = std::find_if(timers.begin(), timers.end(), sameId);
            if (timer == timers.end())
            {
                result.status = S::notFound;
                return result;
            }
            // A duplicated native index is not unique mutation identity.
            if (std::count_if(timers.begin(), timers.end(), sameId) != 1)
                return result;

            const auto binding = nativeTimerBindingRepository_
                ->findByBackendNativeTimer(request.backendId, request.timerId);
            const vdrsuite::timers::NativeTimerBinding* current = nullptr;
            if (binding.status ==
                vdrsuite::timers::NativeTimerBindingRepositoryStatus::ok)
                current = &binding.binding;
            else if (binding.status !=
                vdrsuite::timers::NativeTimerBindingRepositoryStatus::notFound)
                return result;

            const auto revision =
                vdrsuite::timers::nativeTimerPublicDetailRevision(
                    request.backendId, agent->backendGeneration, *timer, current);
            if (revision.status == P::unsafeEvidence)
                return result;

            result.backendId = request.backendId;
            result.timer.id = timer->id;
            result.timer.channelId = timer->channelId;
            result.timer.channelName = timer->channelName;
            result.timer.title = timer->title;
            result.timer.day = timer->day;
            result.timer.weekdays = timer->weekdays;
            result.timer.startTime = timer->startTime;
            result.timer.endTime = timer->endTime;
            result.timer.enabled = timer->enabled;
            result.timer.recording = timer->recording;
            result.timer.pending = timer->pending;
            result.resourceRevision = revision.resourceRevision;

            switch (revision.status)
            {
                case P::verifiedManaged:
                    result.ownership = "managed";
                    result.bindingState = "verified";
                    break;
                case P::verifiedAdopted:
                    result.ownership = "adopted";
                    result.bindingState = "verified";
                    break;
                case P::adoptionRequired:
                    result.ownership = "external";
                    result.bindingState = "adoption_required";
                    break;
                case P::unboundReadOnly:
                    result.ownership = "unbound";
                    result.bindingState = "binding_required";
                    break;
                case P::unsafeEvidence:
                    return PublicNativeTimerDetailResult{};
            }
            result.status = S::ok;
            return result;
        });

    PublicApiRuntime::instance().registerEpgEventDetailLookup(
        [this](const PublicEpgEventDetailRequest& request)
        {
            PublicEpgEventDetailResult result;
            if (!backendRegistryService_ || !vdrSnapshotReadService_ ||
                !backendAgentRepository_ || !epgCacheServiceRegistry_)
                return result;
            const auto backend =
                backendRegistryService_->getBackend(request.backendId);
            if (!backend || !backend->enabled || !backend->online ||
                !vdrSnapshotReadService_->hasSnapshotForBackend(request.backendId))
                return result;
            const auto agent =
                backendAgentRepository_->findAgentForBackend(request.backendId);
            if (!agent || agent->revoked || agent->incompatible ||
                agent->backendGeneration == 0U)
                return result;
            const auto channels =
                vdrSnapshotReadService_->getChannelsForBackend(request.backendId);
            const auto channel = std::find_if(channels.begin(), channels.end(),
                [&request](const VdrChannel& item) {
                    return item.id == request.channelId;
                });
            if (channel == channels.end())
            {
                result.status = PublicEpgNowNextStatus::notFound;
                return result;
            }
            const EpgCacheService* cache =
                epgCacheServiceRegistry_->findService(request.backendId);
            if (!cache || !cache->getStatusForBackend(request.backendId).ready)
                return result;
            const auto event = cache->findSourceEventForBackend(
                request.backendId, request.channelId, request.eventId);
            if (!event)
            {
                result.status = PublicEpgNowNextStatus::notFound;
                return result;
            }
            if (event->id != request.eventId ||
                event->channelId != request.channelId ||
                event->title.empty() || event->title.size() > 512U ||
                event->subtitle.size() > 512U ||
                event->description.size() > 1536U ||
                event->startTime.empty() || event->endTime.empty() ||
                event->startTime.size() > 20U || event->endTime.size() > 20U ||
                event->durationSeconds < 0)
                return result;
            // SHA-256 digest is opaque. Every changing event field and the
            // backend generation are bound as length-delimited inputs.
            std::string fingerprint;
            const auto append = [&fingerprint](const std::string& item) {
                fingerprint += std::to_string(item.size()) + ":" + item;
            };
            append("public-epg-event/1");
            append(request.backendId);
            append(request.channelId);
            append(request.eventId);
            append(std::to_string(agent->backendGeneration));
            append(event->title);
            append(event->subtitle);
            append(event->description);
            append(event->startTime);
            append(event->endTime);
            append(std::to_string(event->durationSeconds));
            const std::string digest =
                vdrsuite::agent::backendAgentNativeTimerDeleteFingerprintToken(
                    fingerprint);
            if (digest.empty()) return result;
            result.resourceRevision = "epg1:" + digest;
            result.backendId = request.backendId;
            result.channelId = request.channelId;
            result.eventId = request.eventId;
            result.title = event->title;
            result.subtitle = event->subtitle;
            result.description = event->description;
            result.startTime = event->startTime;
            result.endTime = event->endTime;
            result.durationSeconds = event->durationSeconds;
            result.status = PublicEpgNowNextStatus::ok;
            return result;
        });

    // Replay is a read-only actor/backend/idempotency lookup and must remain
    // available after the original EPG airing expires or a Backend goes down.
    // It never invents a new Timer and never bypasses Device authorization.
    vdrsuite::public_epg_timer::EpgTimerIntentAdmissionRepository
        epgTimerAdmissionRepository(database_);
    if (!epgTimerAdmissionRepository.ensureSchema())
    {
        std::cerr << "failed to initialize EPG TimerIntent replay schema"
                  << std::endl;
        return false;
    }
    PublicApiRuntime::instance().registerEpgTimerIntentReplayLookup(
        [this](const PublicEpgTimerIntentAdmissionRequest& request)
        {
            PublicEpgTimerIntentAdmissionResult result;
            if (!mutationOperationReadService_)
                return result;
            vdrsuite::public_epg_timer::EpgTimerIntentAdmissionRepository
                repository(database_);
            const auto record = repository.lookup(
                request.actorRef, request.selection);
            using R = vdrsuite::public_epg_timer::AdmissionReplayStatus;
            if (record.status == R::absent)
            {
                result.status = PublicEpgTimerIntentAdmissionStatus::notFound;
                return result;
            }
            if (record.status == R::conflict)
            {
                result.status =
                    PublicEpgTimerIntentAdmissionStatus::idempotencyConflict;
                return result;
            }
            if (record.status != R::same)
                return result;
            const auto found = mutationOperationReadService_->findForActor(
                record.operationId, request.actorRef);
            if (!found.ok() ||
                found.operation.backendId != request.selection.backendId ||
                found.operation.actionFamily != "timer.create")
                return result;
            result.status = PublicEpgTimerIntentAdmissionStatus::replayed;
            result.operation.operationId = found.operation.operationId;
            result.operation.state =
                vdrsuite::operations::mutationOperationStateName(
                    found.operation.state);
            result.operation.backendId = found.operation.backendId;
            result.operation.resourceRevision =
                found.operation.operationRevision;
            return result;
        });

    // Slice 2a: server-owned current EPG identity resolution. This can
    // prepare a native one-shot TimerIntent, but does not persist or dispatch
    // anything: a separate durable submit callback is intentionally absent
    // until atomic TimerIntent -> Assignment -> Operation admission is ready.
    PublicApiRuntime::instance().registerEpgTimerIntentObservationLookup(
        [this](const PublicEpgTimerIntentAdmissionRequest& request)
        {
            PublicEpgTimerIntentObservationResult result;
            if (!backendRegistryService_ || !backendAgentRepository_ ||
                !backendAccessPolicy_ || !vdrSnapshotReadService_ ||
                !epgCacheServiceRegistry_)
                return result;

            const auto& selection = request.selection;
            // The established public EPG detail owner checks online backend,
            // live agent, source-cache readiness, native channel and event ID.
            const auto detail = PublicApiRuntime::instance().lookupEpgEventDetail(
                {selection.backendId, selection.channelId, selection.eventId});
            if (detail.status == PublicEpgNowNextStatus::notFound)
            {
                result.status = PublicEpgTimerIntentObservationStatus::notFound;
                return result;
            }
            if (detail.status != PublicEpgNowNextStatus::ok ||
                detail.backendId != selection.backendId ||
                detail.channelId != selection.channelId ||
                detail.eventId != selection.eventId ||
                detail.resourceRevision.empty())
                return result;

            const auto agent = backendAgentRepository_->findAgentForBackend(
                selection.backendId);
            if (!agent || agent->revoked || agent->incompatible ||
                agent->backendGeneration == 0U)
                return result;

            // Public read wire times are Unix seconds; conversion must never
            // overflow or fall back to the client's asserted schedule.
            const auto seconds = [](const std::string& raw,
                std::int64_t& value) {
                if (raw.empty() || raw.size() > 12U) return false;
                value = 0;
                for (unsigned char c : raw)
                {
                    if (c < '0' || c > '9') return false;
                    value = value * 10 + (c - '0');
                }
                return value > 0;
            };
            std::int64_t begin = 0, end = 0;
            if (!seconds(detail.startTime, begin) ||
                !seconds(detail.endTime, end) || end <= begin)
                return result;

            const auto& backendId = selection.backendId;
            const auto backend = backendRegistryService_->getBackend(backendId);
            if (!backend || !backend->enabled || !backend->online)
                return result;
            const auto access = backendAccessPolicy_->canWriteToBackend(
                *backendRegistryService_, backendId);

            auto& observation = result.observation;
            observation.backendId = backendId;
            observation.channelId = selection.channelId;
            observation.nativeEventId = selection.eventId;
            // The native EPG cache is scoped by exactly one Backend. Never
            // promote that source-qualified airing to a canonical programme.
            observation.sourceId = backendId;
            observation.resourceEtag = vdrsuite::http::publicStrongEntityTag(
                detail.resourceRevision);
            observation.startTime = begin;
            observation.endTime = end;
            observation.backendGeneration = agent->backendGeneration;
            observation.exactNativeIdentity = true;
            observation.observationCurrent = true;
            result.backendAllowsMutation = access.allowed;
            result.status = PublicEpgTimerIntentObservationStatus::ok;
            return result;
        });


    // Public Device-v1 EPG CREATE reuses the Phase-69 TimerIntent, assignment,
    // native CREATE Operation, Agent dispatcher and authoritative readback.
    // Nothing below performs native CREATE: CLOCK and NTPREVIEW are strictly
    // read-only. If any live backend evidence is absent, admit nothing.
    PublicApiRuntime::instance().registerEpgTimerIntentSubmit(
        [this](const PublicEpgTimerIntentAdmissionRequest& request,
            const vdrsuite::public_epg_timer::AuthoritativeObservation& observed)
        {
            using namespace vdrsuite::public_epg_timer;
            using namespace vdrsuite::timers;
            using namespace vdrsuite::agent;
            PublicEpgTimerIntentAdmissionResult result;
            if (!backendRegistryService_ || !backendAccessPolicy_ ||
                !backendAgentRepository_ || !backendAgentCommandRepository_ ||
                !vdrSnapshotReadService_ || !timerIntentRepository_ ||
                !timerAssignmentRepository_ || !nativeTimerBindingRepository_ ||
                !nativeTimerCreateAdmissionService_ ||
                observed.backendId != request.selection.backendId ||
                observed.channelId != request.selection.channelId ||
                observed.nativeEventId != request.selection.eventId ||
                observed.resourceEtag != request.selection.ifMatch ||
                !observed.exactNativeIdentity || !observed.observationCurrent)
                return result;

            const auto& selection = request.selection;
            const auto backend =
                backendRegistryService_->getBackend(selection.backendId);
            if (!backend || !backend->enabled || !backend->online ||
                !backendAccessPolicy_->canWriteToBackend(
                    *backendRegistryService_, selection.backendId).allowed ||
                !vdrSnapshotReadService_->hasSnapshotForBackend(
                    selection.backendId))
            {
                result.status = PublicEpgTimerIntentAdmissionStatus::backendUnavailable;
                return result;
            }

            BackendRuntimeContext* runtime = nullptr;
            for (const auto& context : backendRuntimeContexts_)
                if (context && context->backendId == selection.backendId &&
                    context->suiteBridgeTransport && context->httpClient &&
                    context->service)
                {
                    runtime = context.get();
                    break;
                }
            // Never fall back to localhost or another Backend's SuiteBridge.
            if (!runtime)
                return result;

            const auto detail = PublicApiRuntime::instance().lookupEpgEventDetail(
                {selection.backendId, selection.channelId, selection.eventId});
            if (detail.status != PublicEpgNowNextStatus::ok ||
                detail.backendId != observed.backendId ||
                detail.channelId != observed.channelId ||
                detail.eventId != observed.nativeEventId ||
                vdrsuite::http::publicStrongEntityTag(detail.resourceRevision)
                    != observed.resourceEtag ||
                detail.title.empty() || detail.title.size() > 512U ||
                detail.startTime != std::to_string(observed.startTime) ||
                detail.endTime != std::to_string(observed.endTime))
            {
                result.status = PublicEpgTimerIntentAdmissionStatus::revisionConflict;
                return result;
            }

            const auto agent = backendAgentRepository_->findAgentForBackend(
                selection.backendId);
            if (!agent || agent->backendGeneration != observed.backendGeneration)
            {
                result.status = PublicEpgTimerIntentAdmissionStatus::generationConflict;
                return result;
            }

            std::string reason;
            const auto provider = backendAgentCommandRepository_->selectLocalProvider(
                selection.backendId, agent->agentId, agent->agentInstanceId,
                agent->backendGeneration,
                kBackendAgentNativeTimerCreateAuthorityDomain,
                kBackendAgentNativeTimerCreateCapability, reason);
            if (!provider)
                return result;

            const auto nowSeconds = []() -> std::int64_t {
                return std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            };

            TimerAssignmentPlanningBackendCandidate candidate;
            candidate.backendId = selection.backendId;
            candidate.currentBackendGeneration = observed.backendGeneration;
            candidate.executionAuthorityFence =
                nativeTimerCreatePlanningAuthorityFence(*provider);
            candidate.executionAuthorityCurrent =
                !candidate.executionAuthorityFence.empty();
            candidate.state = TimerAssignmentPlanningBackendState::online;
            candidate.writeAllowed = true;

            const std::int64_t clockReceivedAt = nowSeconds();
            if (!currentNativeTimerCreateExecutionAuthority(
                    *agent, *provider, candidate, clockReceivedAt) ||
                observed.startTime <= clockReceivedAt ||
                observed.endTime <= observed.startTime ||
                observed.endTime - observed.startTime >= 24 * 3600)
                return result;

            ServerOwnedRecordingDefaults defaults;
            // EPG occurrence timestamps are canonical Unix seconds. The
            // owning VDR supplies *both* actual local date/HHMM projections;
            // never convert on the Daemon host or assume its timezone.
            defaults.timezone = "UTC";
            defaults.channelSourceType = "vdr";

            ServerOwnedNativeClockEvidence nativeClock;
            const auto clockReply =
                runtime->suiteBridgeTransport->inspectNativeTimerClockContract(
                    observed.startTime, observed.endTime);
            const auto clockAttestedAt = nowSeconds();
            if (!attestBackendNativeTimerClockReply(
                    clockReply, *agent, *provider, candidate,
                    observed.startTime, observed.endTime, clockAttestedAt,
                    nativeClock, reason))
                return result;

            NativeTimerSpecification native;
            native.channelId = observed.channelId;
            native.title = detail.title;
            native.priority = defaults.priority;
            native.lifetime = defaults.lifetimeDays;
            native.vps = defaults.vpsPreferred;
            native.enabled = true;
            if (!projectEpgBoundaryToNative(
                    observed.startTime, nativeClock.startUtcOffsetSeconds,
                    false, native.day, native.startTime))
                return result;
            std::string endDay;
            if (!projectEpgBoundaryToNative(
                    observed.endTime, nativeClock.stopUtcOffsetSeconds,
                    true, endDay, native.endTime) ||
                !nativeOneShotMatchesEpg(
                    observed, defaults, nativeClock, native) ||
                !nativeTimerSpecificationValid(native))
                return result;
            candidate.desiredNativeTimerSpecification = native;
            candidate.desiredNativeTimerSpecificationPresent = true;

            // Channel mapping is the exact VDR-native channel observed by
            // the authoritative EPG owner, not a guessed canonical mapping.
            candidate.channel.backendGeneration = observed.backendGeneration;
            candidate.channel.mappingSource = "vdr-native-epg";
            candidate.channel.mappingRevision = detail.resourceRevision;
            candidate.channel.sourceType = defaults.channelSourceType;
            candidate.channel.sourceId = observed.sourceId;
            candidate.channel.sourceChannelId = observed.channelId;
            candidate.channel.backendChannelId = observed.channelId;
            candidate.channel.current = true;

            // Native capability is sourced from the current Agent/provider
            // lease; readback uses the already-configured VDR REST inventory.
            candidate.capability.backendGeneration = observed.backendGeneration;
            candidate.capability.revision =
                "suitebridge:" + std::to_string(provider->capabilityRevision);
            candidate.capability.timerCreate = true;
            candidate.capability.timerReadback = true;
            candidate.capability.current = true;

            // These health facts require BOTH a live Agent lease and an
            // online, writable Backend with a scoped native transport.
            candidate.health.backendGeneration = observed.backendGeneration;
            candidate.health.revision =
                "agent:" + std::to_string(agent->heartbeatSequence);
            candidate.health.state = TimerAssignmentPlanningHealthState::healthy;
            candidate.health.timerWritesAvailable = true;
            candidate.health.current = true;

            BackendAgentNativeTimerCreateSpecification agentSpecification;
            agentSpecification.channelId = native.channelId;
            agentSpecification.title = native.title;
            agentSpecification.directory = native.directory;
            agentSpecification.day = native.day;
            agentSpecification.weekdays = native.weekdays;
            agentSpecification.startTime = native.startTime;
            agentSpecification.endTime = native.endTime;
            agentSpecification.priority = native.priority;
            agentSpecification.lifetime = native.lifetime;
            agentSpecification.enabled = native.enabled;
            agentSpecification.vps = native.vps;

            const auto previewReply =
                runtime->suiteBridgeTransport->inspectNativeTimerPreviewContract(
                    agentSpecification, provider->providerInstanceEpoch,
                    observed.startTime, observed.endTime);
            const std::int64_t previewReceivedAt = nowSeconds();
            SuiteBridgeNativeTimerPreviewProjection verifiedPreview;
            if (!SuiteBridgeNativeTimerCreateTransport::verifyNativePreviewReply(
                    previewReply, agentSpecification,
                    observed.startTime, observed.endTime,
                    *provider, previewReceivedAt, verifiedPreview, reason))
                return result;
            ProspectiveNativeTimerConflictProof conflictProof;
            if (!attestBackendNativeTimerPreview(
                    verifiedPreview, *agent, *provider, candidate,
                    previewReceivedAt, conflictProof))
                return result;

            AtomicAdmissionRequest admission;
            admission.selection = selection;
            admission.observation = observed;
            admission.defaults = defaults;
            admission.authenticatedActorId = request.actorRef;
            admission.authoritativeTitle = detail.title;
            admission.nativeClock = nativeClock;
            admission.conflictProof = conflictProof;
            admission.authoritativeAgent = *agent;
            admission.nativeCreateProvider = *provider;
            admission.candidate = candidate;
            admission.requestedAt = nowSeconds();
            // The Service will independently reject stale CLOCK, preview,
            // provider epochs or a backend generation mismatch.
            admission.revalidateSourceAndNativeSpecification =
                [this, selection, observed, original = detail,
                    agent = *agent, provider = *provider, candidate]() {
                    if (!backendRegistryService_ || !backendAccessPolicy_ ||
                        !backendAgentRepository_ || !backendAgentCommandRepository_ ||
                        !backendAccessPolicy_->canWriteToBackend(
                            *backendRegistryService_, selection.backendId).allowed)
                        return false;
                    const auto backendNow =
                        backendRegistryService_->getBackend(selection.backendId);
                    if (!backendNow || !backendNow->enabled || !backendNow->online)
                        return false;
                    const auto current =
                        PublicApiRuntime::instance().lookupEpgEventDetail(
                            {selection.backendId, selection.channelId,
                             selection.eventId});
                    if (current.status != PublicEpgNowNextStatus::ok ||
                        current.resourceRevision != original.resourceRevision ||
                        current.title != original.title ||
                        current.startTime != original.startTime ||
                        current.endTime != original.endTime ||
                        vdrsuite::http::publicStrongEntityTag(
                            current.resourceRevision) != observed.resourceEtag)
                        return false;
                    const auto liveAgent =
                        backendAgentRepository_->findAgentForBackend(
                            selection.backendId);
                    if (!liveAgent || liveAgent->agentId != agent.agentId ||
                        liveAgent->agentInstanceId != agent.agentInstanceId ||
                        liveAgent->backendGeneration !=
                            observed.backendGeneration)
                        return false;
                    std::string why;
                    const auto liveProvider =
                        backendAgentCommandRepository_->selectLocalProvider(
                            selection.backendId, liveAgent->agentId,
                            liveAgent->agentInstanceId,
                            liveAgent->backendGeneration,
                            kBackendAgentNativeTimerCreateAuthorityDomain,
                            kBackendAgentNativeTimerCreateCapability, why);
                    if (!liveProvider ||
                        !backendAgentLocalProviderSameFence(*liveProvider, provider))
                        return false;
                    // Recheck the CURRENT Agent and provider, not only the
                    // pre-preview copy. A revoke, expired lease, stale
                    // heartbeat or capability loss during CLOCK/preview must
                    // not be admitted as a fresh TimerIntent/Assignment.
                    const auto validatedAt =
                        std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
                    return currentNativeTimerCreateExecutionAuthority(
                        *liveAgent, *liveProvider, candidate, validatedAt);
                };

            TimerIntentApplicationService application(
                *timerIntentRepository_, *timerAssignmentRepository_,
                *nativeTimerBindingRepository_);
            EpgTimerIntentAtomicAdmissionService coordinator(
                database_, *timerIntentRepository_, *timerAssignmentRepository_,
                application, *nativeTimerCreateAdmissionService_);
            const auto saved = coordinator.admit(admission);
            switch (saved.status)
            {
            case AtomicAdmissionStatus::prepared:
                result.status = PublicEpgTimerIntentAdmissionStatus::accepted;
                break;
            case AtomicAdmissionStatus::replayed:
                result.status = PublicEpgTimerIntentAdmissionStatus::replayed;
                break;
            case AtomicAdmissionStatus::invalid:
                result.status = PublicEpgTimerIntentAdmissionStatus::invalid;
                return result;
            case AtomicAdmissionStatus::revisionConflict:
                result.status = PublicEpgTimerIntentAdmissionStatus::revisionConflict;
                return result;
            case AtomicAdmissionStatus::generationConflict:
                result.status = PublicEpgTimerIntentAdmissionStatus::generationConflict;
                return result;
            case AtomicAdmissionStatus::duplicateIntent:
            case AtomicAdmissionStatus::stateConflict:
                result.status = PublicEpgTimerIntentAdmissionStatus::stateConflict;
                return result;
            case AtomicAdmissionStatus::idempotencyConflict:
                result.status = PublicEpgTimerIntentAdmissionStatus::idempotencyConflict;
                return result;
            case AtomicAdmissionStatus::backendUnavailable:
                result.status = PublicEpgTimerIntentAdmissionStatus::backendUnavailable;
                return result;
            case AtomicAdmissionStatus::storageError:
                return result;
            }
            result.operation.operationId = saved.operation.operationId;
            result.operation.state =
                vdrsuite::operations::mutationOperationStateName(
                    saved.operation.state);
            result.operation.backendId = saved.operation.backendId;
            result.operation.resourceRevision = saved.operation.operationRevision;
            return result;
        });

    PublicApiRuntime::instance().registerEpgWindowLookup(
        [this](const PublicEpgWindowRequest& request)
        {
            PublicEpgWindowResult result;
            if (!backendRegistryService_ || !vdrSnapshotReadService_ ||
                !epgCacheServiceRegistry_ ||
                request.backendId.empty() || request.channelId.empty() ||
                request.limit == 0U || request.limit > 192U ||
                (request.afterStartTime.empty() !=
                 request.afterCacheEventId.empty()))
            {
                result.status = PublicEpgNowNextStatus::invalid;
                return result;
            }
            const auto backend =
                backendRegistryService_->getBackend(request.backendId);
            if (!backend.has_value() || !backend->enabled ||
                !backend->online ||
                !vdrSnapshotReadService_
                    ->hasSnapshotForBackend(request.backendId))
                return result;
            const auto channels =
                vdrSnapshotReadService_
                    ->getChannelsForBackend(request.backendId);
            const auto channel = std::find_if(
                channels.begin(), channels.end(),
                [&request](const VdrChannel& item) {
                    return item.id == request.channelId;
                });
            if (channel == channels.end())
            {
                result.status = PublicEpgNowNextStatus::notFound;
                return result;
            }
            EpgCacheService* service =
                epgCacheServiceRegistry_->findService(request.backendId);
            if (service == nullptr ||
                !service->getStatusForBackend(request.backendId).ready)
                return result;
            if (!request.afterStartTime.empty() &&
                !service->hasWindowCursorAnchorForBackend(
                    request.backendId, request.channelId,
                    request.fromTime, request.untilTime,
                    request.afterStartTime, request.afterCacheEventId))
            {
                result.status = PublicEpgNowNextStatus::invalid;
                return result;
            }
            const auto events = service->findWindowForBackend(
                request.backendId, request.channelId, request.fromTime,
                request.untilTime, static_cast<int>(request.limit + 1U),
                request.afterStartTime, request.afterCacheEventId);
            result.hasMore = events.size() > request.limit;
            if (result.hasMore)
            {
                result.nextStartTime = events[request.limit - 1U].startTime;
                result.nextCacheEventId = events[request.limit - 1U].id;
            }
            for (std::size_t i = 0U;
                 i < events.size() && i < request.limit; ++i)
            {
                const VdrEvent& event = events[i];
                if (event.channelId != request.channelId ||
                    event.title.empty() || event.startTime.empty() ||
                    event.endTime.empty() || event.durationSeconds < 0)
                    return PublicEpgWindowResult{};
                PublicEpgNowNextItem item;
                item.channelId = event.channelId;
                // Source-ID evidence must survive the EPG cache boundary.
                // Legacy rows and generated title/time cache keys stay
                // usable for the grid but cannot be recorded as native IDs.
                if (!event.id.empty() && event.id.size() <= 160U &&
                    service->hasSourceEventIdForBackend(
                        request.backendId, request.channelId, event.id))
                    item.eventId = event.id;
                item.title = event.title;
                item.subtitle = event.subtitle;
                item.startTime = event.startTime;
                item.endTime = event.endTime;
                item.durationSeconds = event.durationSeconds;
                result.events.push_back(std::move(item));
            }
            result.status = PublicEpgNowNextStatus::ok;
            return result;
        });

    PublicApiRuntime::instance().registerChannelCollectionLookup(
        [this](const PublicChannelCollectionRequest& request)
        {
            PublicChannelCollectionResult result;
            if (!backendRegistryService_ ||
                !vdrSnapshotReadService_ ||
                request.backendIds.empty() ||
                request.backendIds.size() > 16U ||
                request.limit == 0U ||
                request.limit > 100U ||
                (request.afterBackendId.empty() !=
                 request.afterChannelId.empty()) ||
                request.afterChannelNumber < 0 ||
                (request.afterBackendId.empty() &&
                 request.afterChannelNumber != 0) ||
                (request.nativeChannelOrder &&
                 !request.relativeToChannelId.empty()) ||
                (!request.relativeToChannelId.empty() &&
                 (request.backendIds.size() != 1U ||
                  request.limit != 1U ||
                  !request.afterBackendId.empty() ||
                  (request.direction != "up" &&
                   request.direction != "down"))) ||
                (request.relativeToChannelId.empty() &&
                 !request.direction.empty()))
            {
                result.status =
                    PublicChannelCollectionStatus::invalid;
                return result;
            }

            std::size_t successfulSources = 0U;
            for (const std::string& backendId : request.backendIds)
            {
                PublicChannelCollectionSource source;
                source.backendId = backendId;

                const auto backend =
                    backendRegistryService_->getBackend(backendId);
                const bool readable =
                    backend.has_value() &&
                    backend->enabled &&
                    backend->online &&
                    vdrSnapshotReadService_
                        ->hasSnapshotForBackend(backendId);

                if (!readable)
                {
                    source.state = "unavailable";
                    source.code = "backend_unavailable";
                    result.sources.push_back(std::move(source));
                    continue;
                }

                std::vector<PublicChannelCollectionItem> sourceChannels;
                bool sourceValid = true;
                for (const VdrChannel& channel :
                     vdrSnapshotReadService_
                         ->getChannelsForBackend(backendId))
                {
                    if (channel.id.empty() ||
                        channel.id.size() > 256U)
                    {
                        sourceValid = false;
                        break;
                    }

                    PublicChannelCollectionItem item;
                    item.backendId = backendId;
                    item.channelId = channel.id;
                    item.channelNumber = channel.number;
                    item.name = channel.name;
                    item.provider = channel.provider;
                    item.groupName = channel.group;
                    item.radio = channel.radio;
                    item.encrypted = channel.encrypted;
                    item.enabled = channel.enabled;
                    sourceChannels.push_back(std::move(item));
                }

                std::sort(
                    sourceChannels.begin(),
                    sourceChannels.end(),
                    [](const PublicChannelCollectionItem& lhs,
                       const PublicChannelCollectionItem& rhs)
                    {
                        return lhs.channelId < rhs.channelId;
                    });

                for (std::size_t index = 1U;
                     sourceValid && index < sourceChannels.size();
                     ++index)
                {
                    if (sourceChannels[index - 1U].channelId ==
                        sourceChannels[index].channelId)
                    {
                        sourceValid = false;
                    }
                }

                if (!sourceValid)
                {
                    source.state = "unavailable";
                    source.code = "backend_unavailable";
                    result.sources.push_back(std::move(source));
                    continue;
                }

                // Uses the full, authorized VDR snapshot (never the first
                // 30 ID-sorted Public-v1 rows). The same pure projection is
                // directly exercised by the C++ regression test.
                if (!request.relativeToChannelId.empty())
                {
                    const auto neighbor = findPublicVdrChannelNeighbor(
                        sourceChannels,
                        request.relativeToChannelId,
                        request.direction);
                    if (!neighbor)
                    {
                        result.status = PublicChannelCollectionStatus::notFound;
                        return result;
                    }
                    result.channels.push_back(*neighbor);
                    source.state = "ok";
                    result.sources.push_back(std::move(source));
                    result.status = PublicChannelCollectionStatus::ok;
                    result.hasMore = false;
                    return result;
                }

                ++successfulSources;
                source.state = "ok";
                result.sources.push_back(std::move(source));
                result.channels.insert(
                    result.channels.end(),
                    std::make_move_iterator(sourceChannels.begin()),
                    std::make_move_iterator(sourceChannels.end()));
            }

            if (successfulSources == 0U)
            {
                result.status =
                    PublicChannelCollectionStatus::
                        allSourcesUnavailable;
                result.channels.clear();
                return result;
            }

            std::sort(
                result.channels.begin(),
                result.channels.end(),
                [&request](const PublicChannelCollectionItem& lhs,
                           const PublicChannelCollectionItem& rhs)
                {
                    if (request.nativeChannelOrder)
                        return publicVdrChannelNumberLess(lhs, rhs);
                    if (lhs.backendId != rhs.backendId)
                        return lhs.backendId < rhs.backendId;
                    return lhs.channelId < rhs.channelId;
                });

            std::vector<PublicChannelCollectionItem> eligible;
            eligible.reserve(result.channels.size());
            for (PublicChannelCollectionItem& channel : result.channels)
            {
                const bool afterCursor =
                    request.afterBackendId.empty() ||
                    channel.backendId > request.afterBackendId ||
                    (channel.backendId == request.afterBackendId &&
                     (request.nativeChannelOrder ?
                        (channel.channelNumber > request.afterChannelNumber ||
                         (channel.channelNumber == request.afterChannelNumber &&
                          channel.channelId > request.afterChannelId)) :
                        channel.channelId > request.afterChannelId));
                if (afterCursor)
                    eligible.push_back(std::move(channel));
            }

            result.hasMore = eligible.size() > request.limit;
            if (result.hasMore)
                eligible.resize(request.limit);
            result.channels = std::move(eligible);
            result.status = PublicChannelCollectionStatus::ok;
            return result;
        });

    PublicApiRuntime::instance().registerTimerCreateAdmission(
        [this](
            const PublicTimerCreateAdmissionRequest& request)
        {
            PublicTimerCreateAdmissionResult result;

            if (!backendRegistryService_ ||
                !backendAccessPolicy_ ||
                !nativeTimerCreateAdmissionService_)
            {
                result.status =
                    PublicTimerCreateAdmissionStatus::serviceUnavailable;
                return result;
            }

            const BackendAccessDecision access =
                backendAccessPolicy_->canWriteToBackend(
                    *backendRegistryService_,
                    request.backendId);

            if (!access.allowed)
            {
                result.status = access.readOnly
                    ? PublicTimerCreateAdmissionStatus::readOnlyBackend
                    : PublicTimerCreateAdmissionStatus::backendUnavailable;
                return result;
            }

            const std::int64_t requestedAt =
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now()
                        .time_since_epoch()).count();
            if (requestedAt <= 0)
            {
                result.status =
                    PublicTimerCreateAdmissionStatus::serviceUnavailable;
                return result;
            }

            vdrsuite::timers::NativeTimerCreateAdmissionRequest
                admissionRequest;
            admissionRequest.actorId = request.actorRef;
            admissionRequest.idempotencyKey =
                request.idempotencyKey;
            admissionRequest.timerAssignmentId =
                request.timerAssignmentId;
            admissionRequest.expectedAssignmentRevision =
                request.expectedAssignmentRevision;
            admissionRequest.expectedBackendId =
                request.backendId;
            admissionRequest.requestedAt = requestedAt;

            const auto admitted =
                nativeTimerCreateAdmissionService_->admit(
                    admissionRequest);

            switch (admitted.status)
            {
                case vdrsuite::timers::NativeTimerCreateAdmissionStatus::prepared:
                    result.status =
                        PublicTimerCreateAdmissionStatus::accepted;
                    break;

                case vdrsuite::timers::NativeTimerCreateAdmissionStatus::replayed:
                    result.status =
                        PublicTimerCreateAdmissionStatus::replayed;
                    break;

                case vdrsuite::timers::NativeTimerCreateAdmissionStatus::assignmentNotFound:
                    result.status =
                        PublicTimerCreateAdmissionStatus::notFound;
                    return result;

                case vdrsuite::timers::NativeTimerCreateAdmissionStatus::assignmentRevisionConflict:
                    result.status =
                        PublicTimerCreateAdmissionStatus::revisionConflict;
                    return result;

                case vdrsuite::timers::NativeTimerCreateAdmissionStatus::generationConflict:
                    result.status =
                        PublicTimerCreateAdmissionStatus::generationConflict;
                    return result;

                case vdrsuite::timers::NativeTimerCreateAdmissionStatus::idempotencyConflict:
                    result.status =
                        PublicTimerCreateAdmissionStatus::idempotencyConflict;
                    return result;

                case vdrsuite::timers::NativeTimerCreateAdmissionStatus::operationConflict:
                    result.status =
                        PublicTimerCreateAdmissionStatus::operationConflict;
                    return result;

                case vdrsuite::timers::NativeTimerCreateAdmissionStatus::assignmentStateConflict:
                case vdrsuite::timers::NativeTimerCreateAdmissionStatus::backendConflict:
                case vdrsuite::timers::NativeTimerCreateAdmissionStatus::specificationUnavailable:
                case vdrsuite::timers::NativeTimerCreateAdmissionStatus::intentRevisionConflict:
                    result.status =
                        PublicTimerCreateAdmissionStatus::stateConflict;
                    return result;

                case vdrsuite::timers::NativeTimerCreateAdmissionStatus::invalid:
                    result.status =
                        PublicTimerCreateAdmissionStatus::invalid;
                    return result;

                case vdrsuite::timers::NativeTimerCreateAdmissionStatus::identityGenerationFailed:
                case vdrsuite::timers::NativeTimerCreateAdmissionStatus::repositoryError:
                    result.status =
                        PublicTimerCreateAdmissionStatus::serviceUnavailable;
                    return result;
            }

            result.operation.operationId =
                admitted.operation.operationId;
            result.operation.state =
                vdrsuite::operations::mutationOperationStateName(
                    admitted.operation.state);
            result.operation.backendId =
                admitted.operation.backendId;
            result.operation.resourceRevision =
                admitted.operation.operationRevision;
            return result;
        });

 
    apiRouter_ = std::make_unique<ApiRouter>(
        *dashboardController_,
        *jobsController_,
        *recordingsController_,
        *metadataController_,
        *vdrController_,
        *vdrRecordingQueryController_,
        *vdrSnapshotReadService_,
        epgController_.get(),
        personController_.get(),
        recordingPersonSearchController_.get(),
        *backendRegistryController_,
        *capabilityController_,
        *recordingActionValidationController_,
        *recordingActionExecutionController_,
        *recordingActionPreviewController_,
        *vdrTimerActionController_,
        *vdrTimerActionExecutorAdapterRegistry_,
        *runtimeDiagnosticsController_,
        *snapshotChangeFeedController_,
        searchTimerController_.get(),
        *liveTransportController_,
        searchTimerRuntimeMutationPolicyExecutor_.get(),
        epgSearchNativeFuzzyStaleProbeAdministrationController_.get(),
        epgSearchNativeFuzzyOperatorRefreshController_.get(),
        searchTimerDiscoveryController_.get(),
        searchTimerAutomationPreviewController_.get(),
        searchTimerPreviewEpgCacheRefreshController_.get(),
        epgCacheController_.get(),
        vdrChannelMoveController_.get(),
        vdrRecordingFolderController_.get());

    apiRouter_->setSearchTimerPreviewEpgCache(
        searchTimerPreviewEpgCache_.get());

    // Public-v1 MOVE destinations are looked up through the one existing
    // backend-scoped public fld1_ browse index; never accept native paths.
    apiRouter_->setPublicRecordingFolderResolver(
        [this](const std::string& backendId,
               const std::string& publicFolderId,
               std::string& logicalFolder) -> bool {
            if (!vdrRecordingCacheRepository_)
                return false;
            VdrRecordingFolderPage page;
            if (!vdrRecordingCacheRepository_->
                    folderPageForBackendByPublicId(
                        backendId, publicFolderId, 1, 0, page) ||
                !page.cacheReady || page.backendId != backendId)
                return false;
            logicalFolder = page.path;
            return true;
        });

    // Reuse the existing public-ID / cache binding without trusting a
    // backend-native path or service recording ID from the client.
    apiRouter_->setPublicRecordingActionResolver(
        [this](const std::string& backendId,
               const std::string& publicId,
               VdrRecording& recording) -> bool {
            if (!publicRecordingIdentities_ || !vdrRecordingCacheRepository_)
                return false;
            PublicRecordingPlaybackTargetResolver resolver(
                *publicRecordingIdentities_, *vdrRecordingCacheRepository_);
            const auto found = resolver.resolve(backendId, publicId);
            if (found.status != PublicRecordingPlaybackTargetStatus::ready)
                return false;
            recording = found.recording;
            return true;
        });

    std::cout << "API router runtime initialized" << std::endl;
    std::cout << "SearchTimer preview EPG cache runtime initialized" << std::endl;

    // One artwork loader shared with the existing Web artwork wrapper.
    recordingArtworkService_ = std::make_shared<VdrRecordingArtworkService>(
        *vdrRecordingCacheRepository_, config_.recordingArtworkRoots());
    PublicApiRuntime::instance().registerRecordingArtworkLookup(
        [this](const PublicRecordingArtworkRequest& request)
        {
            PublicRecordingArtworkResult result;
            if (request.backendId.empty() || request.backendId == "*" ||
                request.recordingId.size() != 36U)
            {
                result.status = PublicRecordingArtworkStatus::invalid;
                return result;
            }
            if (!backendRegistryService_ || !vdrSnapshotReadService_ ||
                !publicRecordingIdentities_ || !vdrRecordingCacheRepository_ ||
                !recordingArtworkService_)
                return result;
            const auto backend =
                backendRegistryService_->getBackend(request.backendId);
            if (!backend.has_value() || !backend->enabled || !backend->online ||
                !vdrSnapshotReadService_->hasSnapshotForBackend(
                    request.backendId) ||
                vdrRecordingCacheRepository_->statusForBackend(
                    request.backendId).state != "ready")
                return result;
            PublicRecordingPlaybackTargetResolver resolver(
                *publicRecordingIdentities_, *vdrRecordingCacheRepository_);
            const auto found = resolver.resolveForRead(
                request.backendId, request.recordingId);
            if (found.status == PublicRecordingPlaybackTargetStatus::invalidRequest)
            {
                result.status = PublicRecordingArtworkStatus::invalid;
                return result;
            }
            if (found.status != PublicRecordingPlaybackTargetStatus::ready)
            {
                result.status = PublicRecordingArtworkStatus::notFound;
                return result;
            }
            for (const VdrRecordingArtworkRef& artwork :
                 found.recording.metadata.artwork)
            {
                if (!artwork.isValid())
                    continue;
                const auto id = VdrRecordingArtworkIdentity::assetId(
                    found.recording, artwork);
                if (id.empty())
                    continue;
                if (!request.artworkId.empty() && request.artworkId != id)
                    continue;
                PublicRecordingArtworkItem item;
                item.artworkId = id;
                switch (artwork.kind)
                {
                    case VdrRecordingArtworkKind::Poster: item.kind = "poster"; break;
                    case VdrRecordingArtworkKind::Fanart: item.kind = "fanart"; break;
                    case VdrRecordingArtworkKind::Banner: item.kind = "banner"; break;
                    case VdrRecordingArtworkKind::Still: item.kind = "still"; break;
                }
                item.width = artwork.width;
                item.height = artwork.height;
                result.items.push_back(std::move(item));
                if (!request.artworkId.empty())
                    break;
            }
            const auto bound = publicRecordingIdentities_->find(
                request.backendId, found.recording.backendNativeId);
            if (!bound || *bound != request.recordingId)
            {
                result.status = PublicRecordingArtworkStatus::notFound;
                return result;
            }
            if (!request.artworkId.empty())
            {
                if (result.items.empty())
                {
                    result.status = PublicRecordingArtworkStatus::notFound;
                    return result;
                }
                const auto selected = std::find_if(
                    found.recording.metadata.artwork.begin(),
                    found.recording.metadata.artwork.end(),
                    [&request, &found](const VdrRecordingArtworkRef& artwork) {
                        return artwork.isValid() &&
                            VdrRecordingArtworkIdentity::assetId(
                                found.recording, artwork) == request.artworkId;
                    });
                if (selected == found.recording.metadata.artwork.end())
                {
                    result.status = PublicRecordingArtworkStatus::notFound;
                    return result;
                }
                const auto image = recordingArtworkService_->loadPath(
                    VdrRecordingArtworkIdentity::publicUrl(
                        found.recording, *selected));
                if (!image.found())
                {
                    result.status = PublicRecordingArtworkStatus::notFound;
                    return result;
                }
                result.contentType = image.contentType;
                result.content = image.content;
            }
            result.status = PublicRecordingArtworkStatus::ok;
            return result;
        });
    auto clientHttpServer = std::make_unique<RecordingArtworkHttpServer>(
        std::make_unique<TestHttpServer>(*apiRouter_, deviceLiveEventHub_),
        recordingArtworkService_);
    httpServer_ = std::make_unique<BackendAgentHttpServer>(
        std::move(clientHttpServer),
        *backendAgentLifecycleService_,
        *backendAgentCommandDeliveryService_,
        *backendAgentRepository_,
        *backendAgentCredentialVerifierRepository_,
        *backendAgentIdentityRepository_);

    auto lastVdrPoll = std::chrono::steady_clock::now();

    httpListener_ = std::make_unique<SimpleHttpListener>(
        config_.httpListenHost(),
        config_.httpListenPort(),
        *httpServer_,
        []() {
            return shutdownRequested_.load();
        },
        [this, lastVdrPoll]() mutable {
            const auto now = std::chrono::steady_clock::now();
            const bool externalHint = externalVdrChangeHint_.exchange(false);

            if (!externalHint &&
                std::chrono::duration_cast<std::chrono::seconds>(now - lastVdrPoll).count() < 5) {
                return;
            }

            lastVdrPoll = now;
            pollVdrAndUpdateChangeFeed();
        });

    for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
        if (!backendRuntimeContext) {
            continue;
        }

        if (backendRuntimeContext->suiteBridgeAgentRuntime) {
            backendRuntimeContext->suiteBridgeAgentRuntime->start();
            const std::int64_t lifecycleNow =
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            if (!embeddedBackendLifecycleService_ ||
                !embeddedBackendLifecycleService_->startBackend(
                    backendRuntimeContext->backendId,
                    lifecycleNow)) {
                std::cerr
                    << "embedded backend lifecycle unavailable: backend="
                    << backendRuntimeContext->backendId
                    << std::endl;
            }
            std::cout
                << "Suite Bridge embedded Agent runtime started: backend="
                << backendRuntimeContext->backendId
                << std::endl;
        }

        if (backendRuntimeContext->eventStreamClient) {
            backendRuntimeContext->eventStreamClient->start();
        }
    }

    startEpgCacheWarmupWorker();
    startRecordingCacheWarmupWorker();

    std::cout << "HTTP listener runtime initialized" << std::endl;
    std::cout << "dashboard runtime initialized" << std::endl;

    std::signal(SIGINT, DaemonRuntime::handleSignal);
    std::signal(SIGTERM, DaemonRuntime::handleSignal);

    shutdownRequested_ = false;
    initialized_ = true;

    return true;
}
