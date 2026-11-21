#include "RestfulApiChannelMapper.h"
#include "VdrChannel.h"

#include <cassert>
#include <string>
#include <vector>

static void test_channel_mapper_maps_valid_channels_response()
{
    const std::string json =
        "{\"channels\":["
        "{\"name\":\"Das Erste HD\","
        "\"number\":1,"
        "\"channel_id\":\"C-1-1051-10301\","
        "\"image\":false,"
        "\"group\":\"ARD\","
        "\"encrypted\":false,"
        "\"caids\":[],"
        "\"transponder\":386,"
        "\"stream\":\"C-1-1051-10301.ts\","
        "\"is_atsc\":false,"
        "\"is_cable\":true,"
        "\"is_terr\":false,"
        "\"is_sat\":false,"
        "\"is_radio\":false,"
        "\"index\":1"
        "},"
        "{\"name\":\"Radio Hamburg\","
        "\"number\":333,"
        "\"channel_id\":\"C-61441-10000-52876\","
        "\"image\":false,"
        "\"group\":\"Rest\","
        "\"encrypted\":false,"
        "\"caids\":[],"
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

    std::vector<VdrChannel> channels = RestfulApiChannelMapper::parseChannels(json);

    assert(channels.size() == 2);

    assert(channels[0].id == "C-1-1051-10301");
    assert(channels[0].number == 1);
    assert(channels[0].name == "Das Erste HD");
    assert(channels[0].provider == "");
    assert(channels[0].group == "ARD");
    assert(channels[0].radio == false);
    assert(channels[0].encrypted == false);
    assert(channels[0].enabled == true);

    assert(channels[1].id == "C-61441-10000-52876");
    assert(channels[1].number == 333);
    assert(channels[1].name == "Radio Hamburg");
    assert(channels[1].provider == "");
    assert(channels[1].group == "Rest");
    assert(channels[1].radio == true);
    assert(channels[1].encrypted == false);
    assert(channels[1].enabled == true);
}

static void test_channel_mapper_accepts_top_level_array()
{
    const std::string json =
        "[{\"name\":\"ZDF HD\","
        "\"number\":2,"
        "\"channel_id\":\"C-1-1079-28006\","
        "\"group\":\"ZDF\","
        "\"is_radio\":false}]";

    std::vector<VdrChannel> channels = RestfulApiChannelMapper::parseChannels(json);

    assert(channels.size() == 1);
    assert(channels[0].id == "C-1-1079-28006");
    assert(channels[0].number == 2);
    assert(channels[0].name == "ZDF HD");
    assert(channels[0].group == "ZDF");
    assert(channels[0].radio == false);
    assert(channels[0].encrypted == false);
}

static void test_channel_mapper_maps_restfulapi_encryption_flag()
{
    const std::string json =
        "{\"channels\":["
        "{\"name\":\"Sky Sport Bundesliga 2\","
        "\"number\":46,"
        "\"channel_id\":\"C-133-13-277\","
        "\"image\":false,"
        "\"group\":\"Rest\","
        "\"encrypted\":true,"
        "\"caids\":[\"09C7\",\"09EF\",\"1854\"],"
        "\"transponder\":530,"
        "\"stream\":\"C-133-13-277.ts\","
        "\"is_atsc\":false,"
        "\"is_cable\":true,"
        "\"is_terr\":false,"
        "\"is_sat\":false,"
        "\"is_radio\":false,"
        "\"index\":45"
        "}"
        "]}";

    std::vector<VdrChannel> channels = RestfulApiChannelMapper::parseChannels(json);

    assert(channels.size() == 1);
    assert(channels[0].id == "C-133-13-277");
    assert(channels[0].number == 46);
    assert(channels[0].name == "Sky Sport Bundesliga 2");
    assert(channels[0].group == "Rest");
    assert(channels[0].radio == false);
    assert(channels[0].encrypted == true);
    assert(channels[0].enabled == true);
}

static void test_channel_mapper_ignores_objects_without_channel_id()
{
    const std::string json =
        "{\"channels\":["
        "{\"name\":\"Broken\",\"number\":1,\"group\":\"Test\",\"is_radio\":false},"
        "{\"name\":\"Valid\",\"number\":2,\"channel_id\":\"C-1-2-3\",\"group\":\"Test\",\"is_radio\":false}"
        "]}";

    std::vector<VdrChannel> channels = RestfulApiChannelMapper::parseChannels(json);

    assert(channels.size() == 1);
    assert(channels[0].id == "C-1-2-3");
}

static void test_channel_mapper_tolerates_invalid_json()
{
    std::vector<VdrChannel> channels = RestfulApiChannelMapper::parseChannels("not json");

    assert(channels.empty() == true);
}

static void test_channel_mapper_tolerates_empty_channels_array()
{
    std::vector<VdrChannel> channels = RestfulApiChannelMapper::parseChannels("{\"channels\":[]}");

    assert(channels.empty() == true);
}

int main()
{
    test_channel_mapper_maps_valid_channels_response();
    test_channel_mapper_accepts_top_level_array();
    test_channel_mapper_maps_restfulapi_encryption_flag();
    test_channel_mapper_ignores_objects_without_channel_id();
    test_channel_mapper_tolerates_invalid_json();
    test_channel_mapper_tolerates_empty_channels_array();

    return 0;
}
