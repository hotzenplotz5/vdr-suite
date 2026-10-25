#include "FirstAdminClaimHttpService.h"

#include "FirstAdminClaimService.h"
#include "FirstAdminBootstrapRepository.h"
#include "FirstAdminOnscreenSetup.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <sstream>
#include <string>
#include <utility>

namespace
{
constexpr const char* ClaimPath = "/api/security/first-admin/claim";
constexpr const char* SetupStatusPath = "/api/security/first-admin/status";
constexpr const char* OsdStartPath = "/api/security/first-admin/onscreen/start";
constexpr const char* OsdClaimPath = "/api/security/first-admin/onscreen/claim";
constexpr std::size_t MaximumBodyBytes = 8192U;
constexpr unsigned BootstrapIdField = 1U << 0U;
constexpr unsigned SetupSecretField = 1U << 1U;
constexpr unsigned LoginNameField = 1U << 2U;
constexpr unsigned PasswordField = 1U << 3U;
constexpr unsigned DisplayNameField = 1U << 4U;
constexpr unsigned OnscreenFields = SetupSecretField | LoginNameField |
    PasswordField | DisplayNameField;
constexpr unsigned RequiredFields =
    BootstrapIdField |
    SetupSecretField |
    LoginNameField |
    PasswordField |
    DisplayNameField;

struct RequestContext
{
    std::string requestId;
    std::string correlationId;
};

std::string requestPath(const std::string& target)
{
    const std::size_t query = target.find('?');
    return query == std::string::npos
        ? target
        : target.substr(0, query);
}

std::string lowerAscii(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });
    return value;
}

std::string trimAscii(std::string value)
{
    const auto first = std::find_if(
        value.begin(),
        value.end(),
        [](unsigned char character)
        {
            return std::isspace(character) == 0;
        });
    const auto last = std::find_if(
        value.rbegin(),
        value.rend(),
        [](unsigned char character)
        {
            return std::isspace(character) == 0;
        }).base();

    if (first >= last)
    {
        return {};
    }
    return std::string(first, last);
}

bool sameHeaderName(
    const std::string& left,
    const std::string& right)
{
    return lowerAscii(left) == lowerAscii(right);
}

std::string headerValue(
    const HttpServerRequest& request,
    const std::string& name)
{
    for (const auto& header : request.headers)
    {
        if (sameHeaderName(header.first, name))
        {
            return header.second;
        }
    }
    return {};
}

bool jsonContentType(const HttpServerRequest& request)
{
    const std::string contentType =
        lowerAscii(trimAscii(headerValue(request, "Content-Type")));
    if (contentType == "application/json")
    {
        return true;
    }
    return contentType.rfind("application/json;", 0) == 0;
}

bool safeContextToken(const std::string& value)
{
    if (value.empty() || value.size() > 128U)
    {
        return false;
    }

    return std::all_of(
        value.begin(),
        value.end(),
        [](unsigned char character)
        {
            return std::isalnum(character) != 0 ||
                character == '-' ||
                character == '_' ||
                character == '.' ||
                character == ':';
        });
}

void secureWipe(std::string& value) noexcept
{
    volatile char* bytes = value.empty()
        ? nullptr
        : const_cast<volatile char*>(value.data());
    for (std::size_t index = 0; index < value.size(); ++index)
    {
        bytes[index] = 0;
    }
    value.clear();
}

std::size_t skipWhitespace(
    const std::string& value,
    std::size_t position)
{
    while (position < value.size() &&
           std::isspace(
               static_cast<unsigned char>(value[position])) != 0)
    {
        ++position;
    }
    return position;
}

int hexValue(char character)
{
    if (character >= '0' && character <= '9')
    {
        return character - '0';
    }
    if (character >= 'a' && character <= 'f')
    {
        return character - 'a' + 10;
    }
    if (character >= 'A' && character <= 'F')
    {
        return character - 'A' + 10;
    }
    return -1;
}

