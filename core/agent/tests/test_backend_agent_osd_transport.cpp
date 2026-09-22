#include "SuiteBridgeSvdrpTransport.h"
#include "SuiteBridgeHandshakeService.h"
#include "BackendAgentClient.h"
#include <filesystem>
#include <fstream>
#include <unistd.h>
#include "AccountabilityEventRepository.h"
#include "BackendAgentHttpServer.h"
#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendRegistry.h"
#include "BackendRegistryService.h"
#include "CredentialVerifierRepository.h"
#include "Database.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>



namespace
{
std::int64_t testNow() { return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }
const std::string EnrollmentToken =
    "enrollment-token-material-00000000000000000000000000000001";
const std::string AgentSecret =
    "agent-runtime-secret-material-000000000000000000000000000001";
const std::string RotatedAgentSecret =
    "agent-rotated-secret-material-00000000000000000000000000001";

struct Fixture
{
    std::string path;
    Database database;
    BackendRegistry backendRegistry;
    BackendRegistryService backendRegistryService;
    BackendAgentRepository agentRepository;
    SecurityIdentityRepository identityRepository;
    SecurityIdentityProvisioningRepository provisioningRepository;
    CredentialVerifierRepository verifierRepository;
    AccountabilityEventRepository accountabilityRepository;
    BackendAgentCommandRepository commandRepository;
    BackendAgentLifecycleService service;
    BackendAgentCommandDeliveryService commandService;

    Fixture()
        : path("/tmp/vdr-suite-osd-transport-test.db"),
          backendRegistryService(backendRegistry),
          agentRepository(database),
          identityRepository(database),
          provisioningRepository(database),
          verifierRepository(database),
          accountabilityRepository(database),
          commandRepository(database),
          service(
              database,
              agentRepository,
              backendRegistryService,
              provisioningRepository,
              identityRepository,
              verifierRepository,
              accountabilityRepository),
          commandService(
              commandRepository,
              agentRepository,
              accountabilityRepository)
    {
        std::remove(path.c_str());
        assert(database.open(path));
        assert(identityRepository.ensureSchema());
        assert(verifierRepository.ensureSchema());
        assert(accountabilityRepository.ensureSchema());
        assert(agentRepository.ensureSchema());
        assert(commandRepository.ensureSchema());

        BackendNode backend;
        backend.backendId = "default";
        backend.backendName = "Default VDR";
        backend.accessMode = "read-only";
        backend.enabled = true;
        backendRegistry.addBackend(backend);

        BackendNode second;
        second.backendId = "ferienhaus";
        second.backendName = "Ferienhaus";
        second.accessMode = "read-only";
        second.enabled = true;
        backendRegistry.addBackend(second);
    }

