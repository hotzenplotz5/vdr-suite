#include "BackendAgentClient.h"

#include <curl/curl.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <climits>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <limits>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <utility>

namespace
{
constexpr std::size_t MaximumResponseBytes = 16U * 1024U;
constexpr std::size_t MaximumLocalFileBytes = 16U * 1024U;
std::once_flag CurlInitialization;
bool CurlInitialized = false;

void initializeCurl()
{
    CurlInitialized = curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK;
}

std::string generateOpaqueId(const std::string& prefix, std::size_t bytes)
{
    if (prefix.empty() || bytes < 8 || bytes > 32) return {};
    std::ifstream random("/dev/urandom", std::ios::binary);
    if (!random) return {};
    static const char Hex[] = "0123456789abcdef";
    std::string value = prefix;
    value.reserve(prefix.size() + bytes * 2);
    for (std::size_t index = 0; index < bytes; ++index)
    {
        unsigned char byte = 0;
        random.read(reinterpret_cast<char*>(&byte), 1);
        if (!random) return {};
        value.push_back(Hex[(byte >> 4U) & 0x0fU]);
        value.push_back(Hex[byte & 0x0fU]);
    }
    return value;
}

bool safeIdentifier(const std::string& value)
{
    if (value.empty() || value.size() > 128) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isalnum(character) || character == '-' || character == '_' ||
            character == '.' || character == ':';
    });
}

bool safeSoftwareVersion(const std::string& value)
{
    return !value.empty() && value.size() <= 128 &&
        std::none_of(value.begin(), value.end(), [](unsigned char character) {
            return character < 0x20U || character == 0x7fU;
        });
}

bool validCapabilities(
    const std::vector<std::string>& adapters,
    const std::vector<std::string>& domains)
{
    static const std::vector<std::string> AllowedAdapters = {
        "suitebridge", "restfulapi", "svdrp"};
    static const std::vector<std::string> AllowedDomains = {
        "backend-health", "channels", "epg", "recordings", "timers",
        "searchtimers", "metadata"};
    if (adapters.size() + domains.size() > 32) return false;
    const auto allAllowed = [](const std::vector<std::string>& values,
                               const std::vector<std::string>& allowed) {
        std::vector<std::string> unique = values;
        std::sort(unique.begin(), unique.end());
        if (std::adjacent_find(unique.begin(), unique.end()) != unique.end()) return false;
        return std::all_of(values.begin(), values.end(), [&](const std::string& value) {
            return std::find(allowed.begin(), allowed.end(), value) != allowed.end();
        });
    };
    return allAllowed(adapters, AllowedAdapters) && allAllowed(domains, AllowedDomains);
}

bool onlyAllowedKeys(
    const std::map<std::string, std::string>& values,
    const std::vector<std::string>& allowed)
{
    return std::all_of(values.begin(), values.end(), [&](const auto& value) {
        return std::find(allowed.begin(), allowed.end(), value.first) != allowed.end();
    });
}

bool containsControl(const std::string& value)
{
    return std::any_of(value.begin(), value.end(), [](unsigned char character) {
        return character < 0x20U || character == 0x7fU;
    });
}

std::string trim(std::string value)
{
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())))
        value.erase(value.begin());
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
        value.pop_back();
    return value;
}

bool strictUnsigned(const std::string& value, std::uint64_t& parsed)
{
    if (value.empty() || value.size() > 20 ||
        !std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return std::isdigit(character) != 0;
        }))
    {
        return false;
    }
    errno = 0;
    char* end = nullptr;
    const unsigned long long candidate = std::strtoull(value.c_str(), &end, 10);
    if (errno != 0 || end == nullptr || *end != '\0') return false;
    parsed = static_cast<std::uint64_t>(candidate);
    return true;
}

bool strictInt(const std::string& value, int minimum, int maximum, int& parsed)
{
    std::uint64_t candidate = 0;
    if (!strictUnsigned(value, candidate) || candidate > static_cast<std::uint64_t>(INT_MAX) ||
        candidate < static_cast<std::uint64_t>(minimum) ||
        candidate > static_cast<std::uint64_t>(maximum))
    {
        return false;
    }
    parsed = static_cast<int>(candidate);
    return true;
}

