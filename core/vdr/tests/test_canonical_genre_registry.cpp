#include "CanonicalGenreRegistry.h"

#include <cassert>
#include <iostream>

int main()
{
    CanonicalGenreRegistry registry;

    assert(registry.canonicalize("Crime") == "crime");
    assert(registry.canonicalize("Krimi") == "crime");
    assert(registry.canonicalize("Kriminalfilm") == "crime");

    assert(registry.canonicalize("Komödie") == "comedy");
    assert(registry.canonicalize("Komoedie") == "comedy");
    assert(registry.canonicalize("Comedy") == "comedy");

    assert(registry.canonicalize("Doku") == "documentary");
    assert(registry.canonicalize("Dokumentation") == "documentary");

    assert(registry.canonicalize("Spielfilm") == "movie");
    assert(registry.canonicalize("Kinderfilm") == "children");

    assert(registry.canonicalize("Science Fiction") == "science-fiction");
    assert(registry.canonicalize("science_fiction") == "science-fiction");

    assert(registry.hasCanonicalGenre("crime"));
    assert(registry.hasCanonicalGenre("comedy"));
    assert(!registry.hasCanonicalGenre("science-fiction"));

    std::cout << "test_canonical_genre_registry passed" << std::endl;
    return 0;
}