bool parseHexCodeUnit(
    const std::string& input,
    std::size_t& position,
    std::uint32_t& codeUnit)
{
    if (position + 4U > input.size())
    {
        return false;
    }

    codeUnit = 0U;
    for (std::size_t index = 0; index < 4U; ++index)
    {
        const int value = hexValue(input[position + index]);
        if (value < 0)
        {
            return false;
        }
        codeUnit =
            (codeUnit << 4U) |
            static_cast<std::uint32_t>(value);
    }
    position += 4U;
    return true;
}

bool appendUtf8(
    std::string& output,
    std::uint32_t codePoint)
{
    if (codePoint > 0x10ffffU ||
        (codePoint >= 0xd800U && codePoint <= 0xdfffU))
    {
        return false;
    }

    if (codePoint <= 0x7fU)
    {
        output.push_back(static_cast<char>(codePoint));
    }
    else if (codePoint <= 0x7ffU)
    {
        output.push_back(
            static_cast<char>(0xc0U | (codePoint >> 6U)));
        output.push_back(
            static_cast<char>(0x80U | (codePoint & 0x3fU)));
    }
    else if (codePoint <= 0xffffU)
    {
        output.push_back(
            static_cast<char>(0xe0U | (codePoint >> 12U)));
        output.push_back(
            static_cast<char>(
                0x80U | ((codePoint >> 6U) & 0x3fU)));
        output.push_back(
            static_cast<char>(0x80U | (codePoint & 0x3fU)));
    }
    else
    {
        output.push_back(
            static_cast<char>(0xf0U | (codePoint >> 18U)));
        output.push_back(
            static_cast<char>(
                0x80U | ((codePoint >> 12U) & 0x3fU)));
        output.push_back(
            static_cast<char>(
                0x80U | ((codePoint >> 6U) & 0x3fU)));
        output.push_back(
            static_cast<char>(0x80U | (codePoint & 0x3fU)));
    }
    return true;
}

bool parseJsonString(
    const std::string& input,
    std::size_t& position,
    std::string& output)
{
    position = skipWhitespace(input, position);
    if (position >= input.size() || input[position] != '"')
    {
        return false;
    }

    ++position;
    output.clear();

    while (position < input.size())
    {
        const unsigned char character =
            static_cast<unsigned char>(input[position++]);

        if (character == '"')
        {
            return true;
        }
        if (character < 0x20U)
        {
            secureWipe(output);
            return false;
        }
        if (character != '\\')
        {
            output.push_back(static_cast<char>(character));
            continue;
        }

        if (position >= input.size())
        {
            secureWipe(output);
            return false;
        }

        const char escaped = input[position++];
        switch (escaped)
        {
            case '"':
            case '\\':
            case '/':
                output.push_back(escaped);
                break;
            case 'b':
                output.push_back('\b');
                break;
            case 'f':
                output.push_back('\f');
                break;
            case 'n':
                output.push_back('\n');
                break;
            case 'r':
                output.push_back('\r');
                break;
            case 't':
                output.push_back('\t');
                break;
            case 'u':
            {
                std::uint32_t first = 0U;
                if (!parseHexCodeUnit(input, position, first))
                {
                    secureWipe(output);
                    return false;
                }

                std::uint32_t codePoint = first;
                if (first >= 0xd800U && first <= 0xdbffU)
                {
                    if (position + 2U > input.size() ||
                        input[position] != '\\' ||
                        input[position + 1U] != 'u')
                    {
                        secureWipe(output);
                        return false;
                    }
                    position += 2U;

                    std::uint32_t second = 0U;
                    if (!parseHexCodeUnit(
                            input,
                            position,
                            second) ||
                        second < 0xdc00U ||
                        second > 0xdfffU)
                    {
                        secureWipe(output);
                        return false;
                    }

                    codePoint =
                        0x10000U +
                        ((first - 0xd800U) << 10U) +
                        (second - 0xdc00U);
                }
                else if (first >= 0xdc00U &&
                         first <= 0xdfffU)
                {
                    secureWipe(output);
                    return false;
                }

                if (!appendUtf8(output, codePoint))
                {
                    secureWipe(output);
                    return false;
                }
                break;
            }
            default:
                secureWipe(output);
                return false;
        }
    }

    secureWipe(output);
    return false;
}

