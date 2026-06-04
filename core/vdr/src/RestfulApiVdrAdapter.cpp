#include "RestfulApiVdrAdapter.h"

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "RestfulApiChannelMapper.h"
#include "RestfulApiEventMapper.h"
#include "RestfulApiRecordingMapper.h"
#include "RestfulApiStatusMapper.h"
#include "RestfulApiTimerMapper.h"

#include <cctype>
#include <string>

namespace {
int parseIntField(const std::string& json, const std::string& fieldName)
{
    const std::string quotedField = "\"" + fieldName + "\"";
    const std::size_t fieldPosition = json.find(quotedField);

    if (fieldPosition == std::string::npos) {
        return 0;
    }

    const std::size_t colonPosition = json.find(':', fieldPosition + quotedField.size());

    if (colonPosition == std::string::npos) {
        return 0;
    }

    std::size_t valueStart = colonPosition + 1;

    while (valueStart < json.size() && std::isspace(static_cast<unsigned char>(json[valueStart]))) {
        ++valueStart;
    }

    std::size_t valueEnd = valueStart;

    while (valueEnd < json.size() && std::isdigit(static_cast<unsigned char>(json[valueEnd]))) {
        ++valueEnd;
    }

    if (valueEnd == valueStart) {
        return 0;
    }

    try {
        return std::stoi(json.substr(valueStart, valueEnd - valueStart));
    } catch (...) {
        return 0;
    }
}
}

RestfulApiVdrAdapter::RestfulApiVdrAdapter(VdrConfig config, IHttpClient& httpClient)
    : config_(config),
      httpClient_(httpClient)
{
}

VdrStatus RestfulApiVdrAdapter::getStatus() const
{
    HttpRequest request;
    request.method = "GET";
    request.url = "/info.json";
    request.headers["Accept"] = "application/json";

    HttpResponse response = httpClient_.execute(request);

    return RestfulApiStatusMapper::parseStatus(response.body, config_, response.statusCode);
}

std::vector<VdrChannel> RestfulApiVdrAdapter::getChannels() const
{
    HttpRequest request;
    request.method = "GET";
    request.url = "/channels.json";
    request.headers["Accept"] = "application/json";

    HttpResponse response = httpClient_.execute(request);

    if (response.statusCode != 200) {
        return {};
    }

    return RestfulApiChannelMapper::parseChannels(response.body);
}

std::vector<VdrEvent> RestfulApiVdrAdapter::getEvents() const
{
    HttpRequest request;
    request.method = "GET";
    request.url = "/events.json";
    request.headers["Accept"] = "application/json";

    HttpResponse response = httpClient_.execute(request);

    if (response.statusCode != 200) {
        return {};
    }

    return RestfulApiEventMapper::parseEvents(response.body);
}

std::vector<VdrTimer> RestfulApiVdrAdapter::getTimers() const
{
    HttpRequest request;
    request.method = "GET";
    request.url = "/timers.json";
    request.headers["Accept"] = "application/json";

    HttpResponse response = httpClient_.execute(request);

    if (response.statusCode != 200) {
        return {};
    }

    return RestfulApiTimerMapper::parseTimers(response.body);
}

std::vector<VdrRecording> RestfulApiVdrAdapter::getRecordings() const
{
    HttpRequest request;
    request.method = "GET";
    request.url = "/recordings.json";
    request.headers["Accept"] = "application/json";

    HttpResponse response = httpClient_.execute(request);

    if (response.statusCode != 200) {
        return {};
    }

    return RestfulApiRecordingMapper::parseRecordings(response.body);
}

VdrChangeState RestfulApiVdrAdapter::getChangeState() const
{
    HttpRequest request;
    request.method = "GET";
    request.url = "/change-state.json";
    request.headers["Accept"] = "application/json";

    HttpResponse response = httpClient_.execute(request);

    VdrChangeState state;

    if (response.statusCode != 200) {
        return state;
    }

    state.statusVersion = parseIntField(response.body, "statusVersion");
    state.channelsVersion = parseIntField(response.body, "channelsVersion");
    state.recordingsVersion = parseIntField(response.body, "recordingsVersion");
    state.timersVersion = parseIntField(response.body, "timersVersion");
    state.eventsVersion = parseIntField(response.body, "eventsVersion");

    return state;
}