    ~Fixture()
    {
        database.close();
        std::remove(path.c_str());
    }
};

RequestSecurityContext adminContext(const std::string& backendId = "default")
{
    RequestSecurityContext context;
    context.requestId = "request-admin";
    context.correlationId = "correlation-admin";
    context.authenticationState = AuthenticationState::Authenticated;
    context.actor = ActorIdentity{"admin", ActorType::User, "Administrator", true};
    context.grants.push_back(PermissionGrant{"role.admin", backendId});
    context.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    return context;
}

RequestSecurityContext agentContext(
    const BackendAgentRecord& agent,
    AuthenticationState state = AuthenticationState::Authenticated)
{
    RequestSecurityContext context;
    context.requestId = "request-agent";
    context.correlationId = "correlation-agent";
    context.authenticationState = state;
    context.actor = ActorIdentity{agent.actorId, ActorType::Agent, "Backend Agent", true};
    context.device = DeviceIdentity{agent.deviceId, true};
    context.credential = CredentialIdentity{agent.credentialId, true, false, false};
    context.grants.push_back(PermissionGrant{
        "backend.agent.credential.rotate", agent.backendId});
    context.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    return context;
}

std::string base64(const std::string& value)
{
    static const char Alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    for (std::size_t offset = 0; offset < value.size(); offset += 3)
    {
        const std::size_t remaining = value.size() - offset;
        const unsigned int first = static_cast<unsigned char>(value[offset]);
        const unsigned int second = remaining > 1 ? static_cast<unsigned char>(value[offset + 1]) : 0;
        const unsigned int third = remaining > 2 ? static_cast<unsigned char>(value[offset + 2]) : 0;
        const unsigned int encoded = (first << 16) | (second << 8) | third;
        output.push_back(Alphabet[(encoded >> 18) & 0x3f]);
        output.push_back(Alphabet[(encoded >> 12) & 0x3f]);
        output.push_back(remaining > 1 ? Alphabet[(encoded >> 6) & 0x3f] : '=');
        output.push_back(remaining > 2 ? Alphabet[encoded & 0x3f] : '=');
    }
    return output;
}
class RejectClient final : public IHttpServer
{
public:
    HttpServerResponse handleRequest(const HttpServerRequest&) const override
    { HttpServerResponse r; r.statusCode = 404; return r; }
};

class LoopbackTransport final : public IBackendAgentControlPlaneTransport
{
public:
    explicit LoopbackTransport(BackendAgentHttpServer& server) : server(server) {}
    BackendAgentHttpServer& server;
    bool loseOsdReply = false;
    std::string lastOsd;
    std::string lastAuthorization;
    BackendAgentTransportResponse request(const std::string& path,
        const std::string& body, const std::string& authorization)
    {
        HttpServerRequest r;
        r.method = "POST"; r.path = path; r.body = body;
        r.headers["Authorization"] = authorization;
        const auto response = server.handleRequest(r);
        if (path == "/api/agent/v1/observations/osd")
        {
            if (response.statusCode == 200) { lastOsd = body; lastAuthorization = authorization; }
            assert(response.headers.at("Cache-Control") == "no-store");
            if (loseOsdReply) return {false, 0, {}, "transport_failed"};
        }
        return {true, response.statusCode, response.body, {}};
    }
    BackendAgentTransportResponse postEnrollment(const std::string& id,
        const std::string& token, const std::string& path, const std::string& body) override
    { return request(path, body, "VDR-Suite-Enrollment " + id + ":" + token); }
    BackendAgentTransportResponse postAuthenticated(const std::string& id,
        const std::string& secret, const std::string& path, const std::string& body) override
    { return request(path, body, "Basic " + base64(id + ":" + secret)); }
};

vdrsuite::agent::SuiteBridgeOsdFrameSourceSnapshot currentFrame(
    const std::string& backend, std::uint64_t generation, std::uint64_t sequence = 1)
{
    using namespace vdrsuite::agent;
    SuiteBridgeOsdFrameSourceSnapshot s;
    s.state = SuiteBridgeOsdFrameSourceState::Current;
    s.buffer.hasFrame = true;
    s.buffer.observed.sourceConsistent = true;
    auto& f = s.buffer.observed.frame;
    f.surface = {backend, generation, "primary-native-osd", "0123456789abcdef0123456789abcdef"};
    f.state = OsdSurfaceState::Active;
    f.kind = OsdFrameKind::Menu;
    f.frameSequence = sequence;
    f.title = "PRIVATE_OSD_SENTINEL_68C";
    f.items = {{"Menü \"äöü\"\nText", true}};
    f.selectedIndex = 0;
    return s;
}

void test_codec()
{
    BackendAgentOsdObservation v;
    v.backendId = "default"; v.agentInstanceId = "instance-1";
    v.backendGeneration = 1; v.producerSequence = 1;
    v.snapshot = currentFrame(v.backendId, v.backendGeneration);
    const auto body = serializeBackendAgentOsdObservation(v);
    assert(!body.empty());
    BackendAgentOsdObservation parsed;
    assert(parseBackendAgentOsdObservation(body, parsed));
    assert(serializeBackendAgentOsdObservation(parsed) == body);
    assert(!parseBackendAgentOsdObservation(body + "{}", parsed));
    auto invalidUtf8 = body;
    invalidUtf8.insert(invalidUtf8.find("PRIVATE_OSD"), 1, static_cast<char>(0xff));
    assert(!parseBackendAgentOsdObservation(invalidUtf8, parsed));
    assert(!parseBackendAgentOsdObservation(body.substr(0, body.size() - 1), parsed));
    assert(!parseBackendAgentOsdObservation(std::string(v.MaximumBodyBytes + 1, ' '), parsed));
    auto bad = body;
    bad.replace(bad.find("\"osdObservationSchema\":1"), 24, "\"osdObservationSchema\":2");
    assert(!parseBackendAgentOsdObservation(bad, parsed));
    bad = body;
    bad.insert(1, "\"osdObservationSchema\":1,");
    assert(!parseBackendAgentOsdObservation(bad, parsed));
    v.snapshot.buffer.observed.frame.title.assign(257, 'a');
    assert(serializeBackendAgentOsdObservation(v).empty());
    v.snapshot = currentFrame("other", 1);
    assert(serializeBackendAgentOsdObservation(v).empty());
    v.snapshot = currentFrame("default", 1);
    v.snapshot.buffer.resyncRequired = true;
    assert(serializeBackendAgentOsdObservation(v).empty());
    v.snapshot.state = vdrsuite::agent::SuiteBridgeOsdFrameSourceState::ResyncRequired;
    assert(!serializeBackendAgentOsdObservation(v).empty());
}

std::string menuPayload(
    const std::string& epoch,
    std::uint64_t sequence,
    bool complete = true,
    bool consistent = true,
    std::uint64_t dropped = 0,
    const std::string& title = "Main menu")
{
    return
        "{\"osd_schema\":1"
        ",\"active\":true"
        ",\"complete\":" + std::string(complete ? "true" : "false") +
        ",\"consistent\":" + std::string(consistent ? "true" : "false") +
        ",\"kind\":\"menu\""
        ",\"frame_sequence\":" + std::to_string(sequence) +
        ",\"observed_at_ms\":123456"
        ",\"dropped_updates\":" + std::to_string(dropped) +
        ",\"osd_epoch\":\"" + epoch + "\""
        ",\"title\":\"" + title + "\""
        ",\"status\":\"Ready\""
        ",\"red\":\"Red\""
        ",\"green\":\"Green\""
        ",\"yellow\":\"Yellow\""
        ",\"blue\":\"Blue\""
        ",\"items\":["
            "{\"text\":\"Recordings\",\"selectable\":true},"
            "{\"text\":\"Setup\",\"selectable\":true}"
        "]"
        ",\"selected_index\":0"
        ",\"text\":\"\""
        ",\"channel\":\"\""
        ",\"programme\":{"
            "\"present_time\":0,"
            "\"present_title\":\"\","
            "\"present_subtitle\":\"\","
            "\"following_time\":0,"
            "\"following_title\":\"\","
            "\"following_subtitle\":\"\""
        "}}";
}

class LocalOsdTransport final : public vdrsuite::agent::ISuiteBridgeLocalTransport
{
public:
    std::function<std::string()> payload;
    vdrsuite::agent::SuiteBridgeCommandReply execute(vdrsuite::agent::SuiteBridgeLocalCommand command) override
    {
        assert(command == vdrsuite::agent::SuiteBridgeLocalCommand::OsdSnapshot);
        vdrsuite::agent::SuiteBridgeCommandReply result;
        result.transportStatus = vdrsuite::agent::SuiteBridgeTransportStatus::Success;
        result.replyCode = 900;
        result.payload = payload();
        return result;
    }
};

void test_authenticated_vertical()
{
    Fixture fixture;
    std::string reason;
    assert(fixture.service.createEnrollment(adminContext(), "enr_client", "default",
        backendAgentHashSecret(EnrollmentToken), testNow() + 600, testNow(), reason));
    BackendAgentHttpServer server(std::make_unique<RejectClient>(), fixture.service,
        fixture.commandService, fixture.agentRepository, fixture.verifierRepository,
        fixture.identityRepository);
    LoopbackTransport transport(server);
    char pattern[] = "/tmp/vdr-suite-osd-client-XXXXXX";
    const char* temporary = mkdtemp(pattern);
    assert(temporary);
    const std::string root = temporary;
    BackendAgentClientConfig config;
    config.backendId = "default";
    config.controlPlaneUrl = "https://test.invalid";
    config.identityPath = root + "/identity";
    config.enrollmentPath = root + "/enrollment";
    config.commandStatePath = root + "/commands";
    config.adapters = {"suitebridge"};
    config.observationDomains = {"backend-health", "osd"};
    {
        std::ofstream file(root + "/config");
        file << "CONTROL_PLANE_URL=https://test.invalid\nBACKEND_ID=default\n"
             << "IDENTITY_PATH=" << config.identityPath << "\nENROLLMENT_PATH=" << config.enrollmentPath
             << "\nADAPTERS=suitebridge\nOBSERVATION_DOMAINS=osd\n"
             << "SUITEBRIDGE_HOST=127.0.0.1\nSUITEBRIDGE_PORT=6419\n";
    }
    BackendAgentClientConfig parsedConfig;
    assert(BackendAgentClientRuntime::loadConfig(root + "/config", parsedConfig, reason));
    assert(parsedConfig.commandTypes.empty());
    assert(writeBackendAgentEnrollmentPackageAtomically(config.enrollmentPath,
        {"enr_client", "default", EnrollmentToken}, reason));
    std::uint64_t frameSequence = 1;
    bool unavailable = false;
    bool resync = false;
    std::string epoch = "0123456789abcdef0123456789abcdef";
    LocalOsdTransport local;
    local.payload = [&] { return menuPayload(epoch, frameSequence, !resync, !resync,
        0, "PRIVATE_OSD_SENTINEL_68C"); };
    std::unique_ptr<vdrsuite::agent::SuiteBridgeOsdFrameSource> source;
    std::uint64_t sourceGeneration = 0;
    config.osdObservationSource = [&](const std::string& backend, std::uint64_t generation) {
        if (!source || sourceGeneration != generation)
        {
            sourceGeneration = generation;
            source = std::make_unique<vdrsuite::agent::SuiteBridgeOsdFrameSource>(local, backend, generation);
        }
        vdrsuite::agent::SuiteBridgeDiscovery discovery;
        if (!unavailable) discovery.capabilities.push_back({"osd.view",
            vdrsuite::agent::SuiteBridgeCapabilityState::Available, "available"});
        return source->read(discovery);
    };
    std::vector<std::string> logs;
    BackendAgentClientRuntime runtime(config, transport, {}, [&](const std::string& s) { logs.push_back(s); });
    assert(runtime.synchronize(reason));
    assert(!transport.lastOsd.empty());
    auto viewer = adminContext();
    viewer.grants = {{"osd.view", "default"}};
    const auto generation = runtime.state().backendGeneration;
    auto read = fixture.service.readOsdObservation(viewer, "default", generation, testNow());
    assert(read.available);
    assert(read.snapshot.buffer.observed.frame.title == "PRIVATE_OSD_SENTINEL_68C");
    // These are actual server admission and internal read gates, not UI hints.
    assert(!fixture.service.readOsdObservation(adminContext(), "default", generation, testNow()).available);
    assert(!fixture.service.readOsdObservation(viewer, "ferienhaus", generation, testNow()).available);
    assert(!fixture.service.readOsdObservation(viewer, "default", generation + 1, testNow()).available);
    auto revokedViewer = viewer;
    revokedViewer.authenticationState = AuthenticationState::Revoked;
    assert(!fixture.service.readOsdObservation(revokedViewer, "default", generation, testNow()).available);
    auto agent = fixture.agentRepository.findAgent(runtime.state().agentId);
    assert(agent);
    auto machine = agentContext(*agent);
    machine.grants = {{"osd.view", "default"}};
    assert(!fixture.service.readOsdObservation(machine, "default", generation, testNow()).available);

    BackendAgentOsdObservation observation;
    assert(parseBackendAgentOsdObservation(transport.lastOsd, observation));
    assert(fixture.service.ingestOsdObservation(machine, observation, testNow()).replayed);
    assert(!fixture.service.ingestOsdObservation(viewer, observation, testNow()).accepted);
    auto bad = observation;
    bad.backendId = "ferienhaus";
    bad.snapshot.buffer.observed.frame.surface.backendId = bad.backendId;
    assert(!fixture.service.ingestOsdObservation(machine, bad, testNow()).accepted);
    bad = observation; bad.agentInstanceId = "wrong-instance";
    assert(!fixture.service.ingestOsdObservation(machine, bad, testNow()).accepted);
    bad = observation; ++bad.backendGeneration;
    bad.snapshot.buffer.observed.frame.surface.backendGeneration = bad.backendGeneration;
    assert(!fixture.service.ingestOsdObservation(machine, bad, testNow()).accepted);
    bad = observation; bad.snapshot.buffer.observed.frame.title = "conflict";
    assert(!fixture.service.ingestOsdObservation(machine, bad, testNow()).accepted);
    assert(transport.request("/api/agent/v1/observations/osd", transport.lastOsd, "").statusCode == 401);
    assert(transport.request("/api/agent/v1/observations/osd", transport.lastOsd,
        "Basic " + base64("admin:" + AgentSecret)).statusCode == 401);
    assert(transport.request("/api/agent/v1/observations/osd",
        std::string(BackendAgentOsdObservation::MaximumBodyBytes + 1, ' '), transport.lastAuthorization).statusCode == 413);

    // Lost acknowledgements are followed by a new full observation, never a disk queue.
    transport.loseOsdReply = true;
    frameSequence = 17;
    assert(runtime.heartbeat(reason));
    assert(fixture.service.readOsdObservation(viewer, "default", generation, testNow()).snapshot.buffer.observed.frame.frameSequence == 17);
    transport.loseOsdReply = false;
    assert(!fixture.service.ingestOsdObservation(machine, observation, testNow()).accepted);
    unavailable = true;
    assert(runtime.heartbeat(reason));
    read = fixture.service.readOsdObservation(viewer, "default", generation, testNow());
    assert(!read.available && !read.snapshot.buffer.hasFrame);
    assert(transport.lastOsd.find("PRIVATE_OSD") == std::string::npos);
    unavailable = false; resync = true; ++frameSequence;
    assert(runtime.heartbeat(reason));
    read = fixture.service.readOsdObservation(viewer, "default", generation, testNow());
    assert(!read.available && read.reasonCode == "osd_resync_required");
    resync = false; ++frameSequence;
    assert(runtime.heartbeat(reason));
    assert(!fixture.service.readOsdObservation(viewer, "default", generation, testNow()).available);
    epoch = "abcdef0123456789abcdef0123456789";
    frameSequence = 1;
    assert(runtime.heartbeat(reason));
    read = fixture.service.readOsdObservation(viewer, "default", generation, testNow());
    assert(read.available && read.snapshot.buffer.observed.frame.surface.osdEpoch == epoch);
    assert(!fixture.service.ingestOsdObservation(machine, observation, testNow()).accepted);
    assert(!fixture.service.observationCursorForBackend("default", "osd").present);
    for (const auto& log : logs) assert(log.find("PRIVATE_OSD") == std::string::npos);
    BackendAgentClientRuntime restarted(config, transport);
    assert(restarted.synchronize(reason));
    const auto newGeneration = restarted.state().backendGeneration;
    assert(newGeneration != generation);
    assert(!fixture.service.readOsdObservation(viewer, "default", generation, testNow()).available);
    assert(fixture.service.readOsdObservation(viewer, "default", newGeneration, testNow()).available);
    assert(!fixture.service.ingestOsdObservation(machine, observation, testNow()).accepted);
    // Freshness and lease expiry fence reads without waiting for another publication.
    assert(!fixture.service.readOsdObservation(viewer, "default", newGeneration, testNow() + 61).available);
    assert(restarted.heartbeat(reason));
    assert(!fixture.service.readOsdObservation(viewer, "default", newGeneration, testNow() + 91).available);
    assert(restarted.heartbeat(reason));
    assert(fixture.service.revoke(adminContext(), runtime.state().agentId, "test_revocation", testNow(), reason));
    assert(!fixture.service.readOsdObservation(viewer, "default", generation, testNow()).available);
    assert(transport.request("/api/agent/v1/observations/osd", transport.lastOsd,
        transport.lastAuthorization).statusCode == 401);
    for (const auto& item : std::filesystem::directory_iterator(root))
    {
        std::ifstream stream(item.path());
        std::string contents((std::istreambuf_iterator<char>(stream)), {});
        assert(contents.find("PRIVATE_OSD") == std::string::npos);
    }
    std::ifstream db(fixture.path, std::ios::binary);
    std::string contents((std::istreambuf_iterator<char>(db)), {});
    assert(contents.find("PRIVATE_OSD") == std::string::npos);
    std::filesystem::remove_all(root);
}
void test_live_source()
{
    using namespace vdrsuite::agent;
    Fixture fixture;
    std::string reason;
    assert(fixture.service.createEnrollment(adminContext(), "enr_live", "default",
        backendAgentHashSecret(EnrollmentToken), testNow() + 600, testNow(), reason));
    BackendAgentHttpServer server(std::make_unique<RejectClient>(), fixture.service,
        fixture.commandService, fixture.agentRepository, fixture.verifierRepository,
        fixture.identityRepository);
    LoopbackTransport transport(server);
    char pattern[] = "/tmp/vdr-suite-osd-live-XXXXXX";
    const char* temporary = mkdtemp(pattern);
    assert(temporary);
    const std::string root = temporary;
    BackendAgentClientConfig config;
    config.backendId = "default";
    config.controlPlaneUrl = "https://test.invalid";
    config.identityPath = root + "/identity";
    config.enrollmentPath = root + "/enrollment";
    config.commandStatePath = root + "/commands";
    config.adapters = {"suitebridge"};
    config.observationDomains = {"osd"};
    assert(writeBackendAgentEnrollmentPackageAtomically(config.enrollmentPath,
        {"enr_live", "default", EnrollmentToken}, reason));
    SuiteBridgeSvdrpTransportConfig localConfig;
    localConfig.host = "127.0.0.1";
    localConfig.port = 6419;
    SuiteBridgeSvdrpTransport local(localConfig);
    std::unique_ptr<SuiteBridgeOsdFrameSource> source;
    config.osdObservationSource = [&](const std::string& backend, std::uint64_t generation) {
        if (!source) source = std::make_unique<SuiteBridgeOsdFrameSource>(local, backend, generation);
        SuiteBridgeHandshakeService handshake(local);
        const auto discovered = handshake.discover();
        assert(discovered.compatible());
        assert(discovered.discovery.capabilityAvailable("osd.view"));
        assert(discovered.discovery.capabilityAvailable("osd.control"));
        return source->read(discovered.discovery);
    };
    BackendAgentClientRuntime runtime(config, transport);
    assert(runtime.synchronize(reason));
    auto viewer = adminContext(); viewer.grants = {{"osd.view", "default"}};
    const auto read = fixture.service.readOsdObservation(viewer, "default",
        runtime.state().backendGeneration, testNow());
    assert(read.available);
    assert(read.snapshot.buffer.observed.frame.fullFrame);
    assert(read.snapshot.buffer.observed.sourceConsistent);
    assert(!read.snapshot.buffer.resyncRequired);
    assert(!fixture.service.observationCursorForBackend("default", "osd").present);
    std::filesystem::remove_all(root);
    std::cout << "RESULT=PHASE68C_REAL_SOURCE_AUTHENTICATED_RECEIVER_PASS\n";
}
}

int main(int argc, char** argv)
{
    if (argc == 2 && std::string(argv[1]) == "--live")
    {
        test_live_source();
        return 0;
    }
    assert(argc == 1);
    test_codec();
    test_authenticated_vertical();
    std::cout << "RESULT=PHASE68C_AUTHENTICATED_OSD_TRANSPORT_PASS\n";
}