bool assignClaimField(
    const std::string& key,
    std::string value,
    unsigned& fields,
    FirstAdminClaimRequest& request)
{
    auto assign =
        [&](unsigned field, std::string& target)
        {
            if ((fields & field) != 0U)
            {
                secureWipe(value);
                return false;
            }
            fields |= field;
            target = std::move(value);
            return true;
        };

    if (key == "bootstrapId")
    {
        return assign(BootstrapIdField, request.bootstrapId);
    }
    if (key == "onscreenCode")
    {
        return assign(SetupSecretField, request.setupSecret);
    }
    if (key == "setupSecret")
    {
        return assign(SetupSecretField, request.setupSecret);
    }
    if (key == "loginName")
    {
        return assign(LoginNameField, request.loginName);
    }
    if (key == "password")
    {
        return assign(PasswordField, request.password);
    }
    if (key == "displayName")
    {
        return assign(DisplayNameField, request.displayName);
    }

    secureWipe(value);
    return false;
}

bool failClaimBody(FirstAdminClaimRequest& request)
{
    request.clearSecrets();
    return false;
}

bool parseClaimBody(
    const std::string& body,
    FirstAdminClaimRequest& request,
    unsigned expectedFields = RequiredFields)
{
    if (body.empty() || body.size() > MaximumBodyBytes)
    {
        return false;
    }

    std::size_t position = skipWhitespace(body, 0U);
    if (position >= body.size() || body[position] != '{')
    {
        return false;
    }

    ++position;
    unsigned fields = 0U;
    position = skipWhitespace(body, position);
    if (position < body.size() && body[position] == '}')
    {
        return false;
    }

    while (position < body.size())
    {
        std::string key;
        std::string value;
        if (!parseJsonString(body, position, key))
        {
            return failClaimBody(request);
        }

        position = skipWhitespace(body, position);
        if (position >= body.size() || body[position] != ':')
        {
            secureWipe(key);
            return failClaimBody(request);
        }
        ++position;

        if (!parseJsonString(body, position, value))
        {
            secureWipe(key);
            return failClaimBody(request);
        }

        const bool assigned =
            assignClaimField(
                key,
                std::move(value),
                fields,
                request);
        secureWipe(key);
        if (!assigned)
        {
            return failClaimBody(request);
        }

        position = skipWhitespace(body, position);
        if (position >= body.size())
        {
            return failClaimBody(request);
        }

        if (body[position] == '}')
        {
            ++position;
            break;
        }
        if (body[position] != ',')
        {
            return failClaimBody(request);
        }
        ++position;
    }

    position = skipWhitespace(body, position);
    if (position != body.size() ||
        fields != expectedFields)
    {
        return failClaimBody(request);
    }

    return true;
}

std::string jsonEscape(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const char character : value)
    {
        switch (character)
        {
            case '"':
                escaped += "\\\"";
                break;
            case '\\':
                escaped += "\\\\";
                break;
            default:
                escaped.push_back(character);
                break;
        }
    }
    return escaped;
}

HttpServerResponse response(
    int statusCode,
    const std::string& code,
    const std::string& message,
    const RequestContext& context)
{
    HttpServerResponse result;
    result.statusCode = statusCode;
    result.headers["Content-Type"] =
        "application/json; charset=utf-8";
    result.headers["Cache-Control"] = "no-store";
    result.headers["Pragma"] = "no-cache";
    result.headers["X-Content-Type-Options"] = "nosniff";
    result.headers["X-Request-ID"] = context.requestId;
    if (!context.correlationId.empty())
    {
        result.headers["X-Correlation-ID"] =
            context.correlationId;
    }
    result.body =
        "{\"error\":{\"code\":\"" + jsonEscape(code) +
        "\",\"message\":\"" + jsonEscape(message) +
        "\",\"requestId\":\"" +
        jsonEscape(context.requestId) + "\"}}";
    return result;
}

HttpServerResponse successResponse(
    const RequestContext& context)
{
    HttpServerResponse result;
    result.statusCode = 201;
    result.headers["Content-Type"] =
        "application/json; charset=utf-8";
    result.headers["Cache-Control"] = "no-store";
    result.headers["Pragma"] = "no-cache";
    result.headers["X-Content-Type-Options"] = "nosniff";
    result.headers["X-Request-ID"] = context.requestId;
    if (!context.correlationId.empty())
    {
        result.headers["X-Correlation-ID"] =
            context.correlationId;
    }
    result.body =
        "{\"status\":\"claimed\",\"requestId\":\"" +
        jsonEscape(context.requestId) + "\"}";
    return result;
}
}

