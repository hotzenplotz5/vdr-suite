#include "IVdrAdapter.h"
#include "MockHttpClient.h"
#include "RestfulApiVdrAdapter.h"
#include "VdrChannel.h"
#include "VdrConfig.h"
#include "VdrEvent.h"
#include "VdrRecording.h"
#include "VdrStatus.h"

#include <cassert>
#include <memory>
#include <string>
#include <vector>

static VdrConfig make_restfulapi_config()
{
    VdrConfig config;
    config.enabled = true;
    config.mode = "restfulapi";
    config.host = "127.0.0.1";
    config.port = 8002;
    return config;
}

static void test_restful_api_vdr_adapter_reports_status()
{
    VdrConfig config = make_restfulapi_config();
    MockHttpClient httpClient;

    HttpResponse okResponse;
    okResponse.statusCode = 200;
    okResponse.headers["Content-Type"] = "application/json";
    okResponse.body = "{\"status\":\"ok\"}";

    httpClient.setResponse(okResponse);

    RestfulApiVdrAdapter adapter(config, httpClient);

    VdrStatus status = adapter.getStatus();

    assert(status.enabled == true);
    assert(status.mode == "restfulapi");
    assert(status.host == "127.0.0.1");
    assert(status.port == 8002);
    assert(status.state == "restfulapi");

    assert(httpClient.requestCount() == 1);
    assert(httpClient.lastRequest().method == "GET");
    assert(httpClient.lastRequest().url == "/info.json");
    assert(httpClient.lastRequest().headers.at("Accept") == "application/json");
}

static void test_restful_api_vdr_adapter_reports_error_status()
{
    VdrConfig config = make_restfulapi_config();
    MockHttpClient httpClient;

    HttpResponse errorResponse;
    errorResponse.statusCode = 500;
    errorResponse.body = "{\"status\":\"ok\"}";

    httpClient.setResponse(errorResponse);

    RestfulApiVdrAdapter adapter(config, httpClient);
    VdrStatus errorStatus = adapter.getStatus();

    assert(errorStatus.enabled == true);
    assert(errorStatus.mode == "restfulapi");
    assert(errorStatus.host == "127.0.0.1");
    assert(errorStatus.port == 8002);
    assert(errorStatus.state == "error");
}

static void test_restful_api_vdr_adapter_reports_error_for_invalid_status_json()
{
    VdrConfig config = make_restfulapi_config();
    MockHttpClient httpClient;

    HttpResponse invalidResponse;
    invalidResponse.statusCode = 200;
    invalidResponse.body = "not json";

    httpClient.setResponse(invalidResponse);

    RestfulApiVdrAdapter adapter(config, httpClient);
    VdrStatus status = adapter.getStatus();

    assert(status.enabled == true);
    assert(status.mode == "restfulapi");
    assert(status.host == "127.0.0.1");
    assert(status.port == 8002);
    assert(status.state == "error");
}

static void test_restful_api_vdr_adapter_requests_channels_endpoint()
{
    VdrConfig config = make_restfulapi_config();
    MockHttpClient httpClient;

    HttpResponse channelsResponse;
    channelsResponse.statusCode = 200;
    channelsResponse.headers["Content-Type"] = "application/json";
    channelsResponse.body = "{\"channels\":[]}";

    httpClient.setResponse(channelsResponse);

    RestfulApiVdrAdapter adapter(config, httpClient);

    std::vector<VdrChannel> channels = adapter.getChannels();

    assert(channels.empty() == true);
    assert(httpClient.requestCount() == 1);
    assert(httpClient.lastRequest().method == "GET");
    assert(httpClient.lastRequest().url == "/channels.json");
    assert(httpClient.lastRequest().headers.at("Accept") == "application/json");
}

