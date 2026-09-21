#include "CurlExternalArtworkHttpTransport.h"

#include <curl/curl.h>

#include <algorithm>
#include <arpa/inet.h>
#include <cctype>
#include <cerrno>
#include <fcntl.h>
#include <climits>
#include <cstring>
#include <cstdint>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <utility>

namespace
{
struct TransferState
{
    ExternalArtworkHttpResponse* response = nullptr;
    std::size_t maximumBytes = 0;
};

std::once_flag curlInitialization;
bool curlInitialized = false;

void initializeCurl()
{
    curlInitialized = curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK;
}

std::string trimAscii(std::string value)
{
    while (!value.empty() &&
           std::isspace(static_cast<unsigned char>(value.front())))
    {
        value.erase(value.begin());
    }
    while (!value.empty() &&
           std::isspace(static_cast<unsigned char>(value.back())))
    {
        value.pop_back();
    }
    return value;
}

std::string lowerAscii(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](const unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return value;
}

bool containsControl(const std::string& value)
{
    for (const unsigned char character : value)
    {
        if (character < 0x20U || character == 0x7fU)
        {
            return true;
        }
    }
    return false;
}

bool isPrivateIpv4(const in_addr& address)
{
    const std::uint32_t value = ntohl(address.s_addr);
    const std::uint8_t first = static_cast<std::uint8_t>(value >> 24U);
    const std::uint8_t second = static_cast<std::uint8_t>(value >> 16U);

    if (first == 0U || first == 10U || first == 127U || first >= 224U)
    {
        return true;
    }
    if (first == 100U && second >= 64U && second <= 127U)
    {
        return true;
    }
    if (first == 169U && second == 254U)
    {
        return true;
    }
    if (first == 172U && second >= 16U && second <= 31U)
    {
        return true;
    }
    if (first == 192U && (second == 0U || second == 168U))
    {
        return true;
    }
    if (first == 198U && (second == 18U || second == 19U))
    {
        return true;
    }
    return false;
}

bool isPrivateIpv6(const in6_addr& address)
{
    static const in6_addr unspecified = IN6ADDR_ANY_INIT;
    static const in6_addr loopback = IN6ADDR_LOOPBACK_INIT;
    if (std::memcmp(&address, &unspecified, sizeof(address)) == 0 ||
        std::memcmp(&address, &loopback, sizeof(address)) == 0)
    {
        return true;
    }

    const unsigned char first = address.s6_addr[0];
    const unsigned char second = address.s6_addr[1];
    if ((first & 0xfeU) == 0xfcU ||
        (first == 0xfeU && (second & 0xc0U) == 0x80U) ||
        first == 0xffU)
    {
        return true;
    }
    return false;
}

curl_socket_t openSocketCallback(
    void*,
    curlsocktype,
    struct curl_sockaddr* address)
{
    if (address == nullptr)
    {
        return CURL_SOCKET_BAD;
    }

    if (address->family == AF_INET)
    {
        const auto* socketAddress =
            reinterpret_cast<const sockaddr_in*>(&address->addr);
        if (isPrivateIpv4(socketAddress->sin_addr))
        {
            return CURL_SOCKET_BAD;
        }
    }
    else if (address->family == AF_INET6)
    {
        const auto* socketAddress =
            reinterpret_cast<const sockaddr_in6*>(&address->addr);
        if (isPrivateIpv6(socketAddress->sin6_addr))
        {
            return CURL_SOCKET_BAD;
        }
    }
    else
    {
        return CURL_SOCKET_BAD;
    }