std::vector<std::string> splitCsv(const std::string& value)
{
    std::vector<std::string> result;
    std::size_t start = 0;
    while (start <= value.size())
    {
        const std::size_t comma = value.find(',', start);
        const std::string part = trim(value.substr(
            start, comma == std::string::npos ? std::string::npos : comma - start));
        if (!part.empty()) result.push_back(part);
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
    return result;
}

bool readProtectedKeyValueFile(
    const std::string& path,
    std::map<std::string, std::string>& values,
    std::string& reasonCode,
    bool protectedMode)
{
    values.clear();
    struct stat status{};
    if (lstat(path.c_str(), &status) != 0)
    {
        reasonCode = errno == ENOENT ? "local_file_not_found" : "local_file_stat_failed";
        return false;
    }
    if (!S_ISREG(status.st_mode))
    {
        reasonCode = "local_file_not_regular";
        return false;
    }
    if (protectedMode && (status.st_mode & (S_IRWXG | S_IRWXO)) != 0)
    {
        reasonCode = "local_file_permissions_too_open";
        return false;
    }
    if (status.st_size < 0 || static_cast<std::size_t>(status.st_size) > MaximumLocalFileBytes)
    {
        reasonCode = "local_file_too_large";
        return false;
    }
    std::ifstream input(path);
    if (!input)
    {
        reasonCode = "local_file_open_failed";
        return false;
    }
    std::string line;
    while (std::getline(input, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line.front() == '#') continue;
        const std::size_t separator = line.find('=');
        if (separator == std::string::npos)
        {
            reasonCode = "local_file_invalid_line";
            return false;
        }
        const std::string key = trim(line.substr(0, separator));
        const std::string value = line.substr(separator + 1);
        if (key.empty() || key.size() > 128 || value.size() > 4096 ||
            containsControl(key) || containsControl(value) || values.count(key) != 0)
        {
            reasonCode = "local_file_invalid_value";
            return false;
        }
        values[key] = value;
    }
    return true;
}

bool syncParentDirectory(const std::string& path)
{
    const std::size_t separator = path.find_last_of('/');
    const std::string parent = separator == std::string::npos
        ? "."
        : separator == 0
            ? "/"
            : path.substr(0, separator);
    const int descriptor = open(
        parent.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (descriptor < 0) return false;
    const bool synced = fsync(descriptor) == 0;
    const bool closed = close(descriptor) == 0;
    return synced && closed;
}

bool writeAll(int descriptor, const std::string& content)
{
    std::size_t written = 0;
    while (written < content.size())
    {
        const ssize_t result = ::write(
            descriptor, content.data() + written, content.size() - written);
        if (result < 0)
        {
            if (errno == EINTR) continue;
            return false;
        }
        if (result == 0) return false;
        written += static_cast<std::size_t>(result);
    }
    return true;
}

bool writeProtectedFileAtomically(
    const std::string& path,
    const std::string& content,
    std::string& reasonCode)
{
    if (path.empty() || content.empty() || content.size() > MaximumLocalFileBytes)
    {
        reasonCode = "local_file_invalid_content";
        return false;
    }
    const std::string temporary = path + ".tmp." + std::to_string(getpid());
    const int descriptor = open(
        temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    if (descriptor < 0)
    {
        reasonCode = "local_file_create_failed";
        return false;
    }
    const bool complete = writeAll(descriptor, content) && fsync(descriptor) == 0;
    const int closeResult = close(descriptor);
    if (!complete || closeResult != 0 || rename(temporary.c_str(), path.c_str()) != 0)
    {
        unlink(temporary.c_str());
        reasonCode = "local_file_atomic_write_failed";
        return false;
    }
    if (chmod(path.c_str(), 0600) != 0 || !syncParentDirectory(path))
    {
        reasonCode = "local_file_permission_failed";
        return false;
    }
    return true;
}

std::string jsonEscape(const std::string& value)
{
    std::ostringstream output;
    for (unsigned char character : value)
    {
        switch (character)
        {
            case '"': output << "\\\""; break;
            case '\\': output << "\\\\"; break;
            case '\n': output << "\\n"; break;
            case '\r': output << "\\r"; break;
            case '\t': output << "\\t"; break;
            default:
                if (character >= 0x20U) output << static_cast<char>(character);
        }
    }
    return output.str();
}

std::size_t skipSpace(const std::string& body, std::size_t position)
{
    while (position < body.size() &&
           std::isspace(static_cast<unsigned char>(body[position]))) ++position;
    return position;
}

bool jsonString(const std::string& body, const std::string& key, std::string& value)
{
    value.clear();
    const std::string needle = "\"" + key + "\"";
    std::size_t position = body.find(needle);
    if (position == std::string::npos) return false;
    position = body.find(':', position + needle.size());
    if (position == std::string::npos) return false;
    position = skipSpace(body, position + 1);
    if (position >= body.size() || body[position] != '"') return false;
    ++position;
    while (position < body.size())
    {
        const char character = body[position++];
        if (character == '"') return true;
        if (character == '\\' || static_cast<unsigned char>(character) < 0x20U) return false;
        value.push_back(character);
        if (value.size() > 4096) return false;
    }
    return false;
}

bool jsonUnsigned(const std::string& body, const std::string& key, std::uint64_t& value)
{
    const std::string needle = "\"" + key + "\"";
    std::size_t position = body.find(needle);
    if (position == std::string::npos) return false;
    position = body.find(':', position + needle.size());
    if (position == std::string::npos) return false;
    position = skipSpace(body, position + 1);
    const std::size_t start = position;
    while (position < body.size() &&
           std::isdigit(static_cast<unsigned char>(body[position]))) ++position;
    if (start == position) return false;
    const std::size_t delimiter = skipSpace(body, position);
    if (delimiter >= body.size() ||
        (body[delimiter] != ',' && body[delimiter] != '}')) return false;
    if (!strictUnsigned(body.substr(start, position - start), value)) return false;
    return value <= static_cast<std::uint64_t>(
        std::numeric_limits<std::int64_t>::max());
}

std::string jsonArray(const std::vector<std::string>& values)
{
    std::ostringstream output;
    output << '[';
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index != 0) output << ',';
        output << '"' << jsonEscape(values[index]) << '"';
    }
    output << ']';
    return output.str();
}

struct CurlWriteState
{
    std::string* body = nullptr;
};

std::size_t curlWrite(char* data, std::size_t size, std::size_t count, void* userData)
{
    if (size != 0 && count > SIZE_MAX / size) return 0;
    const std::size_t bytes = size * count;
    auto* state = static_cast<CurlWriteState*>(userData);
    if (state == nullptr || state->body == nullptr ||
        state->body->size() > MaximumResponseBytes ||
        bytes > MaximumResponseBytes - state->body->size())
    {
        return 0;
    }
    state->body->append(data, bytes);
    return bytes;
}

bool safeProtocolPath(const std::string& path)
{
    return path.rfind("/api/agent/v1/", 0) == 0 && path.size() <= 128 &&
        path.find('?') == std::string::npos && path.find('#') == std::string::npos &&
        !containsControl(path);
}

std::string responseErrorCode(const BackendAgentTransportResponse& response)
{
    std::string code;
    return jsonString(response.body, "code", code) ? code : "agent_protocol_rejected";
}
}

