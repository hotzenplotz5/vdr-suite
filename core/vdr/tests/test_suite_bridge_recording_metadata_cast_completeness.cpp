#include "SuiteBridgeRecordingMetadataResolver.h"

#include <algorithm>
#include <cassert>
#include <string>

namespace
{

std::string unavailableArtwork()
{
    return "{\"available\":false,\"provider\":\"none\",\"path\":\"\",\"width\":0,\"height\":0}";
}

std::string person(
    const std::string& name,
    const std::string& characterName)
{
    return "{\"role\":\"actor\",\"name\":\"" + name +
        "\",\"characterName\":\"" + characterName +
        "\",\"image\":" + unavailableArtwork() + "}";
}

std::string payload(const std::string& key)
{
    std::string people = "[";
    for (int index = 0; index < 52; ++index)
    {
        if (index > 0)
        {
            people += ',';
        }

        const std::string name = index == 40
            ? "John Travolta"
            : "Supporting Actor " + std::to_string(index);
        const std::string characterName = index == 40
            ? "Vincent Vega"
            : "Supporting Character " + std::to_string(index);
        people += person(name, characterName);
    }
    people += ']';

    return "{\"schema\":1,\"found\":true"
        ",\"reason\":\"none\""
        ",\"provider\":\"tvscraper\""
        ",\"recordingIdentitySchema\":1"
        ",\"recordingKey\":\"" + key + "\""
        ",\"mediaType\":\"movie\""
        ",\"providerId\":680"
        ",\"seasonNumber\":0"
        ",\"episodeNumber\":0"
        ",\"absoluteEpisodeNumber\":0"
        ",\"runtimeMinutes\":154"
        ",\"durationDeviationMinutes\":0"
        ",\"scraperHd\":1"
        ",\"scraperLanguage\":0"
        ",\"popularity\":100"
        ",\"voteAverage\":8.5"
        ",\"voteCount\":1000"
        ",\"adult\":false"
        ",\"collectionId\":0"
        ",\"lastSeason\":0"
        ",\"title\":\"Pulp Fiction\""
        ",\"originalTitle\":\"Pulp Fiction\""
        ",\"episodeName\":\"\""
        ",\"tagline\":\"\""
        ",\"overview\":\"Cast completeness regression payload\""
        ",\"releaseDate\":\"1994-10-14\""
        ",\"firstAired\":\"\""
        ",\"imdbId\":\"tt0110912\""
        ",\"status\":\"Released\""
        ",\"collectionName\":\"\""
        ",\"genres\":[\"Crime\",\"Drama\"]"
        ",\"productionCountries\":[\"US\"]"
        ",\"networks\":[]"
        ",\"preferredArtwork\":" + unavailableArtwork() +
        ",\"people\":" + people +
        ",\"images\":[]}";
}

}

int main()
{
    const std::string key = "c94d0eb9958a85079f81f059a436003c";
    const std::string json = payload(key);
    assert(json.size() > 8192);

    SuiteBridgeRecordingMetadataCommandReply reply;
    reply.transportSucceeded = true;
    reply.replyCode = 250;
    reply.payload = json;

    const VdrRecordingNativeMetadata metadata =
        SuiteBridgeRecordingMetadataResolver::parseReply(key, reply);

    assert(metadata.availability ==
        VdrRecordingNativeMetadataAvailability::Found);
    assert(metadata.people.size() == 52);

    const auto johnTravolta = std::find_if(
        metadata.people.begin(),
        metadata.people.end(),
        [](const VdrRecordingNativePerson& value)
        {
            return value.name == "John Travolta" &&
                value.characterName == "Vincent Vega";
        });
    assert(johnTravolta != metadata.people.end());

    return 0;
}