FirstAdminClaimHttpService::FirstAdminClaimHttpService(
    FirstAdminClaimService& claimService,
    FirstAdminOnscreenSetup* onscreen)
    : claimService_(claimService), onscreen_(onscreen)
{
}

bool FirstAdminClaimHttpService::handles(
    const HttpServerRequest& request) const
{
    const std::string path = requestPath(request.path);
    return path == ClaimPath || path == SetupStatusPath ||
        path == OsdStartPath || path == OsdClaimPath;
}

HttpServerResponse FirstAdminClaimHttpService::handle(
    const HttpServerRequest& request) const
{
    RequestContext context;
    context.requestId =
        headerValue(request, "X-Request-ID");
    if (!safeContextToken(context.requestId))
    {
        context.requestId = opaqueId("req");
    }

    context.correlationId =
        headerValue(request, "X-Correlation-ID");
    if (!context.correlationId.empty() &&
        !safeContextToken(context.correlationId))
    {
        context.correlationId.clear();
    }

    if (requestPath(request.path) == SetupStatusPath)
    {
        if (request.method != "GET")
        {
            HttpServerResponse rejected = response(
                405, "method_not_allowed",
                "First-admin setup status requires GET", context);
            rejected.headers["Allow"] = "GET";
            return rejected;
        }
        const FirstAdminClaimState state = claimService_.claimState();
        if (state == FirstAdminClaimState::unavailable)
        {
            return response(503, "first_admin_status_unavailable",
                "First-admin status is unavailable", context);
        }
        HttpServerResponse result;
        result.statusCode = 200;
        result.headers["Content-Type"] =
            "application/json; charset=utf-8";
        result.headers["Cache-Control"] = "no-store";
        result.headers["Pragma"] = "no-cache";
        result.headers["X-Content-Type-Options"] = "nosniff";
        result.headers["X-Request-ID"] = context.requestId;
        result.body = state == FirstAdminClaimState::unclaimed
            ? "{\"unclaimed\":true}"
            : "{\"unclaimed\":false}";
        return result;
    }

    const std::string path = requestPath(request.path);
    if (path == OsdStartPath || path == OsdClaimPath)
    {
        if (!onscreen_)
            return response(503, "onscreen_setup_unavailable",
                "Local VDR confirmation is unavailable", context);
        if (request.method != "POST")
        {
            HttpServerResponse rejected = response(405,
                "method_not_allowed", "On-screen setup requires POST", context);
            rejected.headers["Allow"] = "POST";
            return rejected;
        }
        // Never send account credentials on an ordinary HTTP browser origin.
        // This is a browser safety fence, not a replacement for TLS on the proxy.
        const std::string origin = headerValue(request, "Origin");
        if (origin.rfind("https://", 0) != 0)
            return response(403, "https_required",
                "First-admin web setup requires HTTPS", context);
        if (!jsonContentType(request) || request.body.size() > MaximumBodyBytes)
            return response(415, "invalid_content_type",
                "First-admin setup requires bounded JSON", context);
        if (path == OsdStartPath)
        {
            if (trimAscii(request.body) != "{}")
                return response(400, "invalid_setup_start",
                    "Setup start has no parameters", context);
            const auto result = onscreen_->start();
            if (result.status == FirstAdminOnscreenStatus::ready)
            {
                HttpServerResponse accepted;
                accepted.statusCode = 202;
                accepted.headers["Content-Type"] = "application/json; charset=utf-8";
                accepted.headers["Cache-Control"] = "no-store";
                accepted.body = "{\"status\":\"displayed\"}";
                return accepted;
            }
            if (result.status == FirstAdminOnscreenStatus::claimed)
                return response(409, "server_already_claimed",
                    "The server already has an administrator", context);
            if (result.status == FirstAdminOnscreenStatus::throttled)
                return response(429, "setup_request_throttled",
                    "Please wait before requesting another TV code", context);
            return response(503, "osd_unavailable",
                "No local VDR on-screen display is reachable", context);
        }
        FirstAdminClaimRequest account;
        if (!parseClaimBody(request.body, account, OnscreenFields))
            return response(400, "invalid_onscreen_claim",
                "Invalid administrator form", context);
        const std::string code = std::move(account.setupSecret);
        account.setupSecret.clear();
        account.requestId = context.requestId;
        account.correlationId = context.correlationId;
        const auto result = onscreen_->complete(code, std::move(account));
        switch (result.status)
        {
            case FirstAdminOnscreenStatus::ready:
                return successResponse(context);
            case FirstAdminOnscreenStatus::claimed:
                return response(409, "server_already_claimed",
                    "The server already has an administrator", context);
            case FirstAdminOnscreenStatus::rejected:
                return response(401, "invalid_onscreen_code",
                    "Invalid local confirmation code", context);
            case FirstAdminOnscreenStatus::expired:
                return response(410, "onscreen_code_expired",
                    "Request another code on the TV", context);
            case FirstAdminOnscreenStatus::invalid:
                return response(400, "invalid_onscreen_claim",
                    "Invalid administrator form", context);
            default:
                return response(503, "onscreen_setup_unavailable",
                    "Could not finish the authorized setup", context);
        }
    }

    if (request.method != "POST")
    {
        HttpServerResponse result = response(
            405,
            "method_not_allowed",
            "First-admin claim requires POST",
            context);
        result.headers["Allow"] = "POST";
        return result;
    }

    if (!jsonContentType(request))
    {
        return response(
            415,
            "unsupported_media_type",
            "First-admin claim requires application/json",
            context);
    }

    FirstAdminClaimRequest claimRequest;
    if (!parseClaimBody(request.body, claimRequest))
    {
        return response(
            400,
            "invalid_first_admin_claim",
            "The first-admin claim request is invalid",
            context);
    }

    claimRequest.requestId = context.requestId;
    claimRequest.correlationId = context.correlationId;

    const FirstAdminClaimResult result =
        claimService_.claim(std::move(claimRequest));

    switch (result.status)
    {
        case FirstAdminClaimStatus::success:
            return successResponse(context);
        case FirstAdminClaimStatus::invalidRequest:
            return response(
                400,
                "invalid_first_admin_claim",
                "The first-admin claim request is invalid",
                context);
        case FirstAdminClaimStatus::claimed:
            return response(
                409,
                "server_already_claimed",
                "The server is already claimed",
                context);
        case FirstAdminClaimStatus::bootstrapNotFound:
        case FirstAdminClaimStatus::bootstrapRejected:
            return response(
                401,
                "invalid_bootstrap_proof",
                "The bootstrap proof is invalid",
                context);
        case FirstAdminClaimStatus::bootstrapExpired:
            return response(
                410,
                "bootstrap_expired",
                "The bootstrap proof has expired",
                context);
        case FirstAdminClaimStatus::bootstrapConsumed:
            return response(
                409,
                "bootstrap_consumed",
                "The bootstrap proof has already been consumed",
                context);
        case FirstAdminClaimStatus::bootstrapInvalidated:
            return response(
                410,
                "bootstrap_invalidated",
                "The bootstrap proof has been invalidated",
                context);
        case FirstAdminClaimStatus::conflict:
            return response(
                409,
                "first_admin_claim_conflict",
                "The requested first-admin identity conflicts with existing state",
                context);
        case FirstAdminClaimStatus::entropyUnavailable:
        case FirstAdminClaimStatus::hashingUnavailable:
        case FirstAdminClaimStatus::storageError:
        default:
            return response(
                503,
                "first_admin_claim_unavailable",
                "First-admin claim is temporarily unavailable",
                context);
    }
}

std::string FirstAdminClaimHttpService::opaqueId(
    const std::string& prefix) const
{
    const auto ticks =
        std::chrono::steady_clock::now()
            .time_since_epoch()
            .count();
    const unsigned long long sequence =
        idCounter_.fetch_add(1U);

    return prefix + "-" +
        std::to_string(ticks) + "-" +
        std::to_string(sequence);
}
