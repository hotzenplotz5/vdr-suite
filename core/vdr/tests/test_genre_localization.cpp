#include "GenreLocalization.h"

#include <cassert>
#include <iostream>

int main()
{
    GenreLocalization localization;

    assert(localization.label("crime", "de") == "Krimi");
    assert(localization.label("crime", "de_DE") == "Krimi");
    assert(localization.label("crime", "en") == "Crime");
    assert(localization.label("comedy", "de") == "Komödie");
    assert(localization.label("documentary", "de") == "Dokumentation");
    assert(localization.label("movie", "de") == "Spielfilm");
    assert(localization.label("series", "en_US") == "Series");

    assert(localization.label("crime", "fr") == "Crime");
    assert(localization.label("unknown-genre", "de") == "unknown-genre");

    std::cout << "test_genre_localization passed" << std::endl;
    return 0;
}