CurlBackendAgentControlPlaneTransport::CurlBackendAgentControlPlaneTransport(
    BackendAgentClientConfig config)
    : config_(std::move(config))
{
}

bool CurlBackendAgentControlPlaneTransport::validProtectedControlPlaneUrl(
    const std::string& url)
{
    static const std::string Prefix = "https://";
    if (url.rfind(Prefix, 0) != 0 || url.size() <= Prefix.size() ||
        url.size() > 4096 || containsControl(url) ||
        std::any_of(url.begin(), url.end(), [](unsigned char character) {
            return std::isspace(character) != 0;
        }) ||
        url.find('@') != std::string::npos ||
        url.find('?') != std::string::npos || url.find('#') != std::string::npos ||
        url.find('\\') != std::string::npos)
    {
        return false;
    }
    const std::size_t path = url.find('/', Prefix.size());
    const std::string authority = url.substr(
        Prefix.size(), path == std::string::npos ? std::string::npos : path - Prefix.size());
    return !authority.empty() && authority != "." && authority != ".." &&
        url.back() != '/';
}

BackendAgentTransportResponse CurlBackendAgentControlPlaneTransport::postEnrollment(
    const std::string& enrollmentId,
    const std::string& enrollmentToken,
    const std::string& path,
    const std::string& body)
{
    return perform(
        path, body, "VDR-Suite-Enrollment " + enrollmentId + ":" + enrollmentToken,
        {}, {});
}

BackendAgentTransportResponse CurlBackendAgentControlPlaneTransport::postAuthenticated(
    const std::string& agentId,
    const std::string& credentialSecret,
    const std::string& path,
    const std::string& body)
{
    return perform(path, body, {}, agentId, credentialSecret);
}

BackendAgentTransportResponse CurlBackendAgentControlPlaneTransport::perform(
    const std::string& path,
    const std::string& body,
    const std::string& enrollmentAuthorization,
    const std::string& basicLogin,
    const std::string& basicSecret)
{
    BackendAgentTransportResponse response;
    if (!validProtectedControlPlaneUrl(config_.controlPlaneUrl) ||
        !safeProtocolPath(path) || body.size() > MaximumResponseBytes ||
        containsControl(enrollmentAuthorization) || containsControl(basicLogin) ||
        containsControl(basicSecret) ||
        config_.connectTimeoutMilliseconds < 100 ||
        config_.connectTimeoutMilliseconds > 30000 ||
        config_.requestTimeoutMilliseconds < config_.connectTimeoutMilliseconds ||
        config_.requestTimeoutMilliseconds > 60000)
    {
        response.errorCode = "invalid_transport_configuration";
        return response;
    }

    std::call_once(CurlInitialization, initializeCurl);
    if (!CurlInitialized)
    {
        response.errorCode = "transport_initialization_failed";
        return response;
    }
    CURL* handle = curl_easy_init();
    if (handle == nullptr)
    {
        response.errorCode = "transport_initialization_failed";
        return response;
    }

    const std::string url = config_.controlPlaneUrl + path;
    curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");
    if (!enrollmentAuthorization.empty())
    {
        const std::string authorization = "Authorization: " + enrollmentAuthorization;
        headers = curl_slist_append(headers, authorization.c_str());
    }

    CurlWriteState writeState{&response.body};
    bool configured = headers != nullptr;
    configured = configured && curl_easy_setopt(handle, CURLOPT_URL, url.c_str()) == CURLE_OK;
    configured = configured && curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers) == CURLE_OK;
    configured = configured && curl_easy_setopt(handle, CURLOPT_POST, 1L) == CURLE_OK;
    configured = configured && curl_easy_setopt(handle, CURLOPT_POSTFIELDS, body.data()) == CURLE_OK;
    configured = configured && curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE_LARGE,
                                                  static_cast<curl_off_t>(body.size())) == CURLE_OK;
    configured = configured && curl_easy_setopt(handle, CURLOPT_USERAGENT,
                                                  "vdr-suite-backend-agent/1") == CURLE_OK;
    configured = configured && curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L) == CURLE_OK;
    configured = configured && curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 0L) == CURLE_OK;
    configured = configured && curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 0L) == CURLE_OK;
    configured = configured && curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1L) == CURLE_OK;
    configured = configured && curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 2L) == CURLE_OK;
    configured = configured && curl_easy_setopt(handle, CURLOPT_NETRC, CURL_NETRC_IGNORED) == CURLE_OK;
    configured = configured && curl_easy_setopt(handle, CURLOPT_PROXY, "") == CURLE_OK;
    configured = configured && curl_easy_setopt(handle, CURLOPT_PROTOCOLS_STR, "https") == CURLE_OK;
    configured = configured && curl_easy_setopt(handle, CURLOPT_REDIR_PROTOCOLS_STR, "https") == CURLE_OK;
    configured = configured && curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT_MS,
                                                  config_.connectTimeoutMilliseconds) == CURLE_OK;
    configured = configured && curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS,
                                                  config_.requestTimeoutMilliseconds) == CURLE_OK;
    configured = configured && curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, curlWrite) == CURLE_OK;
    configured = configured && curl_easy_setopt(handle, CURLOPT_WRITEDATA, &writeState) == CURLE_OK;
    if (!config_.caCertificatePath.empty())
    {
        configured = configured && curl_easy_setopt(
            handle, CURLOPT_CAINFO, config_.caCertificatePath.c_str()) == CURLE_OK;
    }
    if (!basicLogin.empty())
    {
        configured = configured && curl_easy_setopt(handle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC) == CURLE_OK;
        configured = configured && curl_easy_setopt(handle, CURLOPT_USERNAME, basicLogin.c_str()) == CURLE_OK;
        configured = configured && curl_easy_setopt(handle, CURLOPT_PASSWORD, basicSecret.c_str()) == CURLE_OK;
    }

    const CURLcode result = configured ? curl_easy_perform(handle) : CURLE_FAILED_INIT;
    long status = 0;
    if (result == CURLE_OK) curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &status);
    curl_slist_free_all(headers);
    curl_easy_cleanup(handle);

    response.statusCode = static_cast<int>(status);
    response.transportSucceeded = result == CURLE_OK;
    if (!response.transportSucceeded)
    {
        response.body.clear();
        response.errorCode = "protected_transport_failed";
    }
    else if (response.statusCode < 200 || response.statusCode >= 300)
    {
        response.errorCode = responseErrorCode(response);
    }
    return response;
}

