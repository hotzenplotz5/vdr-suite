#include "CanonicalGenreRegistry.h"

#include <cassert>
#include <iostream>

int main()
{
    CanonicalGenreRegistry registry;

    assert(registry.classify("Crime").id == "crime");
    assert(registry.classify("Krimi").id == "crime");
    assert(registry.classify("Kriminalfilm").id == "crime");

    assert(registry.classify("Komödie").id == "comedy");
    assert(registry.classify("Komoedie").id == "comedy");
    assert(registry.classify("Comedy").id == "comedy");

    assert(registry.classify("Doku").id == "documentary");
    assert(registry.classify("Dokumentation").id == "documentary");

    assert(registry.classify("Spielfilm").id == "movie");
    assert(registry.classify("Kinderfilm").id == "children");

    assert(registry.classify("Science Fiction").id == "science-fiction");
    assert(registry.classify("science_fiction").id == "science-fiction");

    assert(registry.isKnown("crime"));
    assert(registry.isKnown("comedy"));
    assert(registry.isKnown("science-fiction"));
    assert(!registry.isKnown("unknown-space-opera"));

    std::cout << "test_canonical_genre_registry passed" << std::endl;
    return 0;
}
