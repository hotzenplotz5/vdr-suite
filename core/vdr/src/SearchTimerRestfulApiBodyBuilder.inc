void addString(std::ostringstream& body, const std::string& key, const std::string& value)
{
    body << "\"" << key << "\":\"" << jsonEscape(value) << "\",";
}

void addInt(std::ostringstream& body, const std::string& key, int value)
{
    body << "\"" << key << "\":" << value << ",";
}

void addBool(std::ostringstream& body, const std::string& key, bool value)
{
    body << "\"" << key << "\":" << (value ? "1" : "0") << ",";
}

void addStringArray(
    std::ostringstream& body,
    const std::string& key,
    const std::vector<std::string>& values)
{
    body << "\"" << key << "\":[";

    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index > 0)
        {
            body << ",";
        }

        body << "\"" << jsonEscape(values[index]) << "\"";
    }

    body << "],";
}

template <typename Request>
std::string buildSearchTimerBody(
    const Request& request,
    const std::string& backendNativeId)
{
    std::ostringstream body;
    body << "{";

    if (!backendNativeId.empty())
    {
        addInt(body, "id", std::atoi(backendNativeId.c_str()));
    }

    addString(body, "search", request.query);
    addBool(body, "use_title", request.compareTitle);
    addBool(body, "use_subtitle", request.compareSubtitle);
    addBool(body, "use_description", request.compareSummary);
    addInt(body, "use_channel", request.useChannel);
    addString(body, "channels", request.channels);
    addString(body, "channel_min", request.channelMin);
    addString(body, "channel_max", request.channelMax);
    addBool(body, "use_time", request.useTime);
    addInt(body, "start_time", request.startTime);
    addInt(body, "stop_time", request.stopTime);
    addBool(body, "use_duration", request.useDuration);
    addInt(body, "duration_min", request.durationMinMinutes);
    addInt(body, "duration_max", request.durationMaxMinutes);
    addBool(body, "use_dayofweek", request.useDayOfWeek);
    addInt(body, "dayofweek", request.dayOfWeek);
    addBool(body, "avoid_repeats", request.avoidRepeats);
    addInt(body, "allowed_repeats", request.allowedRepeats);
    addInt(body, "repeats_within_days", request.repeatsWithinDays);
    addBool(body, "compare_title", request.compareTitle);
    addBool(body, "compare_subtitle", request.compareSubtitle);
    addBool(body, "compare_summary", request.compareSummary);
    addBool(body, "compare_categories", request.compareCategories);
    addBool(body, "compare_time", request.compareTime);
    addBool(body, "use_series_recording", request.useSeriesRecording);
    addInt(body, "keep_recs", request.keepRecordings);
    addInt(body, "del_mode", request.deleteMode);
    addInt(body, "search_timer_action", request.searchTimerAction);
    addInt(body, "blacklist_mode", request.blacklistMode);
    addStringArray(
        body,
        "blacklist_ids",
        listValues(request.blacklistIdList, request.blacklistIds));
    addInt(body, "mode", request.matchMode);
    addBool(body, "match_case", request.matchCase);
    addInt(body, "tolerance", request.matchTolerance);
    addInt(body, "summary_match", request.summaryMatch);
    addBool(body, "use_ext_epg_info", request.useExtendedEpgInfo);
    addStringArray(
        body,
        "ext_epg_info",
        listValues(request.extendedEpgInfoList, request.extendedEpgInfo));
    addBool(body, "ignore_missing_epg_cats", request.ignoreMissingEpgCategories);
    addString(body, "content_descriptors", request.contentDescriptors);
    addBool(body, "use_in_favorites", request.useInFavorites);
    addString(body, "use_as_searchtimer_from", request.activeFrom);
    addString(body, "use_as_searchtimer_til", request.activeUntil);
    addBool(body, "pause_on_recs", request.pauseOnRecordings);
    addInt(body, "switch_min_before", request.switchMinutesBefore);
    addBool(body, "unmute_sound_on_switch", request.unmuteSoundOnSwitch);
    addInt(body, "del_recs_after_days", request.deleteRecordingsAfterDays);
    addInt(body, "del_after_count_recs", request.deleteAfterCountRecordings);
    addInt(body, "del_after_days_of_first_rec", request.deleteAfterDaysOfFirstRecording);
    addString(body, "directory", request.directory);
    addInt(body, "priority", request.priority);
    addInt(body, "lifetime", request.lifetime);
    addInt(body, "margin_start", request.marginStartMinutes);
    addInt(body, "margin_stop", request.marginStopMinutes);
    addBool(body, "use_vps", request.useVps);
    body << "\"use_as_searchtimer\":" << (request.active ? "1" : "0") << "}";

    return body.str();
}
