#include "Database.h"
#include "VdrChannelCacheRepository.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace
{
VdrChannel channel(
    const std::string& id,
    int number,
    const std::string& name)
{
    VdrChannel value;
    value.id = id;
    value.number = number;
    value.name = name;
    value.provider = "provider";
    value.group = "group";
    value.enabled = true;
    return value;
}
}

int main()
{
    const std::string filename =
        "/tmp/vdr-suite-channel-cache-repository-test.sqlite";
    std::remove(filename.c_str());

    {
        Database database;
        assert(database.open(filename));

        VdrChannelCacheRepository repository(database);
        assert(repository.ensureSchema());
        assert(repository.replaceChannelsForBackend(
            "default",
            {channel("C-1", 1, "Das Erste"),
             channel("C-2", 2, "ZDF")}));
        assert(repository.replaceChannelsForBackend(
            "remote",
            {channel("R-1", 1, "Remote One")}));

        assert(repository.countForBackend("default") == 2);
        assert(repository.countForBackend("remote") == 1);

        const std::vector<VdrChannel> initial =
            repository.findAllForBackend("default");
        assert(initial.size() == 2);
        assert(initial.front().id == "C-1");
        assert(initial.front().name == "Das Erste");

        assert(repository.replaceChannelsForBackend(
            "default",
            {channel("C-2", 7, "ZDF HD")}));
        assert(repository.countForBackend("default") == 1);
        assert(repository.countForBackend("remote") == 1);

        const std::vector<VdrChannel> replaced =
            repository.findAllForBackend("default");
        assert(replaced.size() == 1);
        assert(replaced.front().id == "C-2");
        assert(replaced.front().number == 7);
        assert(replaced.front().name == "ZDF HD");
    }

    {
        Database database;
        assert(database.open(filename));
        VdrChannelCacheRepository repository(database);
        const std::vector<VdrChannel> persisted =
            repository.findAllForBackend("default");
        assert(persisted.size() == 1);
        assert(persisted.front().name == "ZDF HD");
        assert(repository.countForBackend("remote") == 1);
    }

    std::remove(filename.c_str());
    std::cout << "vdr channel cache repository ok\n";
    return 0;
}