static void test_restful_api_vdr_adapter_maps_channels_response()
{
    VdrConfig config = make_restfulapi_config();
    MockHttpClient httpClient;

    HttpResponse channelsResponse;
    channelsResponse.statusCode = 200;
    channelsResponse.headers["Content-Type"] = "application/json";
    channelsResponse.body =
        "{\"channels\":["
        "{\"name\":\"DF1 HD\","
        "\"number\":149,"
        "\"channel_id\":\"C-61441-10006-50021\","
        "\"image\":false,"
        "\"group\":\"Rest\","
        "\"transponder\":498,"
        "\"stream\":\"C-61441-10006-50021.ts\","
        "\"is_atsc\":false,"
        "\"is_cable\":true,"
        "\"is_terr\":false,"
        "\"is_sat\":false,"
        "\"is_radio\":false,"
        "\"index\":154"
        "},"
        "{\"name\":\"Radio Hamburg\","
        "\"number\":333,"
        "\"channel_id\":\"C-61441-10000-52876\","
        "\"image\":false,"
        "\"group\":\"Rest\","
        "\"transponder\":466,"
        "\"stream\":\"C-61441-10000-52876.ts\","
        "\"is_atsc\":false,"
        "\"is_cable\":true,"
        "\"is_terr\":false,"
        "\"is_sat\":false,"
        "\"is_radio\":true,"
        "\"index\":338"
        "}"
        "],\"count\":2,\"total\":2}";

    httpClient.setResponse(channelsResponse);

    RestfulApiVdrAdapter adapter(config, httpClient);
    std::vector<VdrChannel> channels = adapter.getChannels();

    assert(channels.size() == 2);

    assert(channels[0].id == "C-61441-10006-50021");
    assert(channels[0].number == 149);
    assert(channels[0].name == "DF1 HD");
    assert(channels[0].provider == "");
    assert(channels[0].group == "Rest");
    assert(channels[0].radio == false);
    assert(channels[0].encrypted == false);
    assert(channels[0].enabled == true);

    assert(channels[1].id == "C-61441-10000-52876");
    assert(channels[1].number == 333);
    assert(channels[1].name == "Radio Hamburg");
    assert(channels[1].radio == true);
}

static void test_restful_api_vdr_adapter_ignores_http_error_for_channels()
{
    VdrConfig config = make_restfulapi_config();
    MockHttpClient httpClient;

    HttpResponse errorResponse;
    errorResponse.statusCode = 500;
    errorResponse.body = "error";

    httpClient.setResponse(errorResponse);

    RestfulApiVdrAdapter adapter(config, httpClient);
    std::vector<VdrChannel> channels = adapter.getChannels();

    assert(channels.empty() == true);
}

static void test_restful_api_vdr_adapter_tolerates_invalid_channel_json()
{
    VdrConfig config = make_restfulapi_config();
    MockHttpClient httpClient;

    HttpResponse invalidResponse;
    invalidResponse.statusCode = 200;
    invalidResponse.body = "not json";

    httpClient.setResponse(invalidResponse);

    RestfulApiVdrAdapter adapter(config, httpClient);
    std::vector<VdrChannel> channels = adapter.getChannels();

    assert(channels.empty() == true);
}

static void test_restful_api_vdr_adapter_requests_events_endpoint()
{
    VdrConfig config = make_restfulapi_config();
    MockHttpClient httpClient;

    HttpResponse eventsResponse;
    eventsResponse.statusCode = 200;
    eventsResponse.headers["Content-Type"] = "application/json";
    eventsResponse.body = "{\"events\":[]}";

    httpClient.setResponse(eventsResponse);

    RestfulApiVdrAdapter adapter(config, httpClient);

    std::vector<VdrEvent> events = adapter.getEvents();

    assert(events.empty() == true);
    assert(httpClient.requestCount() == 1);
    assert(httpClient.lastRequest().method == "GET");
    assert(httpClient.lastRequest().url == "/events.json");
    assert(httpClient.lastRequest().headers.at("Accept") == "application/json");
}

