#include "MediaPlaybackContractResponse.h"

#include <cassert>
#include <string>
#include <utility>

namespace
{

ApiResponse response(const std::string& body)
{
    ApiResponse value;
    value.statusCode = 200;
    value.contentType = "application/json";
    value.body = body;
    return value;
}

} // namespace

int main()
{
    ApiResponse progressive = response(
        "{\"mediaSession\":{"
        "\"id\":\"recording-1\","
        "\"state\":\"ready\","
        "\"presentationProfileId\":\"progressive-fmp4\","
        "\"growing\":false,"
        "\"playback\":{"
            "\"positionSeconds\":120,"
            "\"durationSeconds\":5530,"
            "\"seek\":{\"supported\":true,\"preparing\":false,\"window\":{\"startSeconds\":0,\"endSeconds\":5530}},"
            "\"resume\":{\"supported\":true,\"preparing\":false}},"
        "\"tracks\":{"
            "\"audio\":{\"selectionSupported\":true},"
            "\"subtitles\":{\"selectionSupported\":true,\"offSupported\":true}}"
        "}}"
    );
    progressive = MediaPlaybackContractResponse::augment(std::move(progressive), false);
    assert(progressive.body.find("\"playbackContract\":{") != std::string::npos);
    assert(progressive.body.find("\"resourceMode\":\"recording\"") != std::string::npos);
    assert(progressive.body.find("\"mode\":\"in-session-reposition\"") != std::string::npos);
    assert(progressive.body.find("\"presentationBasePositionSeconds\":120") != std::string::npos);
    assert(progressive.body.find("\"audioSelection\":{\"supported\":true}") != std::string::npos);
    assert(progressive.body.find("\"subtitleOff\":{\"supported\":true}") != std::string::npos);
    assert(progressive.body.find("\"playback\":{\"positionSeconds\":120") != std::string::npos);

    ApiResponse hls = response(
        "{\"mediaSession\":{"
        "\"id\":\"recording-2\","
        "\"state\":\"ready\","
        "\"presentationProfileId\":\"hls-fmp4\","
        "\"growing\":false,"
        "\"playback\":{"
            "\"positionSeconds\":2832,"
            "\"durationSeconds\":5530,"
            "\"seek\":{\"supported\":false,\"preparing\":false},"
            "\"resume\":{\"supported\":true,\"preparing\":false}}"
        "}}"
    );
    hls = MediaPlaybackContractResponse::augment(std::move(hls), false);
    assert(hls.body.find("\"mode\":\"replacement-session-restart\"") != std::string::npos);
    assert(hls.body.find("\"seek\":{\"supported\":true") != std::string::npos);
    assert(hls.body.find("\"presentationBasePositionSeconds\":2832") != std::string::npos);

    ApiResponse tracksOnly = response(
        "{\"mediaSession\":{"
        "\"id\":\"recording-3\","
        "\"state\":\"ready\","
        "\"presentationProfileId\":\"hls-fmp4\","
        "\"growing\":true,"
        "\"tracks\":{"
            "\"audio\":{\"selectionSupported\":false},"
            "\"subtitles\":{\"selectionSupported\":true,\"offSupported\":true}}"
        "}}"
    );
    tracksOnly = MediaPlaybackContractResponse::augment(std::move(tracksOnly), false);
    assert(tracksOnly.body.find("\"resourceMode\":\"growing-recording\"") != std::string::npos);
    assert(tracksOnly.body.find("\"positionSeconds\":null") != std::string::npos);
    assert(tracksOnly.body.find("\"supported\":null,\"mode\":\"replacement-session-restart\"") != std::string::npos);
    assert(tracksOnly.body.find("\"audioSelection\":{\"supported\":false}") != std::string::npos);
    assert(tracksOnly.body.find("\"subtitleSelection\":{\"supported\":true}") != std::string::npos);

    ApiResponse growing = response(
        "{\"mediaSession\":{"
        "\"id\":\"recording-4\","
        "\"state\":\"ready\","
        "\"presentationProfileId\":\"hls-fmp4\","
        "\"growing\":true,"
        "\"playback\":{"
            "\"positionSeconds\":0,"
            "\"durationSeconds\":null,"
            "\"seek\":{\"supported\":false,\"preparing\":false},"
            "\"resume\":{\"supported\":false,\"preparing\":false}}"
        "}}"
    );
    growing = MediaPlaybackContractResponse::augment(std::move(growing), false);
    assert(growing.body.find("\"resourceMode\":\"growing-recording\"") != std::string::npos);
    assert(growing.body.find("\"durationSeconds\":null") != std::string::npos);
    assert(growing.body.find("\"seek\":{\"supported\":false,\"mode\":\"replacement-session-restart\"") != std::string::npos);

    ApiResponse live = response(
        "{\"mediaSession\":{"
        "\"id\":\"live-1\","
        "\"resourceKind\":\"live-channel\","
        "\"state\":\"ready\","
        "\"presentationProfileId\":\"progressive-fmp4\","
        "\"mediaPath\":\"/api/media/sessions/live-1/live/stream.mp4\""
        "}}"
    );
    live = MediaPlaybackContractResponse::augment(std::move(live), true);
    assert(live.body.find("\"resourceMode\":\"live\"") != std::string::npos);
    assert(live.body.find("\"mode\":\"unsupported\"") != std::string::npos);
    assert(live.body.find("\"positionSeconds\":null") != std::string::npos);

    ApiResponse ended = response(
        "{\"mediaSession\":{\"id\":\"recording-5\",\"state\":\"ended\"}}"
    );
    const std::string endedBefore = ended.body;
    ended = MediaPlaybackContractResponse::augment(std::move(ended), false);
    assert(ended.body == endedBefore);

    ApiResponse failure;
    failure.statusCode = 503;
    failure.contentType = "application/json";
    failure.body = "{\"error\":{\"code\":\"media_provision_failed\"}}";
    const std::string failureBefore = failure.body;
    failure = MediaPlaybackContractResponse::augment(std::move(failure), false);
    assert(failure.body == failureBefore);

    return 0;
}
