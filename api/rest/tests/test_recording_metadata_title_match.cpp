#include "RecordingMetadataTitleMatch.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace match = recordingMetadataTitleMatch;

int main()
{
    // International film-part names and written/roman numerals are generic
    // title evidence, not title-specific metadata database entries.
    assert(match::score("Solar Voyage Deel 2 The Red Horizon",
                        "Solar Voyage - Part Two: The Red Horizon") == 120);
    assert(match::score("Solar Voyage Teil 2 Der rote Horizont",
                        "Solar Voyage - Part II: Der rote Horizont") == 120);
    assert(match::score("Quiet Bay: Part III",
                        "Quiet Bay Deel 3") == 120);
    assert(match::score("Deep Space Volume 4",
                        "Deep Space Chapter IV") == 120);
    assert(match::score("7 Jahre in Tibet",
                        "Sieben Jahre in Tibet") == 120);

    // A different numbered instalment cannot win just because the series
    // title or the prefix is the same.
    assert(match::score("Solar Voyage Deel 2 The Red Horizon",
                        "Solar Voyage - Part One: The Beginning") == 0);
    assert(match::score("Solar Voyage Deel 2 The Red Horizon",
                        "Solar Voyage") == 0);
    assert(match::score("Quiet Bay Part 2", "Quiet Bay Part 3") == 0);

    // Provider originalTitle is evaluated independently from translated
    // display title: the title itself need not be translated by regex.
    assert(match::score("Die Insel im Nebel",
                        "Die Insel im Nebel") == 120);
    assert(match::score("Sherlock Holmes A Game of Shadows",
                        "Sherlock Holmes: A Game of Shadows") == 120);
    assert(match::score("Sherlock Holmes A Game of Shadows",
                        "Sherlock Holmes: Spiel im Schatten") == 0);

    // Non-franchise series: punctuation variations are general.
    assert(match::score("Star Wars-Andor", "Star Wars: Andor") == 120);
    assert(match::score("Andor", "Star Wars: Andor") == 105);
    assert(match::score("Andor", "Different Series") == 0);
    assert(match::score("Foundation", "Foundation") == 120);

    const auto part = match::searchQueries(
        "Solar Voyage Deel 2 The Red Horizon");
    assert(part.size() == 3);
    assert(part[0] == "Solar Voyage Deel 2 The Red Horizon");
    assert(part[1] == "Solar Voyage");
    assert(part[2] == "The Red Horizon");

    const auto franchise = match::searchQueries("Star Wars-Andor");
    assert(franchise.size() == 2);
    assert(franchise[0] == "Star Wars-Andor");
    assert(franchise[1] == "Andor");

    const auto exact = match::searchQueries("Foundation");
    assert(exact.size() == 1 && exact[0] == "Foundation");

    // No repeated query and no unbounded query expansion.
    assert(match::searchQueries("Film: Part Two: The End").size() <= 4);

    std::cout << "METADATA_TITLE_MATCH_GENERIC=PASS\n";
}