static void test_restful_api_vdr_adapter_maps_events_response()
{
    VdrConfig config = make_restfulapi_config();
    MockHttpClient httpClient;

    HttpResponse eventsResponse;
    eventsResponse.statusCode = 200;
    eventsResponse.headers["Content-Type"] = "application/json";
    eventsResponse.body =
        "{\"events\":["
        "{\"id\":56748,"
        "\"title\":\"kinokino\","
        "\"short_text\":\"Filmmagazin\","
        "\"description\":\"Filmmagazin Beschreibung\","
        "\"start_time\":1780453800,"
        "\"channel\":\"C-61441-10023-10376\","
        "\"channel_name\":\"ONE HD\","
        "\"duration\":900,"
        "\"table_id\":80,"
        "\"version\":11,"
        "\"images\":0,"
        "\"timer_exists\":false,"
        "\"timer_active\":false,"
        "\"timer_id\":\"\","
        "\"parental_rating\":0,"
        "\"vps\":1780453800,"
        "\"components\":[],"
        "\"contents\":[],"
        "\"raw_contents\":[],"
        "\"details\":[],"
        "\"additional_media\":null"
        "},"
        "{\"id\":56761,"
        "\"title\":\"Zero\","
        "\"short_text\":\"Fernsehfilm Deutschland 2021\","
        "\"description\":\"Berlin in naher Zukunft\","
        "\"start_time\":1780487700,"
        "\"channel\":\"C-61441-10023-10376\","
        "\"channel_name\":\"ONE HD\","
        "\"duration\":5400,"
        "\"table_id\":80,"
        "\"version\":11,"
        "\"images\":0,"
        "\"timer_exists\":false,"
        "\"timer_active\":false,"
        "\"timer_id\":\"\","
        "\"parental_rating\":6,"
        "\"vps\":1780487700,"
        "\"components\":[],"
        "\"contents\":[\"Film/Drama\",\"Detektiv/Thriller\"],"
        "\"raw_contents\":[16,17],"
        "\"details\":[],"
        "\"additional_media\":{\"type\":\"movie\",\"title\":\"Zero\",\"actors\":[{\"name\":\"Heike Makatsch\"}]}"
        "}"
        "]}";

    httpClient.setResponse(eventsResponse);

    RestfulApiVdrAdapter adapter(config, httpClient);
    std::vector<VdrEvent> events = adapter.getEvents();

    assert(events.size() == 2);

    assert(events[0].id == "56748");
    assert(events[0].channelId == "C-61441-10023-10376");
    assert(events[0].title == "kinokino");
    assert(events[0].subtitle == "Filmmagazin");
    assert(events[0].description == "Filmmagazin Beschreibung");
    assert(events[0].startTime == "1780453800");
    assert(events[0].endTime == "1780454700");
    assert(events[0].durationSeconds == 900);
    assert(events[0].contentDescriptors.empty() == true);
    assert(events[0].parentalRating == 0);

    assert(events[1].id == "56761");
    assert(events[1].channelId == "C-61441-10023-10376");
    assert(events[1].title == "Zero");
    assert(events[1].subtitle == "Fernsehfilm Deutschland 2021");
    assert(events[1].description == "Berlin in naher Zukunft");
    assert(events[1].startTime == "1780487700");
    assert(events[1].endTime == "1780493100");
    assert(events[1].durationSeconds == 5400);
    assert(events[1].contentDescriptors.size() == 2);
    assert(events[1].contentDescriptors[0] == "Film/Drama");
    assert(events[1].contentDescriptors[1] == "Detektiv/Thriller");
    assert(events[1].parentalRating == 6);
}

static void test_restful_api_vdr_adapter_ignores_http_error_for_events()
{
    VdrConfig config = make_restfulapi_config();
    MockHttpClient httpClient;

    HttpResponse errorResponse;
    errorResponse.statusCode = 500;
    errorResponse.body = "error";

    httpClient.setResponse(errorResponse);

    RestfulApiVdrAdapter adapter(config, httpClient);
    std::vector<VdrEvent> events = adapter.getEvents();

    assert(events.empty() == true);
}

static void test_restful_api_vdr_adapter_tolerates_invalid_json()
{
    VdrConfig config = make_restfulapi_config();
    MockHttpClient httpClient;

    HttpResponse invalidResponse;
    invalidResponse.statusCode = 200;
    invalidResponse.body = "not json";

    httpClient.setResponse(invalidResponse);

    RestfulApiVdrAdapter adapter(config, httpClient);
    std::vector<VdrEvent> events = adapter.getEvents();

    assert(events.empty() == true);
}

static void test_restful_api_vdr_adapter_requests_recordings_endpoint()
{
    VdrConfig config = make_restfulapi_config();
    MockHttpClient httpClient;

    HttpResponse recordingsResponse;
    recordingsResponse.statusCode = 200;
    recordingsResponse.headers["Content-Type"] = "application/json";
    recordingsResponse.body = "{\"recordings\":[]}";

    httpClient.setResponse(recordingsResponse);

    RestfulApiVdrAdapter adapter(config, httpClient);

    std::vector<VdrRecording> recordings = adapter.getRecordings();

    assert(recordings.empty() == true);
    assert(httpClient.requestCount() == 1);
    assert(httpClient.lastRequest().method == "GET");
    assert(httpClient.lastRequest().url == "/recordings.json");
    assert(httpClient.lastRequest().headers.at("Accept") == "application/json");
}

