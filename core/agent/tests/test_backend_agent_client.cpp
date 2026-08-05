#include "BackendAgentClient.h"
#include "BackendAgentChannelObservation.h"
#include "BackendAgentChannelObservationJson.h"
#include "BackendAgentLifecycle.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <deque>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace
{
const std::string Secret =
    "agent-client-test-secret-material-00000000000000000000000001";

class FakeTransport : public IBackendAgentControlPlaneTransport
{
public:
    std::deque<BackendAgentTransportResponse> responses;
    std::vector<std::string> paths;
    std::vector<std::string> bodies;
    std::vector<std::string> logins;
    std::vector<std::string> credentialSecrets;

    BackendAgentTransportResponse postEnrollment(
        const std::string& enrollmentId,
        const std::string& enrollmentToken,
        const std::string& path,
        const std::string& body) override
    {
        assert(enrollmentId == "enr_client");
        assert(enrollmentToken == "enrollment-client-token-material-00000000000000000001");
        paths.push_back(path);
        bodies.push_back(body);
        return pop();
    }

    BackendAgentTransportResponse postAuthenticated(
        const std::string& agentId,
        const std::string& credentialSecret,
        const std::string& path,
        const std::string& body) override
    {
        assert(agentId == "agt_client");
        assert(credentialSecret.size() >= 32);
        paths.push_back(path);
        bodies.push_back(body);
        logins.push_back(agentId);
        credentialSecrets.push_back(credentialSecret);
        return pop();
    }

private:
    BackendAgentTransportResponse pop()
    {
        assert(!responses.empty());
        BackendAgentTransportResponse response = responses.front();
        responses.pop_front();
        return response;
    }
};

BackendAgentTransportResponse success(int status, const std::string& body)
{
    return BackendAgentTransportResponse{true, status, body, {}};
}

BackendAgentTransportResponse observationSuccess(
    std::uint64_t snapshotGeneration,
    std::uint64_t producerSequence,
    const std::string& outcome = "accepted")
{
    return success(
        200,
        "{\"outcome\":\"" + outcome +
        "\",\"reasonCode\":\"backend_health_observation_test\"," +
        "\"snapshotGeneration\":" + std::to_string(snapshotGeneration) +
        ",\"producerSequence\":" + std::to_string(producerSequence) +
        ",\"lastAcceptedSequence\":" + std::to_string(producerSequence) + "}");
}

BackendAgentClientConfig configFor(const std::string& root)
{
    BackendAgentClientConfig config;
    config.controlPlaneUrl = "https://control-plane.example.test";
    config.backendId = "default";
    config.identityPath = root + "/identity";
    config.enrollmentPath = root + "/enrollment";
    config.adapters = {};
    config.observationDomains = {"backend-health"};
    config.heartbeatIntervalSeconds = 10;
    config.reconnectInitialSeconds = 1;
    config.reconnectMaximumSeconds = 8;
    return config;
}

void removeTree(const std::string& root)
{
    std::remove((root + "/identity").c_str());
    std::remove((root + "/identity-link").c_str());
    std::remove((root + "/identity.channels.pending.json").c_str());
    std::remove((root + "/channels.conf").c_str());
    std::remove((root + "/enrollment").c_str());
    std::remove((root + "/config").c_str());
    rmdir(root.c_str());
}

void test_protected_url_and_configuration()
{
    assert(CurlBackendAgentControlPlaneTransport::validProtectedControlPlaneUrl(
        "https://control-plane.example.test"));
    assert(!CurlBackendAgentControlPlaneTransport::validProtectedControlPlaneUrl(
        "http://control-plane.example.test"));
    assert(!CurlBackendAgentControlPlaneTransport::validProtectedControlPlaneUrl(
        "https://user@control-plane.example.test"));

    const std::string root = "/tmp/vdr-suite-agent-client-config";
    removeTree(root);
    assert(mkdir(root.c_str(), 0700) == 0);
    const std::string path = root + "/config";
    std::ofstream output(path);
    output << "CONTROL_PLANE_URL=https://control-plane.example.test\n"
           << "BACKEND_ID=default\n"
           << "IDENTITY_PATH=" << root << "/identity\n"
           << "ENROLLMENT_PATH=" << root << "/enrollment\n"
           << "ADAPTERS=\n"
           << "OBSERVATION_DOMAINS=backend-health\n"
           << "HEARTBEAT_INTERVAL_SECONDS=30\n";
    output.close();
    BackendAgentClientConfig config;
    std::string reason;
    assert(BackendAgentClientRuntime::loadConfig(path, config, reason));
    assert(reason == "configuration_loaded");
    assert(config.backendId == "default");
    assert(config.observationDomains.size() == 1);

    std::ofstream invalid(path);
    invalid << "CONTROL_PLANE_URL=https://control-plane.example.test\n"
            << "BACKEND_ID=default\n"
            << "IDENTITY_PATH=" << root << "/identity\n"
            << "ENROLLMENT_PATH=" << root << "/enrollment\n"
            << "ADAPTERS=channels-conf\n"
            << "OBSERVATION_DOMAINS=backend-health\n";
    invalid.close();
    assert(!BackendAgentClientRuntime::loadConfig(path, config, reason));
    assert(reason == "invalid_channel_source_configuration");
    removeTree(root);
}

void test_enrollment_connect_capability_heartbeat_and_restart()
{
    const std::string root = "/tmp/vdr-suite-agent-client-runtime";
    removeTree(root);
    assert(mkdir(root.c_str(), 0700) == 0);
    const BackendAgentClientConfig config = configFor(root);
    BackendAgentEnrollmentPackage package;
    package.enrollmentId = "enr_client";
    package.backendId = "default";
    package.enrollmentToken =
        "enrollment-client-token-material-00000000000000000001";
    std::string reason;
    assert(writeBackendAgentEnrollmentPackageAtomically(
        config.enrollmentPath, package, reason));

    FakeTransport transport;
    transport.responses.push_back(success(
        200,
        "{\"agentId\":\"agt_client\",\"backendId\":\"default\","
        "\"credentialId\":\"agc_client\",\"credentialGeneration\":1}"));
    transport.responses.push_back(success(
        200,
        "{\"agentId\":\"agt_client\",\"backendId\":\"default\","
        "\"backendGeneration\":1,\"credentialGeneration\":1,"
        "\"heartbeatSequence\":0,"
        "\"capabilityRevision\":0,\"leaseDurationSeconds\":90,"
        "\"disposition\":\"replace\"}"));
    transport.responses.push_back(success(
        200, "{\"capabilityRevision\":1,\"duplicate\":false}"));
    transport.responses.push_back(success(
        200, "{\"heartbeatSequence\":1,\"leaseExpiresAt\":123,"
             "\"duplicate\":false}"));
    transport.responses.push_back(observationSuccess(1, 1));

    BackendAgentClientRuntime runtime(config, transport);
    assert(runtime.synchronize(reason));
    assert(reason == "agent_online");
    assert(runtime.state().backendGeneration == 1);
    assert(runtime.state().heartbeatSequence == 1);
    assert(runtime.state().capabilityRevision == 1);
    assert(access(config.enrollmentPath.c_str(), F_OK) != 0);
    struct stat identityStatus{};
    assert(stat(config.identityPath.c_str(), &identityStatus) == 0);
    assert((identityStatus.st_mode & (S_IRWXG | S_IRWXO)) == 0);
    assert(transport.paths == std::vector<std::string>({
        "/api/agent/v1/enroll", "/api/agent/v1/connect",
        "/api/agent/v1/capabilities", "/api/agent/v1/heartbeat",
        "/api/agent/v1/observations/backend-health"}));
    for (const std::string& body : transport.bodies)
    {
        assert(body.find(package.enrollmentToken) == std::string::npos);
    }

    transport.responses.push_back(success(
        200, "{\"heartbeatSequence\":2,\"leaseExpiresAt\":153,"
             "\"duplicate\":false}"));
    transport.responses.push_back(observationSuccess(1, 2));
    assert(runtime.heartbeat(reason));
    assert(runtime.state().heartbeatSequence == 2);

    BackendAgentClientState persisted;
    assert(BackendAgentClientRuntime::loadIdentity(
        config.identityPath, persisted, reason));
    assert(persisted.agentId == "agt_client");
    assert(persisted.heartbeatSequence == 2);
    assert(persisted.credentialSecret.size() >= 32);

    FakeTransport restartedTransport;
    restartedTransport.responses.push_back(success(
        200,
        "{\"agentId\":\"agt_client\",\"backendId\":\"default\","
        "\"backendGeneration\":2,\"credentialGeneration\":1,"
        "\"heartbeatSequence\":0,"
        "\"capabilityRevision\":0,\"leaseDurationSeconds\":90,"
        "\"disposition\":\"replace\"}"));
    restartedTransport.responses.push_back(success(
        200, "{\"capabilityRevision\":1,\"duplicate\":false}"));
    restartedTransport.responses.push_back(success(
        200, "{\"heartbeatSequence\":1,\"leaseExpiresAt\":200,"
             "\"duplicate\":false}"));
    restartedTransport.responses.push_back(observationSuccess(2, 1));
    BackendAgentClientRuntime restarted(config, restartedTransport);
    assert(restarted.synchronize(reason));
    assert(restarted.state().backendGeneration == 2);
    assert(restarted.state().heartbeatSequence == 1);
    assert(restarted.state().capabilityRevision == 1);
    assert(restarted.agentInstanceId() != runtime.agentInstanceId());

    removeTree(root);
}

void test_credential_rotation_and_lost_response_recovery()
{
    const std::string root = "/tmp/vdr-suite-agent-client-rotation";
    removeTree(root);
    assert(mkdir(root.c_str(), 0700) == 0);
    const BackendAgentClientConfig config = configFor(root);
    BackendAgentClientState initial;
    initial.agentId = "agt_client";
    initial.backendId = "default";
    initial.credentialId = "agc_client";
    initial.credentialSecret = Secret;
    initial.credentialGeneration = 1;
    std::string reason;
    assert(BackendAgentClientRuntime::writeIdentityAtomically(
        config.identityPath, initial, reason));

    FakeTransport transport;
    transport.responses.push_back(success(
        200,
        "{\"agentId\":\"agt_client\",\"backendId\":\"default\","
        "\"backendGeneration\":1,\"credentialGeneration\":1,"
        "\"heartbeatSequence\":0,\"capabilityRevision\":0,"
        "\"leaseDurationSeconds\":90,\"disposition\":\"replace\"}"));
    transport.responses.push_back(success(
        200, "{\"capabilityRevision\":1,\"duplicate\":false}"));
    transport.responses.push_back(success(
        200, "{\"heartbeatSequence\":1,\"leaseExpiresAt\":123,"
             "\"duplicate\":false}"));
    transport.responses.push_back(observationSuccess(1, 1));
    BackendAgentClientRuntime runtime(config, transport);
    assert(runtime.synchronize(reason));
    const std::string oldSecret = runtime.state().credentialSecret;

    transport.responses.push_back(success(
        200, "{\"credentialGeneration\":2,\"idempotent\":false}"));
    transport.responses.push_back(success(
        200,
        "{\"agentId\":\"agt_client\",\"backendId\":\"default\","
        "\"backendGeneration\":1,\"credentialGeneration\":2,"
        "\"heartbeatSequence\":1,\"capabilityRevision\":1,"
        "\"leaseDurationSeconds\":90,\"disposition\":\"resume\"}"));
    transport.responses.push_back(success(
        200, "{\"heartbeatSequence\":2,\"leaseExpiresAt\":153,"
             "\"duplicate\":false}"));
    transport.responses.push_back(observationSuccess(1, 2));
    assert(runtime.rotateCredential(reason));
    assert(reason == "credential_rotated");
    assert(runtime.state().credentialGeneration == 2);
    assert(runtime.state().credentialSecret != oldSecret);
    assert(runtime.state().pendingRotationId.empty());
    const auto rotationPath = std::find(
        transport.paths.begin(), transport.paths.end(),
        "/api/agent/v1/credentials/rotate");
    assert(rotationPath != transport.paths.end());
    assert(std::find(
        transport.credentialSecrets.begin(), transport.credentialSecrets.end(),
        oldSecret) != transport.credentialSecrets.end());
    assert(std::find(
        transport.credentialSecrets.begin(), transport.credentialSecrets.end(),
        runtime.state().credentialSecret) != transport.credentialSecrets.end());

    BackendAgentClientState persisted;
    assert(BackendAgentClientRuntime::loadIdentity(
        config.identityPath, persisted, reason));
    assert(persisted.credentialGeneration == 2);
    assert(persisted.pendingRotationId.empty());

    FakeTransport ambiguousTransport;
    ambiguousTransport.responses.push_back(success(
        200,
        "{\"agentId\":\"agt_client\",\"backendId\":\"default\","
        "\"backendGeneration\":2,\"credentialGeneration\":2,"
        "\"heartbeatSequence\":0,\"capabilityRevision\":0,"
        "\"leaseDurationSeconds\":90,\"disposition\":\"replace\"}"));
    ambiguousTransport.responses.push_back(success(
        200, "{\"capabilityRevision\":1,\"duplicate\":false}"));
    ambiguousTransport.responses.push_back(success(
        200, "{\"heartbeatSequence\":1,\"leaseExpiresAt\":200,"
             "\"duplicate\":false}"));
    ambiguousTransport.responses.push_back(observationSuccess(2, 1));
    BackendAgentClientRuntime ambiguous(config, ambiguousTransport);
    assert(ambiguous.synchronize(reason));
    ambiguousTransport.responses.push_back(BackendAgentTransportResponse{
        false, 0, {}, "protected_transport_failed"});
    assert(!ambiguous.rotateCredential(reason));
    assert(reason == "protected_transport_failed");
    assert(!ambiguous.state().pendingRotationId.empty());
    const std::string pendingSecret = ambiguous.state().pendingCredentialSecret;
    const std::uint64_t pendingGeneration =
        ambiguous.state().pendingCredentialGeneration;

    BackendAgentClientState pendingPersisted;
    assert(BackendAgentClientRuntime::loadIdentity(
        config.identityPath, pendingPersisted, reason));
    assert(pendingPersisted.pendingCredentialSecret == pendingSecret);
    assert(pendingPersisted.pendingCredentialGeneration == pendingGeneration);

    FakeTransport recoveryTransport;
    recoveryTransport.responses.push_back(success(
        200,
        "{\"agentId\":\"agt_client\",\"backendId\":\"default\","
        "\"backendGeneration\":2,\"credentialGeneration\":3,"
        "\"heartbeatSequence\":1,\"capabilityRevision\":1,"
        "\"leaseDurationSeconds\":90,\"disposition\":\"resume\"}"));
    recoveryTransport.responses.push_back(success(
        200,
        "{\"agentId\":\"agt_client\",\"backendId\":\"default\","
        "\"backendGeneration\":2,\"credentialGeneration\":3,"
        "\"heartbeatSequence\":1,\"capabilityRevision\":1,"
        "\"leaseDurationSeconds\":90,\"disposition\":\"resume\"}"));
    recoveryTransport.responses.push_back(success(
        200, "{\"heartbeatSequence\":2,\"leaseExpiresAt\":230,"
             "\"duplicate\":false}"));
    recoveryTransport.responses.push_back(observationSuccess(2, 2));
    BackendAgentClientRuntime recovered(config, recoveryTransport);
    assert(recovered.synchronize(reason));
    assert(recovered.state().credentialGeneration == 3);
    assert(recovered.state().credentialSecret == pendingSecret);
    assert(recovered.state().pendingRotationId.empty());
    assert(recoveryTransport.credentialSecrets[0] == pendingSecret);
    assert(recoveryTransport.credentialSecrets[1] == pendingSecret);

    removeTree(root);
}



void test_backend_health_observation_lost_response_and_resync_recovery()
{
    const std::string root = "/tmp/vdr-suite-agent-client-observation";
    removeTree(root);
    assert(mkdir(root.c_str(), 0700) == 0);
    const BackendAgentClientConfig config = configFor(root);
    BackendAgentClientState state;
    state.agentId = "agt_client";
    state.backendId = "default";
    state.credentialId = "agc_client";
    state.credentialSecret = Secret;
    state.credentialGeneration = 1;
    std::string reason;
    assert(BackendAgentClientRuntime::writeIdentityAtomically(
        config.identityPath, state, reason));

    FakeTransport transport;
    transport.responses.push_back(success(
        200,
        "{\"agentId\":\"agt_client\",\"backendId\":\"default\","
        "\"backendGeneration\":1,\"credentialGeneration\":1,"
        "\"heartbeatSequence\":0,\"capabilityRevision\":0,"
        "\"leaseDurationSeconds\":90,\"disposition\":\"replace\"}"));
    transport.responses.push_back(success(
        200, "{\"capabilityRevision\":1,\"duplicate\":false}"));
    transport.responses.push_back(success(
        200, "{\"heartbeatSequence\":1,\"leaseExpiresAt\":123,"
             "\"duplicate\":false}"));
    transport.responses.push_back(observationSuccess(1, 1));
    BackendAgentClientRuntime runtime(config, transport);
    assert(runtime.synchronize(reason));
    assert(runtime.state().observationSnapshotGeneration == 1);
    assert(runtime.state().observationProducerSequence == 1);

    transport.responses.push_back(success(
        200, "{\"heartbeatSequence\":2,\"leaseExpiresAt\":153,"
             "\"duplicate\":false}"));
    transport.responses.push_back(BackendAgentTransportResponse{
        false, 0, {}, "protected_transport_failed"});
    assert(!runtime.heartbeat(reason));
    assert(reason == "protected_transport_failed");
    assert(runtime.state().heartbeatSequence == 2);
    assert(runtime.state().observationProducerSequence == 1);
    assert(runtime.state().pendingObservationProducerSequence == 2);

    BackendAgentClientState pending;
    assert(BackendAgentClientRuntime::loadIdentity(
        config.identityPath, pending, reason));
    assert(pending.pendingObservationProducerSequence == 2);
    assert(pending.pendingObservationHeartbeatSequence == 2);

    transport.responses.push_back(success(
        200,
        "{\"agentId\":\"agt_client\",\"backendId\":\"default\","
        "\"backendGeneration\":1,\"credentialGeneration\":1,"
        "\"heartbeatSequence\":2,\"capabilityRevision\":1,"
        "\"leaseDurationSeconds\":90,\"disposition\":\"resume\"}"));
    transport.responses.push_back(observationSuccess(1, 2, "replayed"));
    transport.responses.push_back(success(
        200, "{\"heartbeatSequence\":3,\"leaseExpiresAt\":183,"
             "\"duplicate\":false}"));
    transport.responses.push_back(observationSuccess(1, 3));
    assert(runtime.synchronize(reason));
    assert(runtime.state().pendingObservationKind.empty());
    assert(runtime.state().observationProducerSequence == 3);

    transport.responses.push_back(success(
        200, "{\"heartbeatSequence\":4,\"leaseExpiresAt\":213,"
             "\"duplicate\":false}"));
    transport.responses.push_back(success(
        409,
        "{\"error\":{\"code\":\"observation_resync_required\","
        "\"message\":\"Agent protocol request rejected\"}}"));
    transport.responses.push_back(observationSuccess(2, 1));
    assert(runtime.heartbeat(reason));
    assert(runtime.state().observationSnapshotGeneration == 2);
    assert(runtime.state().observationProducerSequence == 1);
    assert(runtime.state().pendingObservationKind.empty());

    removeTree(root);
}

void test_legacy_identity_migrates_without_observation_cursor()
{
    const std::string root = "/tmp/vdr-suite-agent-client-legacy-identity";
    removeTree(root);
    assert(mkdir(root.c_str(), 0700) == 0);
    const BackendAgentClientConfig config = configFor(root);
    {
        std::ofstream output(config.identityPath);
        output << "version=1\n"
               << "agent_id=agt_client\n"
               << "backend_id=default\n"
               << "credential_id=agc_client\n"
               << "credential_generation=1\n"
               << "credential_secret=" << Secret << "\n"
               << "backend_generation=7\n"
               << "heartbeat_sequence=11\n"
               << "capability_revision=3\n";
    }
    assert(chmod(config.identityPath.c_str(), 0600) == 0);

    BackendAgentClientState loaded;
    std::string reason;
    assert(BackendAgentClientRuntime::loadIdentity(
        config.identityPath, loaded, reason));
    assert(reason == "identity_loaded");
    assert(loaded.backendGeneration == 7);
    assert(loaded.heartbeatSequence == 11);
    assert(loaded.capabilityRevision == 3);
    assert(loaded.observationBackendGeneration == 0);
    assert(loaded.observationSnapshotGeneration == 0);
    assert(loaded.observationProducerSequence == 0);
    assert(loaded.pendingObservationKind.empty());

    assert(BackendAgentClientRuntime::writeIdentityAtomically(
        config.identityPath, loaded, reason));
    std::ifstream migrated(config.identityPath);
    const std::string contents(
        (std::istreambuf_iterator<char>(migrated)),
        std::istreambuf_iterator<char>());
    assert(contents.find("version=3\n") == 0);
    assert(contents.find("observation_backend_generation=0\n") !=
           std::string::npos);
    assert(contents.find("channel_observation_backend_generation=0\n") !=
           std::string::npos);
    removeTree(root);
}

void writeChannelsFixture(const std::string& path, bool renamed)
{
    std::ofstream output(path);
    output << ":News\n"
           << (renamed ? "RTL Zwei,RTL2;RTL World" : "RTL Television,RTL;RTL World")
           << ":12187:hC34M2O0S0:S19.2E:27500:163=2:104=deu;106=deu:105:0:12003:1:1089:0\n"
           << ":@201 Radio\n"
           << "Radio One;Provider:11000:V:S19.2E:22000:0:100=deu:0:1702:42:0:0:0\n";
    output.close();
}

void test_channel_source_and_strict_json_contract()
{
    const std::string root = "/tmp/vdr-suite-agent-client-channel-source";
    removeTree(root);
    assert(mkdir(root.c_str(), 0700) == 0);
    const std::string path = root + "/channels.conf";
    writeChannelsFixture(path, false);

    BackendAgentChannelSnapshot snapshot;
    std::string reason;
    assert(readBackendAgentChannelsConfSnapshot(path, snapshot, reason));
    assert(reason == "channels_conf_snapshot_loaded");
    assert(snapshot.channels.size() == 2);
    assert(snapshot.channels[0].channelId == "S19.2E-1-1089-12003");
    assert(snapshot.channels[0].channelNumber == 1);
    assert(snapshot.channels[0].name == "RTL Television");
    assert(snapshot.channels[0].provider == "RTL World");
    assert(snapshot.channels[0].groupName == "News");
    assert(!snapshot.channels[0].radio);
    assert(!snapshot.channels[0].encrypted);
    assert(snapshot.channels[1].channelId == "S19.2E-0-211000-42");
    assert(snapshot.channels[1].channelNumber == 201);
    assert(snapshot.channels[1].radio);
    assert(snapshot.channels[1].encrypted);
    assert(!snapshot.resourceRevision.empty());

    BackendAgentObservationRequest request;
    request.protocolVersion = "vdr-suite-agent/1";
    request.backendId = "default";
    request.agentInstanceId = "agi_channel_test";
    request.backendGeneration = 1;
    request.observationDomain = "channels";
    request.snapshotGeneration = 1;
    request.producerSequence = 1;
    request.kind = "completeSnapshot";
    request.capturedAt = 100;
    request.resourceRevision = snapshot.resourceRevision;
    request.observedHeartbeatSequence = 1;
    request.channels = snapshot.channels;
    const std::string body = serializeBackendAgentChannelObservationJson(request);
    assert(!body.empty());
    BackendAgentObservationRequest parsed;
    assert(parseBackendAgentChannelObservationJson(body, parsed, reason));
    assert(parsed.channels.size() == 2);
    assert(parsed.channels[1].channelId == snapshot.channels[1].channelId);

    const std::string unknown = body.substr(0, body.size() - 1) + ",\"unknown\":true}";
    assert(!parseBackendAgentChannelObservationJson(unknown, parsed, reason));
    assert(reason == "invalid_channel_observation_json");
    removeTree(root);
}

void test_channel_observation_publication_change_and_pending_retry()
{
    const std::string root = "/tmp/vdr-suite-agent-client-channel-runtime";
    removeTree(root);
    assert(mkdir(root.c_str(), 0700) == 0);
    BackendAgentClientConfig config = configFor(root);
    config.adapters = {"channels-conf"};
    config.observationDomains = {"channels"};
    config.channelsConfPath = root + "/channels.conf";
    writeChannelsFixture(config.channelsConfPath, false);

    BackendAgentEnrollmentPackage package;
    package.enrollmentId = "enr_client";
    package.backendId = "default";
    package.enrollmentToken =
        "enrollment-client-token-material-00000000000000000001";
    std::string reason;
    assert(writeBackendAgentEnrollmentPackageAtomically(
        config.enrollmentPath, package, reason));

    FakeTransport transport;
    transport.responses.push_back(success(
        200, "{\"agentId\":\"agt_client\",\"backendId\":\"default\","
             "\"credentialId\":\"agc_client\",\"credentialGeneration\":1}"));
    transport.responses.push_back(success(
        200, "{\"agentId\":\"agt_client\",\"backendId\":\"default\","
             "\"backendGeneration\":1,\"credentialGeneration\":1,"
             "\"heartbeatSequence\":0,\"capabilityRevision\":0,"
             "\"leaseDurationSeconds\":90,\"disposition\":\"replace\"}"));
    transport.responses.push_back(success(
        200, "{\"capabilityRevision\":1,\"duplicate\":false}"));
    transport.responses.push_back(success(
        200, "{\"heartbeatSequence\":1,\"leaseExpiresAt\":123,"
             "\"duplicate\":false}"));
    transport.responses.push_back(observationSuccess(1, 1));

    BackendAgentClientRuntime runtime(config, transport);
    assert(runtime.synchronize(reason));
    assert(runtime.state().channelObservationSnapshotGeneration == 1);
    assert(runtime.state().channelObservationProducerSequence == 1);
    assert(!runtime.state().channelObservationResourceRevision.empty());
    assert(transport.paths.back() == "/api/agent/v1/observations/channels");
    const std::string firstBody = transport.bodies.back();

    transport.responses.push_back(success(
        200, "{\"heartbeatSequence\":2,\"leaseExpiresAt\":153,"
             "\"duplicate\":false}"));
    const std::size_t pathsBeforeUnchanged = transport.paths.size();
    assert(runtime.heartbeat(reason));
    assert(transport.paths.size() == pathsBeforeUnchanged + 1);

    writeChannelsFixture(config.channelsConfPath, true);
    transport.responses.push_back(success(
        200, "{\"heartbeatSequence\":3,\"leaseExpiresAt\":183,"
             "\"duplicate\":false}"));
    transport.responses.push_back(observationSuccess(2, 1));
    assert(runtime.heartbeat(reason));
    assert(runtime.state().channelObservationSnapshotGeneration == 2);
    assert(runtime.state().channelObservationProducerSequence == 1);
    assert(transport.bodies.back() != firstBody);

    writeChannelsFixture(config.channelsConfPath, false);
    transport.responses.push_back(success(
        200, "{\"heartbeatSequence\":4,\"leaseExpiresAt\":213,"
             "\"duplicate\":false}"));
    transport.responses.push_back(BackendAgentTransportResponse{
        false, 0, {}, "protected_transport_failed"});
    assert(!runtime.heartbeat(reason));
    assert(reason == "protected_transport_failed");
    const std::string pendingPath = config.identityPath + ".channels.pending.json";
    assert(access(pendingPath.c_str(), F_OK) == 0);
    std::ifstream pendingInput(pendingPath);
    const std::string pendingBody(
        (std::istreambuf_iterator<char>(pendingInput)),
        std::istreambuf_iterator<char>());

    transport.responses.push_back(success(
        200, "{\"agentId\":\"agt_client\",\"backendId\":\"default\","
             "\"backendGeneration\":1,\"credentialGeneration\":1,"
             "\"heartbeatSequence\":4,\"capabilityRevision\":1,"
             "\"leaseDurationSeconds\":90,\"disposition\":\"resume\"}"));
    transport.responses.push_back(observationSuccess(3, 1, "replayed"));
    transport.responses.push_back(success(
        200, "{\"heartbeatSequence\":5,\"leaseExpiresAt\":243,"
             "\"duplicate\":false}"));
    assert(runtime.synchronize(reason));
    assert(access(pendingPath.c_str(), F_OK) != 0);
    assert(runtime.state().channelObservationSnapshotGeneration == 3);
    assert(runtime.state().channelObservationProducerSequence == 1);
    const auto retried = std::find(
        transport.bodies.begin(), transport.bodies.end(), pendingBody);
    assert(retried != transport.bodies.end());
    removeTree(root);
}

void test_malformed_control_plane_numbers_fail_closed()
{
    const std::string root = "/tmp/vdr-suite-agent-client-malformed";
    removeTree(root);
    assert(mkdir(root.c_str(), 0700) == 0);
    const BackendAgentClientConfig config = configFor(root);
    BackendAgentClientState state;
    state.agentId = "agt_client";
    state.backendId = "default";
    state.credentialId = "agc_client";
    state.credentialSecret = Secret;
    state.credentialGeneration = 1;
    std::string reason;
    assert(BackendAgentClientRuntime::writeIdentityAtomically(
        config.identityPath, state, reason));

    FakeTransport malformed;
    malformed.responses.push_back(success(
        200,
        "{\"agentId\":\"agt_client\",\"backendId\":\"default\","
        "\"backendGeneration\":1x,\"credentialGeneration\":1,"
        "\"heartbeatSequence\":0,\"capabilityRevision\":0,"
        "\"leaseDurationSeconds\":90,\"disposition\":\"replace\"}"));
    BackendAgentClientRuntime malformedRuntime(config, malformed);
    assert(!malformedRuntime.synchronize(reason));
    assert(reason == "invalid_connect_response");

    FakeTransport overflow;
    overflow.responses.push_back(success(
        200,
        "{\"agentId\":\"agt_client\",\"backendId\":\"default\","
        "\"backendGeneration\":9223372036854775808,"
        "\"credentialGeneration\":1,\"heartbeatSequence\":0,"
        "\"capabilityRevision\":0,\"leaseDurationSeconds\":90,"
        "\"disposition\":\"replace\"}"));
    BackendAgentClientRuntime overflowRuntime(config, overflow);
    assert(!overflowRuntime.synchronize(reason));
    assert(reason == "invalid_connect_response");
    removeTree(root);
}

void test_permissions_and_bounded_backoff()
{
    const std::string root = "/tmp/vdr-suite-agent-client-backoff";
    removeTree(root);
    assert(mkdir(root.c_str(), 0700) == 0);
    const BackendAgentClientConfig config = configFor(root);
    BackendAgentClientState state;
    state.agentId = "agt_client";
    state.backendId = "default";
    state.credentialId = "agc_client";
    state.credentialSecret = Secret;
    state.credentialGeneration = 1;
    std::string reason;
    assert(BackendAgentClientRuntime::writeIdentityAtomically(
        config.identityPath, state, reason));
    assert(chmod(config.identityPath.c_str(), 0644) == 0);
    BackendAgentClientState rejected;
    assert(!BackendAgentClientRuntime::loadIdentity(
        config.identityPath, rejected, reason));
    assert(reason == "local_file_permissions_too_open");
    assert(chmod(config.identityPath.c_str(), 0600) == 0);

    {
        std::ofstream appended(config.identityPath, std::ios::app);
        appended << "unknown_identity_key=value\n";
    }
    assert(!BackendAgentClientRuntime::loadIdentity(
        config.identityPath, rejected, reason));
    assert(reason == "unknown_identity_key");
    assert(BackendAgentClientRuntime::writeIdentityAtomically(
        config.identityPath, state, reason));
    const std::string linkPath = root + "/identity-link";
    assert(symlink(config.identityPath.c_str(), linkPath.c_str()) == 0);
    assert(!BackendAgentClientRuntime::loadIdentity(
        linkPath, rejected, reason));
    assert(reason == "local_file_not_regular");

    FakeTransport transport;
    transport.responses.push_back(BackendAgentTransportResponse{
        false, 0, {}, "protected_transport_failed"});
    transport.responses.push_back(BackendAgentTransportResponse{
        false, 0, {}, "protected_transport_failed"});
    transport.responses.push_back(BackendAgentTransportResponse{
        false, 0, {}, "protected_transport_failed"});
    std::vector<int> sleeps;
    int stopChecks = 0;
    BackendAgentClientRuntime runtime(
        config,
        transport,
        [&](int seconds) { sleeps.push_back(seconds); },
        [](const std::string&) {});
    runtime.run([&] { return stopChecks++ >= 3; });
    assert(sleeps == std::vector<int>({1, 2, 4}));
    removeTree(root);
}
}

int main()
{
    test_protected_url_and_configuration();
    test_enrollment_connect_capability_heartbeat_and_restart();
    test_credential_rotation_and_lost_response_recovery();
    test_backend_health_observation_lost_response_and_resync_recovery();
    test_legacy_identity_migrates_without_observation_cursor();
    test_channel_source_and_strict_json_contract();
    test_channel_observation_publication_change_and_pending_retry();
    test_malformed_control_plane_numbers_fail_closed();
    test_permissions_and_bounded_backoff();
    std::cout << "test_backend_agent_client passed" << std::endl;
    return 0;
}
