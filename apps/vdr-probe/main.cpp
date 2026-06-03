#include "BasicHttpClient.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "RestfulApiVdrAdapter.h"
#include "VdrChannel.h"
#include "VdrConfig.h"
#include "VdrEvent.h"
#include "VdrOverview.h"
#include "VdrOverviewJsonSerializer.h"
#include "VdrOverviewService.h"
#include "VdrRecording.h"
#include "VdrService.h"
#include "VdrStatus.h"
#include "VdrTimer.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::string previewBody(const std::string& body)
{
    constexpr std::size_t maxPreviewLength = 800;

    if (body.size() <= maxPreviewLength) {
        return body;
    }

    return body.substr(0, maxPreviewLength) + "\n... output truncated ...";
}

int parsePort(const char* value)
{
    return std::stoi(std::string(value));
}

void printRecordingPreview(const std::vector<VdrRecording>& recordings)
{
    std::cout << "Recordings: " << recordings.size() << std::endl;

    if (recordings.empty()) {
        return;
    }

    const VdrRecording& firstRecording = recordings.front();

    std::cout << "First recording:" << std::endl;
    std::cout << "  id: " << firstRecording.id << std::endl;
    std::cout << "  title: " << firstRecording.title << std::endl;
    std::cout << "  path: " << firstRecording.path << std::endl;
    std::cout << "  startTime: " << firstRecording.startTime << std::endl;
    std::cout << "  durationSeconds: " << firstRecording.durationSeconds << std::endl;
    std::cout << "  sizeMb: " << firstRecording.sizeMb << std::endl;
}

} // namespace

int main(int argc, char** argv)
{
    std::string host = "127.0.0.1";
    int port = 8002;

    if (argc >= 2) {
        host = argv[1];
    }

    if (argc >= 3) {
        port = parsePort(argv[2]);
    }

    try {
        std::cout << "VDR-Suite VDR probe" << std::endl;
        std::cout << "Host: " << host << std::endl;
        std::cout << "Port: " << port << std::endl;
        std::cout << "Endpoint: /info.json" << std::endl;
        std::cout << std::endl;

        BasicHttpClient httpClient(host, port);

        HttpRequest request;
        request.method = "GET";
        request.url = "/info.json";
        request.headers["Accept"] = "application/json";

        HttpResponse response = httpClient.execute(request);

        std::cout << "Raw HTTP status: " << response.statusCode << std::endl;
        std::cout << "Raw body size: " << response.body.size() << " bytes" << std::endl;
        std::cout << std::endl;
        std::cout << "Raw body preview:" << std::endl;
        std::cout << previewBody(response.body) << std::endl;
        std::cout << std::endl;

        VdrConfig config;
        config.enabled = true;
        config.mode = "restfulapi";
        config.host = host;
        config.port = port;

        RestfulApiVdrAdapter adapter(config, httpClient);
        VdrService vdrService(adapter);
        VdrOverviewService overviewService(vdrService);
        VdrOverviewJsonSerializer overviewJsonSerializer;

        VdrStatus status = vdrService.getStatus();

        std::cout << "Parsed VDR status:" << std::endl;
        std::cout << "enabled: " << (status.enabled ? "true" : "false") << std::endl;
        std::cout << "mode: " << status.mode << std::endl;
        std::cout << "host: " << status.host << std::endl;
        std::cout << "port: " << status.port << std::endl;
        std::cout << "state: " << status.state << std::endl;
        std::cout << std::endl;

        std::vector<VdrRecording> recordings = vdrService.getRecordings();
        std::vector<VdrTimer> timers = vdrService.getTimers();
        std::vector<VdrChannel> channels = vdrService.getChannels();
        std::vector<VdrEvent> events = vdrService.getEvents();

        std::cout << "Parsed VDR data:" << std::endl;
        printRecordingPreview(recordings);
        std::cout << "Timers: " << timers.size() << std::endl;
        std::cout << "Channels: " << channels.size() << std::endl;
        std::cout << "Events: " << events.size() << std::endl;
        std::cout << std::endl;

        VdrOverview overview = overviewService.getOverview();

        std::cout << "VDR overview JSON:" << std::endl;
        std::cout << overviewJsonSerializer.serialize(overview) << std::endl;

        return response.statusCode == 200 ? 0 : 2;
    } catch (const std::exception& ex) {
        std::cerr << "vdr-probe failed: " << ex.what() << std::endl;
        return 1;
    }
}