BackendAgentClientRuntime::BackendAgentClientRuntime(
    BackendAgentClientConfig config,
    IBackendAgentControlPlaneTransport& transport,
    Sleep sleep,
    Log log)
    : config_(std::move(config)),
      transport_(transport),
      sleep_(sleep ? std::move(sleep) : Sleep([](int seconds) {
          std::this_thread::sleep_for(std::chrono::seconds(seconds));
      })),
      log_(std::move(log)),
      agentInstanceId_(generateOpaqueId("agi_", 16))
{
}

const BackendAgentClientState& BackendAgentClientRuntime::state() const
{
    return state_;
}

const std::string& BackendAgentClientRuntime::agentInstanceId() const
{
    return agentInstanceId_;
}

void BackendAgentClientRuntime::log(const std::string& message) const
{
    if (log_) log_(message);
}

bool BackendAgentClientRuntime::loadConfig(
    const std::string& path,
    BackendAgentClientConfig& config,
    std::string& reasonCode)
{
    std::map<std::string, std::string> values;
    if (!readProtectedKeyValueFile(path, values, reasonCode, false)) return false;
    static const std::vector<std::string> Allowed = {
        "CONTROL_PLANE_URL", "BACKEND_ID", "IDENTITY_PATH", "ENROLLMENT_PATH",
        "CA_CERTIFICATE_PATH", "SOFTWARE_VERSION", "ADAPTERS", "OBSERVATION_DOMAINS",
        "HEARTBEAT_INTERVAL_SECONDS", "RECONNECT_INITIAL_SECONDS",
        "RECONNECT_MAXIMUM_SECONDS", "CONNECT_TIMEOUT_MILLISECONDS",
        "REQUEST_TIMEOUT_MILLISECONDS"};
    for (const auto& value : values)
    {
        if (std::find(Allowed.begin(), Allowed.end(), value.first) == Allowed.end())
        {
            reasonCode = "unknown_configuration_key";
            return false;
        }
    }
    config.controlPlaneUrl = values["CONTROL_PLANE_URL"];
    config.backendId = values["BACKEND_ID"];
    config.identityPath = values["IDENTITY_PATH"];
    config.enrollmentPath = values["ENROLLMENT_PATH"];
    config.caCertificatePath = values["CA_CERTIFICATE_PATH"];
    if (!values["SOFTWARE_VERSION"].empty()) config.softwareVersion = values["SOFTWARE_VERSION"];
    config.adapters = splitCsv(values["ADAPTERS"]);
    if (!values["OBSERVATION_DOMAINS"].empty())
        config.observationDomains = splitCsv(values["OBSERVATION_DOMAINS"]);
    if ((!values["HEARTBEAT_INTERVAL_SECONDS"].empty() &&
         !strictInt(values["HEARTBEAT_INTERVAL_SECONDS"], 10, 60,
                    config.heartbeatIntervalSeconds)) ||
        (!values["RECONNECT_INITIAL_SECONDS"].empty() &&
         !strictInt(values["RECONNECT_INITIAL_SECONDS"], 1, 60,
                    config.reconnectInitialSeconds)) ||
        (!values["RECONNECT_MAXIMUM_SECONDS"].empty() &&
         !strictInt(values["RECONNECT_MAXIMUM_SECONDS"], 1, 300,
                    config.reconnectMaximumSeconds)))
    {
        reasonCode = "invalid_configuration_value";
        return false;
    }
    int timeout = 0;
    if (!values["CONNECT_TIMEOUT_MILLISECONDS"].empty())
    {
        if (!strictInt(values["CONNECT_TIMEOUT_MILLISECONDS"], 100, 30000, timeout))
        {
            reasonCode = "invalid_configuration_value";
            return false;
        }
        config.connectTimeoutMilliseconds = timeout;
    }
    if (!values["REQUEST_TIMEOUT_MILLISECONDS"].empty())
    {
        if (!strictInt(values["REQUEST_TIMEOUT_MILLISECONDS"], 100, 60000, timeout))
        {
            reasonCode = "invalid_configuration_value";
            return false;
        }
        config.requestTimeoutMilliseconds = timeout;
    }
    if (!CurlBackendAgentControlPlaneTransport::validProtectedControlPlaneUrl(
            config.controlPlaneUrl) ||
        !safeIdentifier(config.backendId) ||
        config.identityPath.empty() || config.enrollmentPath.empty() ||
        !safeSoftwareVersion(config.softwareVersion) ||
        config.reconnectInitialSeconds > config.reconnectMaximumSeconds ||
        config.requestTimeoutMilliseconds < config.connectTimeoutMilliseconds)
    {
        reasonCode = "invalid_configuration";
        return false;
    }
    if (!validCapabilities(config.adapters, config.observationDomains))
    {
        reasonCode = "invalid_capability_configuration";
        return false;
    }
    reasonCode = "configuration_loaded";
    return true;
}