    const curl_socket_t descriptor =
        ::socket(address->family, address->socktype, address->protocol);
    if (descriptor != CURL_SOCKET_BAD)
    {
        const int flags = ::fcntl(descriptor, F_GETFD);
        if (flags >= 0)
        {
            ::fcntl(descriptor, F_SETFD, flags | FD_CLOEXEC);
        }
    }
    return descriptor;
}

std::size_t writeCallback(
    char* data,
    std::size_t size,
    std::size_t count,
    void* userData)
{
    TransferState* state = static_cast<TransferState*>(userData);
    if (state == nullptr || state->response == nullptr || data == nullptr)
    {
        return 0;
    }

    if (size != 0U && count > SIZE_MAX / size)
    {
        return 0;
    }
    const std::size_t bytes = size * count;
    if (state->response->body.size() > state->maximumBytes ||
        bytes > state->maximumBytes - state->response->body.size())
    {
        return 0;
    }

    state->response->body.append(data, bytes);
    return bytes;
}

std::size_t headerCallback(
    char* data,
    std::size_t size,
    std::size_t count,
    void* userData)
{
    TransferState* state = static_cast<TransferState*>(userData);
    if (state == nullptr || state->response == nullptr || data == nullptr)
    {
        return 0;
    }
    if (size != 0U && count > SIZE_MAX / size)
    {
        return 0;
    }

    const std::size_t bytes = size * count;
    std::string header(data, bytes);
    const std::size_t separator = header.find(':');
    if (separator == std::string::npos)
    {
        return bytes;
    }

    const std::string name = lowerAscii(trimAscii(header.substr(0, separator)));
    const std::string value = trimAscii(header.substr(separator + 1));
    if (name == "content-type")
    {
        const std::size_t parameter = value.find(';');
        state->response->contentType = lowerAscii(trimAscii(
            value.substr(0, parameter)));
    }
    else if (name == "location")
    {
        if (value.size() <= 8192U && !containsControl(value))
        {
            state->response->location = value;
        }
    }
    else if (name == "retry-after")
    {
        errno = 0;
        char* end = nullptr;
        const long parsed = std::strtol(value.c_str(), &end, 10);
        if (errno == 0 && end != value.c_str() && end != nullptr &&
            *end == '\0' && parsed >= 0 && parsed <= 3600)
        {
            state->response->retryAfterSeconds = parsed;
        }
    }
    return bytes;
}

bool setOption(CURL* handle, CURLoption option, long value)
{
    return curl_easy_setopt(handle, option, value) == CURLE_OK;
}

bool isTokenSafe(const std::string& token)
{
    if (token.size() > 4096U || containsControl(token))
    {
        return false;
    }
    return std::none_of(
        token.begin(),
        token.end(),
        [](const unsigned char character) {
            return std::isspace(character) != 0;
        });
}
}

CurlExternalArtworkHttpTransport::CurlExternalArtworkHttpTransport(
    CurlExternalArtworkHttpTransportConfig config)
    : config_(std::move(config))
{
}

bool CurlExternalArtworkHttpTransport::isAllowedHttpsUrl(
    const std::string& url,
    const std::set<std::string>& allowedHosts)
{
    static const std::string Prefix = "https://";
    if (url.size() <= Prefix.size() ||
        url.compare(0, Prefix.size(), Prefix) != 0 ||
        url.size() > 8192U || containsControl(url))
    {
        return false;
    }

    const std::size_t authorityEnd = url.find('/', Prefix.size());
    const std::string authority = url.substr(
        Prefix.size(),
        authorityEnd == std::string::npos
            ? std::string::npos
            : authorityEnd - Prefix.size());
    if (authority.empty() || authority.find('@') != std::string::npos ||
        authority.find(':') != std::string::npos)
    {
        return false;
    }

    const std::string host = lowerAscii(authority);
    if (allowedHosts.find(host) == allowedHosts.end())
    {
        return false;
    }

    const std::string path = authorityEnd == std::string::npos
        ? "/"
        : url.substr(authorityEnd);
    return !path.empty() && path.front() == '/' &&
        path.find("\\") == std::string::npos;
}

