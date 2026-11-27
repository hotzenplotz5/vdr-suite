#include "ImageGalleryCatalog.h"

#include <algorithm>
#include <filesystem>
#include <string_view>

namespace vdrsuite::imagegallery {
namespace {
char lower(char c)
{
    return c >= 'A' && c <= 'Z' ? static_cast<char>(c + ('a' - 'A')) : c;
}
bool digit(char c) { return c >= '0' && c <= '9'; }

// Compare numeric runs without integer conversion or overflow. For equal
// numeric values, fewer leading zeroes sort first. Preserve original names.
bool naturalLess(std::string_view a, std::string_view b)
{
    std::size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (digit(a[i]) && digit(b[j])) {
            const auto startA = i, startB = j;
            while (i < a.size() && digit(a[i])) ++i;
            while (j < b.size() && digit(b[j])) ++j;
            auto significantA = startA, significantB = startB;
            while (significantA < i && a[significantA] == '0') ++significantA;
            while (significantB < j && b[significantB] == '0') ++significantB;
            const auto lenA = i - significantA, lenB = j - significantB;
            if (lenA != lenB) return lenA < lenB;
            const auto cmp = a.substr(significantA, lenA).compare(b.substr(significantB, lenB));
            if (cmp != 0) return cmp < 0;
            if (i - startA != j - startB) return i - startA < j - startB;
            continue;
        }
        const auto ca = static_cast<unsigned char>(lower(a[i++]));
        const auto cb = static_cast<unsigned char>(lower(b[j++]));
        if (ca != cb) return ca < cb;
    }
    if (i != a.size() || j != b.size()) return i == a.size();
    return a < b;
}
} // namespace

Source yavdrDefaultSource()
{
    return {"pictures", "Pictures", "/srv/picture"};
}

bool isPhotoFilename(const std::string& name)
{
    auto extension = std::filesystem::path(name).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), lower);
    // Initial web format candidates; byte validation/decoder support is
    // the image provider's responsibility, not inferred from the suffix.
    return extension == ".jpg" || extension == ".jpeg" ||
           extension == ".png" || extension == ".bmp" || extension == ".webp";
}

std::vector<Entry> galleryEntries(std::vector<Entry> entries)
{
    entries.erase(std::remove_if(entries.begin(), entries.end(), [](const Entry& e) {
        return !e.directory && !isPhotoFilename(e.name);
    }), entries.end());
    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
        if (a.directory != b.directory) return a.directory;
        if (a.name == b.name) return a.relativePath < b.relativePath;
        return naturalLess(a.name, b.name);
    });
    return entries;
}

} // namespace vdrsuite::imagegallery