bool BackendAgentClientRuntime::loadIdentity(
    const std::string& path,
    BackendAgentClientState& state,
    std::string& reasonCode)
{
    state = BackendAgentClientState{};
    std::map<std::string, std::string> values;
    if (!readProtectedKeyValueFile(path, values, reasonCode, true)) return false;
    static const std::vector<std::string> Allowed = {
        "version", "agent_id", "backend_id", "credential_id",
        "credential_generation", "credential_secret", "backend_generation",
        "heartbeat_sequence", "capability_revision", "pending_rotation_id",
        "pending_credential_generation", "pending_credential_secret"};
    if (!onlyAllowedKeys(values, Allowed))
    {
        reasonCode = "unknown_identity_key";
        return false;
    }
    std::uint64_t credentialGeneration = 0;
    std::uint64_t backendGeneration = 0;
    std::uint64_t heartbeatSequence = 0;
    std::uint64_t capabilityRevision = 0;
    std::uint64_t pendingCredentialGeneration = 0;
    if (values["version"] != "1" ||
        !safeIdentifier(values["agent_id"]) ||
        !safeIdentifier(values["backend_id"]) ||
        !safeIdentifier(values["credential_id"]) ||
        values["credential_secret"].size() < 32 || values["credential_secret"].size() > 1024 ||
        !strictUnsigned(values["credential_generation"], credentialGeneration) ||
        !strictUnsigned(values["backend_generation"], backendGeneration) ||
        !strictUnsigned(values["heartbeat_sequence"], heartbeatSequence) ||
        !strictUnsigned(values["capability_revision"], capabilityRevision) ||
        credentialGeneration > static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()) ||
        backendGeneration > static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()) ||
        heartbeatSequence > static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()) ||
        capabilityRevision > static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()))
    {
        reasonCode = "invalid_identity_file";
        return false;
    }
    const bool hasPendingRotation = !values["pending_rotation_id"].empty();
    if (hasPendingRotation &&
        (!safeIdentifier(values["pending_rotation_id"]) ||
         values["pending_credential_secret"].size() < 32 ||
         values["pending_credential_secret"].size() > 1024 ||
         credentialGeneration >= static_cast<std::uint64_t>(
             std::numeric_limits<std::int64_t>::max()) ||
         !strictUnsigned(
             values["pending_credential_generation"],
             pendingCredentialGeneration) ||
         pendingCredentialGeneration != credentialGeneration + 1))
    {
        reasonCode = "invalid_pending_credential_rotation";
        return false;
    }
    if (!hasPendingRotation &&
        (!values["pending_credential_secret"].empty() ||
         !values["pending_credential_generation"].empty()))
    {
        reasonCode = "invalid_pending_credential_rotation";
        return false;
    }
    state.agentId = values["agent_id"];
    state.backendId = values["backend_id"];
    state.credentialId = values["credential_id"];
    state.credentialSecret = values["credential_secret"];
    state.credentialGeneration = credentialGeneration;
    state.backendGeneration = backendGeneration;
    state.heartbeatSequence = heartbeatSequence;
    state.capabilityRevision = capabilityRevision;
    if (hasPendingRotation)
    {
        state.pendingRotationId = values["pending_rotation_id"];
        state.pendingCredentialSecret = values["pending_credential_secret"];
        state.pendingCredentialGeneration = pendingCredentialGeneration;
    }
    reasonCode = "identity_loaded";
    return true;
}

bool BackendAgentClientRuntime::loadEnrollmentPackage(
    const std::string& path,
    BackendAgentEnrollmentPackage& package,
    std::string& reasonCode)
{
    package = BackendAgentEnrollmentPackage{};
    std::map<std::string, std::string> values;
    if (!readProtectedKeyValueFile(path, values, reasonCode, true)) return false;
    static const std::vector<std::string> Allowed = {
        "version", "enrollment_id", "backend_id", "enrollment_token"};
    if (!onlyAllowedKeys(values, Allowed))
    {
        reasonCode = "unknown_enrollment_key";
        return false;
    }
    if (values["version"] != "1" ||
        !safeIdentifier(values["enrollment_id"]) ||
        !safeIdentifier(values["backend_id"]) ||
        values["enrollment_token"].size() < 32 || values["enrollment_token"].size() > 1024)
    {
        reasonCode = "invalid_enrollment_file";
        return false;
    }
    package.enrollmentId = values["enrollment_id"];
    package.backendId = values["backend_id"];
    package.enrollmentToken = values["enrollment_token"];
    reasonCode = "enrollment_loaded";
    return true;
}

