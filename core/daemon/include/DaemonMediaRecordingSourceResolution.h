#pragma once

#include "VdrRecording.h"

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>

// Local-only path projection for Media Check and HEVC Shrink. The public
// rec_ identity must already have been resolved through the canonical
// PublicRecordingPlaybackTargetResolver (identity + current Recording cache).
// The provider's backendNativeId is the full native .rec identity; its
// presentation path can be only a display-relative name.
namespace vdrsuite::mediatools {
inline std::optional<std::filesystem::path> resolveConfiguredRecordingPath(
    const std::string& configuredRoot, const VdrRecording& recording)
{
    namespace fs = std::filesystem;
    const fs::path root(configuredRoot);
    const fs::path native(recording.backendNativeId);
    if (!root.is_absolute() || root == root.root_path() ||
        root.lexically_normal() != root ||
        !native.is_absolute() || native.lexically_normal() != native ||
        native.extension() != ".rec")
        return std::nullopt;

    // The admin-configured root must itself contain no symlink components,
    // just as the existing Media Check/Shrink engines require.
    std::error_code error;
    const fs::path canonicalRoot = fs::canonical(root, error);
    if (error || canonicalRoot != root) return std::nullopt;

    // Native VDR identifiers may use an alias/symlink *to* the configured
    // video root (e.g. a provider's legacy VDR mountpoint). Identify that
    // prefix by its filesystem identity, never by a distribution path string.
    // Symlinks BELOW that trusted root are not admitted.
    bool reachedRoot = false;
    fs::path prefix;
    for (const auto& part : native) {
        prefix /= part;
        if (!reachedRoot) {
            const fs::path resolved = fs::canonical(prefix, error);
            if (error) return std::nullopt;
            reachedRoot = resolved == root;
        } else {
            if (fs::is_symlink(fs::symlink_status(prefix, error)) || error)
                return std::nullopt;
        }
    }
    if (!reachedRoot) return std::nullopt;

    const fs::path absolute = fs::canonical(native, error);
    if (error) return std::nullopt;
    const fs::path relative = absolute.lexically_relative(root);
    if (relative.empty() || relative.is_absolute() ||
        relative.extension() != ".rec" ||
        std::any_of(relative.begin(), relative.end(),
            [](const fs::path& part) { return part == "." || part == ".."; }))
        return std::nullopt;

    // Both worker paths independently enforce their root/symlink confinement
    // at use time. Never expose this path in a Public API response.
    return absolute;
}
} // namespace vdrsuite::mediatools