static void test_restful_api_vdr_adapter_maps_recordings_response()
{
    VdrConfig config = make_restfulapi_config();
    MockHttpClient httpClient;

    HttpResponse recordingsResponse;
    recordingsResponse.statusCode = 200;
    recordingsResponse.headers["Content-Type"] = "application/json";
    recordingsResponse.body =
        "{\"recordings\":["
        "{\"number\":0,"
        "\"name\":\"Mystery~The Village - Das Dorf\","
        "\"file_name\":\"/srv/vdr/video/Mystery/The_Village_-_Das_Dorf/2010-10-31.02.29.10-0.rec\","
        "\"relative_file_name\":\"/Mystery/The_Village_-_Das_Dorf/2010-10-31.02.29.10-0.rec\","
        "\"duration\":5835,"
        "\"filesize_mb\":5555,"
        "\"event_start_time\":1288488540,"
        "\"event_duration\":5820"
        "},"
        "{\"number\":840,"
        "\"name\":\"Serien~The Walking Dead~S08E08 Kampf um die Zukunft\","
        "\"file_name\":\"/srv/vdr/video/Serien/The_Walking_Dead/S08E08_Kampf_um_die_Zukunft/2017-12-11.21.01.4-0.rec\","
        "\"relative_file_name\":\"/Serien/The_Walking_Dead/S08E08_Kampf_um_die_Zukunft/2017-12-11.21.01.4-0.rec\","
        "\"duration\":3551,"
        "\"filesize_mb\":4191,"
        "\"event_start_time\":1513022400,"
        "\"event_duration\":3900"
        "}"
        "]}";

    httpClient.setResponse(recordingsResponse);

    RestfulApiVdrAdapter adapter(config, httpClient);
    std::vector<VdrRecording> recordings = adapter.getRecordings();

    assert(recordings.size() == 2);

    assert(recordings[0].id == "0");
    assert(recordings[0].title == "Mystery~The Village - Das Dorf");
    assert(recordings[0].path == "/Mystery/The_Village_-_Das_Dorf/2010-10-31.02.29.10-0.rec");
    assert(recordings[0].startTime == "1288488540");
    assert(recordings[0].durationSeconds == 5835);
    assert(recordings[0].sizeMb == 5555);

    assert(recordings[1].id == "840");
    assert(recordings[1].title == "Serien~The Walking Dead~S08E08 Kampf um die Zukunft");
    assert(recordings[1].durationSeconds == 3551);
    assert(recordings[1].sizeMb == 4191);
}

static void test_restful_api_vdr_adapter_ignores_http_error_for_recordings()
{
    VdrConfig config = make_restfulapi_config();
    MockHttpClient httpClient;

    HttpResponse errorResponse;
    errorResponse.statusCode = 500;
    errorResponse.body = "error";

    httpClient.setResponse(errorResponse);

    RestfulApiVdrAdapter adapter(config, httpClient);
    std::vector<VdrRecording> recordings = adapter.getRecordings();

    assert(recordings.empty() == true);
}

static void test_restful_api_vdr_adapter_tolerates_invalid_recording_json()
{
    VdrConfig config = make_restfulapi_config();
    MockHttpClient httpClient;

    HttpResponse invalidResponse;
    invalidResponse.statusCode = 200;
    invalidResponse.body = "not json";

    httpClient.setResponse(invalidResponse);

    RestfulApiVdrAdapter adapter(config, httpClient);
    std::vector<VdrRecording> recordings = adapter.getRecordings();

    assert(recordings.empty() == true);
}

static void test_restful_api_vdr_adapter_can_be_used_through_interface()
{
    VdrConfig config = make_restfulapi_config();
    MockHttpClient httpClient;

    HttpResponse okResponse;
    okResponse.statusCode = 200;
    okResponse.body = "{\"status\":\"ok\"}";

    httpClient.setResponse(okResponse);

    std::unique_ptr<IVdrAdapter> interfaceAdapter =
        std::make_unique<RestfulApiVdrAdapter>(config, httpClient);

    VdrStatus interfaceStatus = interfaceAdapter->getStatus();

    assert(interfaceStatus.mode == "restfulapi");
}

int main()
{
    test_restful_api_vdr_adapter_reports_status();
    test_restful_api_vdr_adapter_reports_error_status();
    test_restful_api_vdr_adapter_reports_error_for_invalid_status_json();
    test_restful_api_vdr_adapter_requests_channels_endpoint();
    test_restful_api_vdr_adapter_maps_channels_response();
    test_restful_api_vdr_adapter_ignores_http_error_for_channels();
    test_restful_api_vdr_adapter_tolerates_invalid_channel_json();
    test_restful_api_vdr_adapter_requests_events_endpoint();
    test_restful_api_vdr_adapter_maps_events_response();
    test_restful_api_vdr_adapter_ignores_http_error_for_events();
    test_restful_api_vdr_adapter_tolerates_invalid_json();
    test_restful_api_vdr_adapter_requests_recordings_endpoint();
    test_restful_api_vdr_adapter_maps_recordings_response();
    test_restful_api_vdr_adapter_ignores_http_error_for_recordings();
    test_restful_api_vdr_adapter_tolerates_invalid_recording_json();
    test_restful_api_vdr_adapter_can_be_used_through_interface();

    return 0;
}
