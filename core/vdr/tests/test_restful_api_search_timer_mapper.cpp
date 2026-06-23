#include "RestfulApiSearchTimerMapper.h"

#include <cassert>
#include <iostream>
#include <vector>

int main()
{
    const std::string json =
        "{\"searchtimers\":["
        "{\"id\":1,\"search\":\"Terra X\",\"use_as_searchtimer\":1,"
        "\"directory\":\"Doku\",\"priority\":50,\"lifetime\":99,"
        "\"margin_start\":5,\"margin_stop\":10,\"use_vps\":1,"
        "\"use_channel\":1,\"use_dayofweek\":1,\"use_duration\":1,"
        "\"duration_min\":30,\"duration_max\":120,"
        "\"compare_title\":1,\"compare_subtitle\":1,\"compare_summary\":1,"
        "\"compare_categories\":1,\"compare_time\":1,"
        "\"avoid_repeats\":1,\"allowed_repeats\":3,\"repeats_within_days\":14,"
        "\"channels\":\"1,2,3\",\"channel_min\":1,\"channel_max\":99,"
        "\"use_series_recording\":1,\"keep_recs\":10,"
        "\"del_mode\":2,\"search_timer_action\":1,"
        "\"blacklist_mode\":2,\"blacklist_ids\":\"4,5\","
        "\"mode\":5,\"match_case\":1,\"tolerance\":2,\"summary_match\":3,"
        "\"use_ext_epg_info\":1,\"ext_epg_info\":\"category=movie\","
        "\"ignore_missing_epg_cats\":1,\"content_descriptors\":\"16,32\","
        "\"use_in_favorites\":1,\"use_as_searchtimer_from\":\"2026-06-01\","
        "\"use_as_searchtimer_til\":\"2026-12-31\","
        "\"pause_on_recs\":1,\"switch_min_before\":5,\"unmute_sound_on_switch\":1,"
        "\"del_recs_after_days\":30,\"del_after_count_recs\":10,"
        "\"del_after_days_of_first_rec\":90},"
        "{\"id\":2,\"search\":\"Tatort\",\"use_as_searchtimer\":0}"
        "],\"count\":2,\"total\":2}";

    std::vector<SearchTimer> timers =
        RestfulApiSearchTimerMapper::parseSearchTimers(
            "livingroom",
            json);

    assert(timers.size() == 2);
    assert(timers.at(0).backendId() == "livingroom");
    assert(timers.at(0).backendNativeId() == "1");
    assert(timers.at(0).name() == "Terra X");
    assert(timers.at(0).query() == "Terra X");
    assert(timers.at(0).isActive());
    assert(timers.at(0).recordingOptions().directory() == "Doku");
    assert(timers.at(0).recordingOptions().priority() == 50);
    assert(timers.at(0).recordingOptions().lifetime() == 99);
    assert(timers.at(0).scheduleOptions().marginStartMinutes() == 5);
    assert(timers.at(0).scheduleOptions().marginStopMinutes() == 10);
    assert(timers.at(0).scheduleOptions().useVps());
    assert(timers.at(0).filterOptions().useChannel());
    assert(timers.at(0).filterOptions().useDayOfWeek());
    assert(timers.at(0).filterOptions().useDuration());
    assert(timers.at(0).filterOptions().durationMinMinutes() == 30);
    assert(timers.at(0).filterOptions().durationMaxMinutes() == 120);
    assert(timers.at(0).comparisonOptions().compareTitle());
    assert(timers.at(0).comparisonOptions().compareSubtitle());
    assert(timers.at(0).comparisonOptions().compareSummary());
    assert(timers.at(0).comparisonOptions().compareCategories());
    assert(timers.at(0).comparisonOptions().compareTime());
    assert(timers.at(0).repeatOptions().avoidRepeats());
    assert(timers.at(0).repeatOptions().allowedRepeats() == 3);
    assert(timers.at(0).repeatOptions().repeatsWithinDays() == 14);
    assert(timers.at(0).channelOptions().channels() == "1,2,3");
    assert(timers.at(0).channelOptions().channelMin() == 1);
    assert(timers.at(0).channelOptions().channelMax() == 99);
    assert(timers.at(0).seriesOptions().useSeriesRecording());
    assert(timers.at(0).seriesOptions().keepRecordings() == 10);
    assert(timers.at(0).seriesOptions().deleteMode() == 2);
    assert(timers.at(0).seriesOptions().searchTimerAction() == 1);
    assert(timers.at(0).blacklistOptions().blacklistMode() == 2);
    assert(timers.at(0).blacklistOptions().blacklistIds() == "4,5");
    assert(timers.at(0).matchOptions().mode() == 5);
    assert(timers.at(0).matchOptions().matchCase());
    assert(timers.at(0).matchOptions().tolerance() == 2);
    assert(timers.at(0).matchOptions().summaryMatch() == 3);
    assert(timers.at(0).extendedEpgOptions().useExtendedEpgInfo());
    assert(timers.at(0).extendedEpgOptions().extendedEpgInfo() == "category=movie");
    assert(timers.at(0).extendedEpgOptions().ignoreMissingEpgCategories());
    assert(timers.at(0).extendedEpgOptions().contentDescriptors() == "16,32");
    assert(timers.at(0).validityOptions().useInFavorites());
    assert(timers.at(0).validityOptions().activeFrom() == "2026-06-01");
    assert(timers.at(0).validityOptions().activeUntil() == "2026-12-31");
    assert(timers.at(0).actionOptions().pauseOnRecordings());
    assert(timers.at(0).actionOptions().switchMinutesBefore() == 5);
    assert(timers.at(0).actionOptions().unmuteSoundOnSwitch());
    assert(timers.at(0).actionOptions().deleteRecordingsAfterDays() == 30);
    assert(timers.at(0).actionOptions().deleteAfterCountRecordings() == 10);
    assert(timers.at(0).actionOptions().deleteAfterDaysOfFirstRecording() == 90);

    assert(timers.at(1).backendId() == "livingroom");
    assert(timers.at(1).backendNativeId() == "2");
    assert(timers.at(1).name() == "Tatort");
    assert(timers.at(1).query() == "Tatort");
    assert(timers.at(1).isInactive());

    const std::string escapedJson =
        "{\"searchtimers\":["
        "{\"id\":3,\"search\":\"Bob \\\"Marley\\\"\",\"use_as_searchtimer\":1}"
        "]}";

    std::vector<SearchTimer> escapedTimers =
        RestfulApiSearchTimerMapper::parseSearchTimers(
            "livingroom",
            escapedJson);

    assert(escapedTimers.size() == 1);
    assert(escapedTimers.at(0).name() == "Bob \"Marley\"");

    std::vector<SearchTimer> emptyTimers =
        RestfulApiSearchTimerMapper::parseSearchTimers(
            "livingroom",
            "{\"searchtimers\":[],\"count\":0,\"total\":0}");

    assert(emptyTimers.empty());

    std::vector<SearchTimer> invalidTimers =
        RestfulApiSearchTimerMapper::parseSearchTimers(
            "",
            json);

    assert(invalidTimers.empty());

    std::cout << "test_restful_api_search_timer_mapper passed" << std::endl;
    return 0;
}