bool BackendAgentClientRuntime::writeIdentityAtomically(
    const std::string& path,
    const BackendAgentClientState& state,
    std::string& reasonCode)
{
    const bool hasPendingRotation = !state.pendingRotationId.empty();
    const bool validPendingRotation = !hasPendingRotation ||
        (safeIdentifier(state.pendingRotationId) &&
         state.pendingCredentialSecret.size() >= 32 &&
         state.pendingCredentialSecret.size() <= 1024 &&
         state.credentialGeneration != UINT64_MAX &&
         state.pendingCredentialGeneration == state.credentialGeneration + 1);
    if (!safeIdentifier(state.agentId) ||
        !safeIdentifier(state.backendId) ||
        !safeIdentifier(state.credentialId) ||
        state.credentialSecret.size() < 32 || state.credentialSecret.size() > 1024 ||
        state.credentialGeneration == 0 ||
        !validPendingRotation ||
        (!hasPendingRotation &&
         (!state.pendingCredentialSecret.empty() ||
          state.pendingCredentialGeneration != 0)))
    {
        reasonCode = "invalid_identity_state";
        return false;
    }
    std::ostringstream content;
    content << "version=1\n"
            << "agent_id=" << state.agentId << "\n"
            << "backend_id=" << state.backendId << "\n"
            << "credential_id=" << state.credentialId << "\n"
            << "credential_generation=" << state.credentialGeneration << "\n"
            << "credential_secret=" << state.credentialSecret << "\n"
            << "backend_generation=" << state.backendGeneration << "\n"
            << "heartbeat_sequence=" << state.heartbeatSequence << "\n"
            << "capability_revision=" << state.capabilityRevision << "\n";
    if (hasPendingRotation)
    {
        content << "pending_rotation_id=" << state.pendingRotationId << "\n"
                << "pending_credential_generation="
                << state.pendingCredentialGeneration << "\n"
                << "pending_credential_secret="
                << state.pendingCredentialSecret << "\n";
    }
    if (!writeProtectedFileAtomically(path, content.str(), reasonCode)) return false;
    reasonCode = "identity_persisted";
    return true;
}

bool writeBackendAgentEnrollmentPackageAtomically(
    const std::string& path,
    const BackendAgentEnrollmentPackage& package,
    std::string& reasonCode)
{
    if (!safeIdentifier(package.enrollmentId) ||
        !safeIdentifier(package.backendId) ||
        package.enrollmentToken.size() < 32 || package.enrollmentToken.size() > 1024)
    {
        reasonCode = "invalid_enrollment_package";
        return false;
    }
    std::ostringstream content;
    content << "version=1\n"
            << "enrollment_id=" << package.enrollmentId << "\n"
            << "backend_id=" << package.backendId << "\n"
            << "enrollment_token=" << package.enrollmentToken << "\n";
    if (!writeProtectedFileAtomically(path, content.str(), reasonCode)) return false;
    reasonCode = "enrollment_package_persisted";
    return true;
}

bool BackendAgentClientRuntime::persist(std::string& reasonCode)
{
    return writeIdentityAtomically(config_.identityPath, state_, reasonCode);
}

bool BackendAgentClientRuntime::enroll(std::string& reasonCode)
{
    BackendAgentEnrollmentPackage package;
    if (!loadEnrollmentPackage(config_.enrollmentPath, package, reasonCode)) return false;
    if (package.backendId != config_.backendId)
    {
        reasonCode = "enrollment_backend_mismatch";
        return false;
    }
    BackendAgentClientState pending;
    pending.backendId = package.backendId;
    pending.credentialSecret = generateOpaqueId("ags_", 24);
    if (pending.credentialSecret.size() < 32)
    {
        reasonCode = "credential_generation_failed";
        return false;
    }
    const std::string body = "{\"credentialSecret\":\"" +
        jsonEscape(pending.credentialSecret) + "\"}";
    BackendAgentTransportResponse response = transport_.postEnrollment(
        package.enrollmentId, package.enrollmentToken, "/api/agent/v1/enroll", body);
    std::fill(package.enrollmentToken.begin(), package.enrollmentToken.end(), '\0');
    if (!response.transportSucceeded || response.statusCode != 200)
    {
        reasonCode = response.errorCode.empty() ? "enrollment_transport_failed" : response.errorCode;
        return false;
    }
    if (!jsonString(response.body, "agentId", pending.agentId) ||
        !jsonString(response.body, "backendId", pending.backendId) ||
        !jsonString(response.body, "credentialId", pending.credentialId) ||
        !jsonUnsigned(response.body, "credentialGeneration", pending.credentialGeneration) ||
        pending.backendId != config_.backendId)
    {
        reasonCode = "invalid_enrollment_response";
        return false;
    }
    state_ = std::move(pending);
    if (!persist(reasonCode)) return false;
    if (unlink(config_.enrollmentPath.c_str()) != 0 && errno != ENOENT)
    {
        reasonCode = "enrollment_cleanup_failed";
        return false;
    }
    reasonCode = "enrollment_succeeded";
    return true;
}

bool BackendAgentClientRuntime::connect(std::string& reasonCode)
{
    if (!connectWithCredential(
            state_.credentialSecret, state_.credentialGeneration, reasonCode))
    {
        return false;
    }
    return persist(reasonCode);
}

bool BackendAgentClientRuntime::connectWithCredential(
    const std::string& credentialSecret,
    std::uint64_t expectedCredentialGeneration,
    std::string& reasonCode)
{
    if (credentialSecret.size() < 32 || credentialSecret.size() > 1024 ||
        expectedCredentialGeneration == 0)
    {
        reasonCode = "invalid_agent_credential_state";
        return false;
    }
    std::ostringstream body;
    body << "{\"backendId\":\"" << jsonEscape(state_.backendId)
         << "\",\"agentInstanceId\":\"" << jsonEscape(agentInstanceId_)
         << "\",\"protocolVersion\":\"vdr-suite-agent/1\""
         << ",\"softwareVersion\":\"" << jsonEscape(config_.softwareVersion)
         << "\",\"backendGeneration\":" << state_.backendGeneration
         << ",\"heartbeatSequence\":" << state_.heartbeatSequence
         << ",\"capabilityRevision\":" << state_.capabilityRevision << '}';
    const BackendAgentTransportResponse response = transport_.postAuthenticated(
        state_.agentId, credentialSecret, "/api/agent/v1/connect", body.str());
    if (!response.transportSucceeded || response.statusCode != 200)
    {
        reasonCode = response.errorCode.empty() ? "connect_transport_failed" : response.errorCode;
        return false;
    }
    std::string backendId;
    std::string disposition;
    std::uint64_t backendGeneration = 0;
    std::uint64_t credentialGeneration = 0;
    std::uint64_t heartbeatSequence = 0;
    std::uint64_t capabilityRevision = 0;
    if (!jsonString(response.body, "backendId", backendId) ||
        !jsonString(response.body, "disposition", disposition) ||
        !jsonUnsigned(response.body, "backendGeneration", backendGeneration) ||
        !jsonUnsigned(response.body, "credentialGeneration", credentialGeneration) ||
        !jsonUnsigned(response.body, "heartbeatSequence", heartbeatSequence) ||
        !jsonUnsigned(response.body, "capabilityRevision", capabilityRevision) ||
        backendId != state_.backendId || backendGeneration == 0 ||
        credentialGeneration != expectedCredentialGeneration ||
        (disposition != "resume" && disposition != "replace" &&
         disposition != "resync-required"))
    {
        reasonCode = "invalid_connect_response";
        return false;
    }
    state_.backendGeneration = backendGeneration;
    state_.heartbeatSequence = heartbeatSequence;
    state_.capabilityRevision = capabilityRevision;
    reasonCode = disposition;
    return true;
}

