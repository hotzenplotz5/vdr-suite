#include "BackendRegistryJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

static BackendNode makeBackend(
    const std::string& backendId,
    const std::string& backendName,
    bool enabled,
    bool online,
    const std::string& accessMode = "read-write")
{
    BackendNode backend;

    backend.backendId = backendId;
    backend.backendName = backendName;
    backend.backendType = "vdr";
    backend.accessMode = accessMode;
    backend.enabled = enabled;
    backend.online = online;

    return backend;
}

static void test_serializer_serializes_single_backend()
{
    BackendNode backend =
        makeBackend("default", "Default VDR", true, false);

    BackendRegistryJsonSerializer serializer;

    const std::string json =
        serializer.serializeBackend(backend);

    assert(json.find("\"backendId\":\"default\"") != std::string::npos);
    assert(json.find("\"backendName\":\"Default VDR\"") != std::string::npos);
    assert(json.find("\"backendType\":\"vdr\"") != std::string::npos);
    assert(json.find("\"accessMode\":\"read-write\"") != std::string::npos);
    assert(json.find("\"readOnly\":false") != std::string::npos);
    assert(json.find("\"enabled\":true") != std::string::npos);
    assert(json.find("\"online\":false") != std::string::npos);
}

static void test_serializer_serializes_read_only_backend()
{
    BackendNode backend =
        makeBackend("remote-house", "Remote House VDR", true, true, "read-only");

    BackendRegistryJsonSerializer serializer;

    const std::string json =
        serializer.serializeBackend(backend);

    assert(json.find("\"backendId\":\"remote-house\"") != std::string::npos);
    assert(json.find("\"accessMode\":\"read-only\"") != std::string::npos);
    assert(json.find("\"readOnly\":true") != std::string::npos);
}

static void test_serializer_serializes_backend_list()
{
    std::vector<BackendNode> backends;
    backends.push_back(
        makeBackend("default", "Default VDR", true, false));
    backends.push_back(
        makeBackend("ferienhaus", "Ferienhaus VDR", true, true, "read-only"));

    BackendRegistryJsonSerializer serializer;

    const std::string json =
        serializer.serializeBackends(backends);

    assert(json.find("\"backends\":[") != std::string::npos);
    assert(json.find("\"backendId\":\"default\"") != std::string::npos);
    assert(json.find("\"backendId\":\"ferienhaus\"") != std::string::npos);
    assert(json.find("\"accessMode\":\"read-write\"") != std::string::npos);
    assert(json.find("\"accessMode\":\"read-only\"") != std::string::npos);
    assert(json.find("\"readOnly\":true") != std::string::npos);
    assert(json.find("\"online\":true") != std::string::npos);
}

static void test_serializer_serializes_empty_backend_list()
{
    BackendRegistryJsonSerializer serializer;

    assert(serializer.serializeBackends({}) == "{\"backends\":[]}");
}

int main()
{
    test_serializer_serializes_single_backend();
    test_serializer_serializes_read_only_backend();
    test_serializer_serializes_backend_list();
    test_serializer_serializes_empty_backend_list();

    std::cout
        << "test_backend_registry_json_serializer passed"
        << std::endl;

    return 0;
}
