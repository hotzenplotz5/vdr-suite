#include "SearchTimerDiscovery.h"

#include <cassert>
#include <iostream>
#include <vector>

int main()
{
    SearchTimerDiscoveryExtendedEpgInfo info =
        SearchTimerDiscoveryExtendedEpgInfo::create(
            7,
            "Category",
            std::vector<std::string>{"Movie", "Series"},
            "7|Category|Movie,Series");

    assert(info.id() == 7);
    assert(info.name() == "Category");
    assert(info.values().size() == 2);
    assert(info.values()[0] == "Movie");
    assert(info.values()[1] == "Series");
    assert(info.config() == "7|Category|Movie,Series");
    assert(info.hasValues());
    assert(info.hasConfig());

    SearchTimerDiscoveryExtendedEpgInfo emptyInfo =
        SearchTimerDiscoveryExtendedEpgInfo::create(
            8,
            "Rating",
            std::vector<std::string>{},
            "");

    assert(emptyInfo.id() == 8);
    assert(emptyInfo.name() == "Rating");
    assert(!emptyInfo.hasValues());
    assert(!emptyInfo.hasConfig());

    SearchTimerDiscoveryChannelGroup group =
        SearchTimerDiscoveryChannelGroup::create("HD Channels");
    assert(group.name() == "HD Channels");
    assert(group.isValid());

    SearchTimerDiscoveryChannelGroup emptyGroup =
        SearchTimerDiscoveryChannelGroup::create("");
    assert(!emptyGroup.isValid());

    SearchTimerDiscoveryBlacklist blacklist =
        SearchTimerDiscoveryBlacklist::create(4, "Teleshopping");
    assert(blacklist.id() == 4);
    assert(blacklist.search() == "Teleshopping");
    assert(blacklist.isValid());

    SearchTimerDiscoveryBlacklist emptyBlacklist =
        SearchTimerDiscoveryBlacklist::create(5, "");
    assert(!emptyBlacklist.isValid());

    SearchTimerDiscoveryRecordingDirectory directory =
        SearchTimerDiscoveryRecordingDirectory::create("Doku/Natur");
    assert(directory.path() == "Doku/Natur");
    assert(directory.isValid());

    SearchTimerDiscoveryRecordingDirectory emptyDirectory =
        SearchTimerDiscoveryRecordingDirectory::create("");
    assert(!emptyDirectory.isValid());

    SearchTimerDiscoveryCatalog catalog;
    assert(catalog.empty());
    assert(catalog.backendId().empty());
    assert(catalog.extendedEpgInfoCount() == 0);
    assert(catalog.channelGroupCount() == 0);
    assert(catalog.blacklistCount() == 0);
    assert(catalog.recordingDirectoryCount() == 0);

    catalog.setBackendId("livingroom");
    catalog.addExtendedEpgInfo(info);
    catalog.addChannelGroup(group);
    catalog.addBlacklist(blacklist);
    catalog.addRecordingDirectory(directory);

    assert(!catalog.empty());
    assert(catalog.backendId() == "livingroom");
    assert(catalog.extendedEpgInfoCount() == 1);
    assert(catalog.channelGroupCount() == 1);
    assert(catalog.blacklistCount() == 1);
    assert(catalog.recordingDirectoryCount() == 1);

    assert(catalog.extendedEpgInfos()[0].name() == "Category");
    assert(catalog.channelGroups()[0].name() == "HD Channels");
    assert(catalog.blacklists()[0].search() == "Teleshopping");
    assert(catalog.recordingDirectories()[0].path() == "Doku/Natur");

    std::cout << "test_search_timer_discovery passed" << std::endl;
    return 0;
}