bool BackendAgentClientRuntime::promotePendingCredential(std::string& reasonCode)
{
    if (state_.pendingRotationId.empty() ||
        state_.pendingCredentialSecret.size() < 32 ||
        state_.credentialGeneration >= static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()) ||
        state_.pendingCredentialGeneration != state_.credentialGeneration + 1)
    {
        reasonCode = "invalid_pending_credential_rotation";
        return false;
    }
    std::fill(state_.credentialSecret.begin(), state_.credentialSecret.end(), '\0');
    state_.credentialSecret = std::move(state_.pendingCredentialSecret);
    state_.credentialGeneration = state_.pendingCredentialGeneration;
    state_.pendingRotationId.clear();
    state_.pendingCredentialSecret.clear();
    state_.pendingCredentialGeneration = 0;
    if (!persist(reasonCode)) return false;
    reasonCode = "credential_rotation_promoted";
    return true;
}

bool BackendAgentClientRuntime::submitPendingCredentialRotation(
    const std::string& authenticationSecret,
    std::string& reasonCode)
{
    if (state_.pendingRotationId.empty() ||
        state_.pendingCredentialSecret.size() < 32 ||
        state_.credentialGeneration >= static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()) ||
        state_.pendingCredentialGeneration != state_.credentialGeneration + 1)
    {
        reasonCode = "invalid_pending_credential_rotation";
        return false;
    }
    std::ostringstream body;
    body << "{\"backendId\":\"" << jsonEscape(state_.backendId)
         << "\",\"agentInstanceId\":\"" << jsonEscape(agentInstanceId_)
         << "\",\"backendGeneration\":" << state_.backendGeneration
         << ",\"rotationId\":\"" << jsonEscape(state_.pendingRotationId)
         << "\",\"expectedCredentialGeneration\":" << state_.credentialGeneration
         << ",\"credentialSecret\":\""
         << jsonEscape(state_.pendingCredentialSecret) << "\"}";
    const BackendAgentTransportResponse response = transport_.postAuthenticated(
        state_.agentId, authenticationSecret,
        "/api/agent/v1/credentials/rotate", body.str());
    std::uint64_t acceptedGeneration = 0;
    if (!response.transportSucceeded || response.statusCode != 200 ||
        !jsonUnsigned(response.body, "credentialGeneration", acceptedGeneration) ||
        acceptedGeneration != state_.pendingCredentialGeneration)
    {
        reasonCode = response.errorCode.empty()
            ? "credential_rotation_failed"
            : response.errorCode;
        return false;
    }
    reasonCode = "credential_rotation_accepted";
    return true;
}

bool BackendAgentClientRuntime::reconcilePendingCredentialRotation(
    std::string& reasonCode)
{
    if (state_.pendingRotationId.empty())
    {
        reasonCode = "no_pending_credential_rotation";
        return true;
    }

    std::string pendingConnectReason;
    if (connectWithCredential(
            state_.pendingCredentialSecret,
            state_.pendingCredentialGeneration,
            pendingConnectReason))
    {
        if (!promotePendingCredential(reasonCode)) return false;
        reasonCode = "credential_rotation_recovered";
        return true;
    }
    if (pendingConnectReason != "agent_authentication_failed")
    {
        reasonCode = pendingConnectReason;
        return false;
    }

    std::string currentConnectReason;
    if (!connectWithCredential(
            state_.credentialSecret,
            state_.credentialGeneration,
            currentConnectReason))
    {
        reasonCode = currentConnectReason == "agent_authentication_failed"
            ? "credential_rotation_reconciliation_conflict"
            : currentConnectReason;
        return false;
    }
    if (!submitPendingCredentialRotation(state_.credentialSecret, reasonCode))
    {
        synchronized_ = false;
        return false;
    }
    if (!promotePendingCredential(reasonCode)) return false;
    reasonCode = "credential_rotation_resubmitted";
    return true;
}

bool BackendAgentClientRuntime::rotateCredential(std::string& reasonCode)
{
    if (!synchronized_ || state_.backendGeneration == 0 ||
        state_.credentialGeneration == 0 ||
        state_.credentialGeneration == UINT64_MAX)
    {
        reasonCode = "agent_not_synchronized";
        return false;
    }
    if (state_.pendingRotationId.empty())
    {
        state_.pendingRotationId = generateOpaqueId("agr_", 16);
        state_.pendingCredentialSecret = generateOpaqueId("ags_", 24);
        state_.pendingCredentialGeneration = state_.credentialGeneration + 1;
        if (state_.pendingRotationId.empty() ||
            state_.pendingCredentialSecret.size() < 32 ||
            !persist(reasonCode))
        {
            reasonCode = "credential_rotation_preparation_failed";
            return false;
        }
    }

    if (!submitPendingCredentialRotation(state_.credentialSecret, reasonCode))
    {
        synchronized_ = false;
        return false;
    }
    if (!promotePendingCredential(reasonCode))
    {
        synchronized_ = false;
        return false;
    }
    synchronized_ = false;
    if (!synchronize(reasonCode)) return false;
    reasonCode = "credential_rotated";
    return true;
}