ExternalArtworkHttpResponse CurlExternalArtworkHttpTransport::perform(
    const ExternalArtworkHttpRequest& request)
{
    ExternalArtworkHttpResponse response;
    response.attempted = true;

    if (!isAllowedHttpsUrl(request.url, config_.allowedHosts) ||
        !isTokenSafe(request.bearerToken) ||
        request.connectTimeoutMs < 100 ||
        request.connectTimeoutMs > 10000 ||
        request.totalTimeoutMs < request.connectTimeoutMs ||
        request.totalTimeoutMs > 30000 ||
        request.maximumResponseBytes < 1U ||
        request.maximumResponseBytes > 32U * 1024U * 1024U ||
        request.accept.size() > 128U || containsControl(request.accept))
    {
        response.transportError = true;
        return response;
    }

    std::call_once(curlInitialization, initializeCurl);
    if (!curlInitialized)
    {
        response.transportError = true;
        return response;
    }

    CURL* handle = curl_easy_init();
    if (handle == nullptr)
    {
        response.transportError = true;
        return response;
    }

    curl_slist* headers = nullptr;
    bool headersReady = true;
    if (!request.bearerToken.empty())
    {
        const std::string authorization =
            "Authorization: Bearer " + request.bearerToken;
        curl_slist* appended = curl_slist_append(
            headers,
            authorization.c_str());
        headersReady = appended != nullptr;
        if (headersReady) headers = appended;
    }
    if (headersReady && !request.accept.empty())
    {
        const std::string accept = "Accept: " + request.accept;
        curl_slist* appended = curl_slist_append(headers, accept.c_str());
        headersReady = appended != nullptr;
        if (headersReady) headers = appended;
    }

    TransferState state;
    state.response = &response;
    state.maximumBytes = request.maximumResponseBytes;

    bool configured = headersReady;
    configured = configured &&
        curl_easy_setopt(handle, CURLOPT_URL, request.url.c_str()) == CURLE_OK;
    configured = configured &&
        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers) == CURLE_OK;
    configured = configured &&
        curl_easy_setopt(handle, CURLOPT_USERAGENT, config_.userAgent.c_str()) == CURLE_OK;
    configured = configured && setOption(handle, CURLOPT_NOSIGNAL, 1L);
    configured = configured && setOption(handle, CURLOPT_FOLLOWLOCATION, 0L);
    configured = configured && setOption(handle, CURLOPT_MAXREDIRS, 0L);
    configured = configured && setOption(handle, CURLOPT_SSL_VERIFYPEER, 1L);
    configured = configured && setOption(handle, CURLOPT_SSL_VERIFYHOST, 2L);
    configured = configured && setOption(handle, CURLOPT_NETRC, CURL_NETRC_IGNORED);
    configured = configured &&
        curl_easy_setopt(handle, CURLOPT_PROXY, "") == CURLE_OK;
    configured = configured &&
        curl_easy_setopt(handle, CURLOPT_PROTOCOLS_STR, "https") == CURLE_OK;
    configured = configured &&
        curl_easy_setopt(handle, CURLOPT_REDIR_PROTOCOLS_STR, "https") == CURLE_OK;
    configured = configured &&
        curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT_MS,
                         static_cast<long>(request.connectTimeoutMs)) == CURLE_OK;
    configured = configured &&
        curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS,
                         static_cast<long>(request.totalTimeoutMs)) == CURLE_OK;
    configured = configured &&
        curl_easy_setopt(handle, CURLOPT_MAXFILESIZE_LARGE,
                         static_cast<curl_off_t>(request.maximumResponseBytes)) == CURLE_OK;
    configured = configured &&
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeCallback) == CURLE_OK;
    configured = configured &&
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, &state) == CURLE_OK;
    configured = configured &&
        curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, headerCallback) == CURLE_OK;
    configured = configured &&
        curl_easy_setopt(handle, CURLOPT_HEADERDATA, &state) == CURLE_OK;
    configured = configured &&
        curl_easy_setopt(handle, CURLOPT_OPENSOCKETFUNCTION, openSocketCallback) == CURLE_OK;
    configured = configured &&
        curl_easy_setopt(handle, CURLOPT_OPENSOCKETDATA, nullptr) == CURLE_OK;

    CURLcode result = CURLE_FAILED_INIT;
    if (configured)
    {
        result = curl_easy_perform(handle);
        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &response.statusCode);
    }

    response.transportError = !configured || result != CURLE_OK;
    if (response.transportError)
    {
        response.body.clear();
        response.contentType.clear();
        response.location.clear();
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(handle);
    return response;
}