bool BackendAgentClientRuntime::publishCapabilities(std::string& reasonCode)
{
    if (state_.capabilityRevision >= static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()))
    {
        reasonCode = "capability_revision_exhausted";
        return false;
    }
    const std::uint64_t revision = state_.capabilityRevision + 1;
    std::ostringstream body;
    body << "{\"backendId\":\"" << jsonEscape(state_.backendId)
         << "\",\"agentInstanceId\":\"" << jsonEscape(agentInstanceId_)
         << "\",\"backendGeneration\":" << state_.backendGeneration
         << ",\"capabilityRevision\":" << revision
         << ",\"readOnly\":true,\"adapters\":" << jsonArray(config_.adapters)
         << ",\"observationDomains\":" << jsonArray(config_.observationDomains) << '}';
    const BackendAgentTransportResponse response = transport_.postAuthenticated(
        state_.agentId, state_.credentialSecret, "/api/agent/v1/capabilities", body.str());
    std::uint64_t acceptedRevision = 0;
    if (!response.transportSucceeded || response.statusCode != 200 ||
        !jsonUnsigned(response.body, "capabilityRevision", acceptedRevision) ||
        acceptedRevision != revision)
    {
        reasonCode = response.errorCode.empty() ? "capability_publication_failed" : response.errorCode;
        return false;
    }
    state_.capabilityRevision = acceptedRevision;
    if (!persist(reasonCode)) return false;
    reasonCode = "capabilities_published";
    return true;
}

bool BackendAgentClientRuntime::heartbeat(std::string& reasonCode)
{
    if (!synchronized_ || state_.backendGeneration == 0 || state_.capabilityRevision == 0)
    {
        reasonCode = "agent_not_synchronized";
        return false;
    }
    if (state_.heartbeatSequence >= static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()))
    {
        synchronized_ = false;
        reasonCode = "heartbeat_sequence_exhausted";
        return false;
    }
    const std::uint64_t sequence = state_.heartbeatSequence + 1;
    std::ostringstream body;
    body << "{\"backendId\":\"" << jsonEscape(state_.backendId)
         << "\",\"agentInstanceId\":\"" << jsonEscape(agentInstanceId_)
         << "\",\"backendGeneration\":" << state_.backendGeneration
         << ",\"heartbeatSequence\":" << sequence << '}';
    const BackendAgentTransportResponse response = transport_.postAuthenticated(
        state_.agentId, state_.credentialSecret, "/api/agent/v1/heartbeat", body.str());
    std::uint64_t acceptedSequence = 0;
    if (!response.transportSucceeded || response.statusCode != 200 ||
        !jsonUnsigned(response.body, "heartbeatSequence", acceptedSequence) ||
        acceptedSequence != sequence)
    {
        synchronized_ = false;
        reasonCode = response.errorCode.empty() ? "heartbeat_failed" : response.errorCode;
        return false;
    }
    state_.heartbeatSequence = acceptedSequence;
    if (!persist(reasonCode))
    {
        synchronized_ = false;
        return false;
    }
    reasonCode = "lease_renewed";
    return true;
}

bool BackendAgentClientRuntime::synchronize(std::string& reasonCode)
{
    if (agentInstanceId_.empty())
    {
        reasonCode = "agent_instance_generation_failed";
        return false;
    }
    BackendAgentClientState loaded;
    if (loadIdentity(config_.identityPath, loaded, reasonCode))
    {
        state_ = std::move(loaded);
    }
    else if (reasonCode == "local_file_not_found")
    {
        if (!enroll(reasonCode)) return false;
    }
    else
    {
        return false;
    }
    if (state_.backendId != config_.backendId)
    {
        reasonCode = "identity_backend_mismatch";
        return false;
    }
    if (!state_.pendingRotationId.empty() &&
        !reconcilePendingCredentialRotation(reasonCode))
    {
        synchronized_ = false;
        return false;
    }
    if (!connect(reasonCode)) return false;
    if (state_.capabilityRevision == 0 && !publishCapabilities(reasonCode)) return false;
    synchronized_ = true;
    if (!heartbeat(reasonCode)) return false;
    synchronized_ = true;
    reasonCode = "agent_online";
    return true;
}

int BackendAgentClientRuntime::run(const std::function<bool()>& stopRequested)
{
    int reconnectDelay = config_.reconnectInitialSeconds;
    while (!stopRequested())
    {
        std::string reason;
        if (!synchronized_)
        {
            if (synchronize(reason))
            {
                reconnectDelay = config_.reconnectInitialSeconds;
                log("Backend Agent synchronized");
            }
            else
            {
                log("Backend Agent synchronization failed: " + reason);
                sleep_(reconnectDelay);
                reconnectDelay = std::min(
                    config_.reconnectMaximumSeconds,
                    reconnectDelay > config_.reconnectMaximumSeconds / 2
                        ? config_.reconnectMaximumSeconds
                        : reconnectDelay * 2);
                continue;
            }
        }
        sleep_(config_.heartbeatIntervalSeconds);
        if (stopRequested()) break;
        if (!heartbeat(reason))
        {
            log("Backend Agent heartbeat failed: " + reason);
        }
    }
    return 0;
}
