#include "SeriesArtworkBackendSettingsService.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <fcntl.h>
#include <filesystem>
#include <limits>
#include <sqlite3.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace
{
bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    return sqlite3_bind_text(
        statement,
        index,
        value.c_str(),
        -1,
        SQLITE_TRANSIENT) == SQLITE_OK;
}

std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value == nullptr
        ? std::string()
        : reinterpret_cast<const char*>(value);
}

bool tokenSyntaxValid(const std::string& token)
{
    return !token.empty() && token.size() <= 4096U &&
        std::all_of(
            token.begin(),
            token.end(),
            [](unsigned char character)
            {
                return character > 0x20U && character != 0x7fU;
            });
}

int openDirectoryNoFollow(const std::filesystem::path& path)
{
    const std::filesystem::path normalized = path.lexically_normal();
    if (!normalized.is_absolute() || normalized == normalized.root_path())
    {
        return -1;
    }

    int current = ::open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (current < 0)
    {
        return -1;
    }

    for (const auto& component : normalized.relative_path())
    {
        const std::string name = component.string();
        if (name.empty() || name == "." || name == "..")
        {
            ::close(current);
            return -1;
        }

        const int next = ::openat(
            current,
            name.c_str(),
            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        ::close(current);
        if (next < 0)
        {
            return -1;
        }
        current = next;
    }

    struct stat metadata{};
    if (::fstat(current, &metadata) != 0 ||
        !S_ISDIR(metadata.st_mode) ||
        (metadata.st_mode & 0077) != 0)
    {
        ::close(current);
        return -1;
    }

    return current;
}

bool writeAll(int descriptor, const std::string& value)
{
    std::size_t offset = 0;
    while (offset < value.size())
    {
        const ssize_t written = ::write(
            descriptor,
            value.data() + offset,
            value.size() - offset);
        if (written < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return false;
        }
        if (written == 0)
        {
            return false;
        }
        offset += static_cast<std::size_t>(written);
    }
    return true;
}

std::string tokenFilename(const std::string& backendId)
{
    return backendId + ".tmdb-token";
}
}

SeriesArtworkBackendSettingsService::SeriesArtworkBackendSettingsService(
    Database& database,
    IExternalArtworkHttpTransport& transport,
    ISeriesArtworkProviderCache& cache,
    SeriesArtworkBackendSettingsConfig config)
    : database_(database),
      transport_(transport),
      cache_(cache),
      config_(std::move(config))
{
    if (!validProvider(config_.defaultProvider))
    {
        config_.defaultProvider = "none";
    }
}

bool SeriesArtworkBackendSettingsService::validBackendId(
    const std::string& backendId)
{
    return !backendId.empty() && backendId.size() <= 128U &&
        std::all_of(
            backendId.begin(),
            backendId.end(),
            [](unsigned char character)
            {
                return std::isalnum(character) ||
                    character == '-' || character == '_' || character == '.';
            });
}

bool SeriesArtworkBackendSettingsService::validProvider(
    const std::string& provider)
{
    return provider == "none" ||
        provider == "tvmaze" ||
        provider == "tmdb";
}

bool SeriesArtworkBackendSettingsService::ensureSchema()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return ensureSchemaLocked();
}

bool SeriesArtworkBackendSettingsService::ensureSchemaLocked() const
{
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS backend_series_artwork_settings ("
        "backend_id TEXT PRIMARY KEY,"
        "provider TEXT NOT NULL,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "CHECK(provider IN ('none','tvmaze','tmdb'))"
        ");");
}

bool SeriesArtworkBackendSettingsService::readManagedProviderLocked(
    const std::string& backendId,
    std::string& provider) const
{
    provider.clear();
    if (!ensureSchemaLocked())
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT provider FROM backend_series_artwork_settings "
        "WHERE backend_id=? LIMIT 1;";
    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool bound = bindText(statement, 1, backendId);
    const int step = bound ? sqlite3_step(statement) : SQLITE_ERROR;
    if (step == SQLITE_ROW)
    {
        provider = columnText(statement, 0);
    }
    sqlite3_finalize(statement);
    return step == SQLITE_ROW || step == SQLITE_DONE;
}

bool SeriesArtworkBackendSettingsService::storeManagedProviderLocked(
    const std::string& backendId,
    const std::string& provider) const
{
    if (!ensureSchemaLocked())
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO backend_series_artwork_settings "
        "(backend_id,provider,updated_at) VALUES (?,?,CURRENT_TIMESTAMP) "
        "ON CONFLICT(backend_id) DO UPDATE SET "
        "provider=excluded.provider,updated_at=CURRENT_TIMESTAMP;";
    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool stored =
        bindText(statement, 1, backendId) &&
        bindText(statement, 2, provider) &&
        sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return stored;
}

std::string SeriesArtworkBackendSettingsService::readManagedTokenLocked(
    const std::string& backendId) const
{
    const int directory = openDirectoryNoFollow(config_.secretRoot);
    if (directory < 0)
    {
        return {};
    }

    const std::string filename = tokenFilename(backendId);
    const int descriptor = ::openat(
        directory,
        filename.c_str(),
        O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    ::close(directory);
    if (descriptor < 0)
    {
        return {};
    }

    struct stat metadata{};
    if (::fstat(descriptor, &metadata) != 0 ||
        !S_ISREG(metadata.st_mode) ||
        (metadata.st_mode & 0077) != 0 ||
        metadata.st_size <= 0 || metadata.st_size > 4096)
    {
        ::close(descriptor);
        return {};
    }

    std::string token(static_cast<std::size_t>(metadata.st_size), '\0');
    std::size_t offset = 0;
    while (offset < token.size())
    {
        const ssize_t count = ::read(
            descriptor,
            &token[offset],
            token.size() - offset);
        if (count < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            token.clear();
            break;
        }
        if (count == 0)
        {
            token.clear();
            break;
        }
        offset += static_cast<std::size_t>(count);
    }
    ::close(descriptor);

    return tokenSyntaxValid(token) ? token : std::string{};
}

bool SeriesArtworkBackendSettingsService::writeManagedTokenLocked(
    const std::string& backendId,
    const std::string& token) const
{
    if (!tokenSyntaxValid(token))
    {
        return false;
    }

    const int directory = openDirectoryNoFollow(config_.secretRoot);
    if (directory < 0)
    {
        return false;
    }

    const std::string filename = tokenFilename(backendId);
    const std::string temporary =
        "." + filename + "." + std::to_string(::getpid()) + ".tmp";
    const int descriptor = ::openat(
        directory,
        temporary.c_str(),
        O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW,
        S_IRUSR | S_IWUSR);
    if (descriptor < 0)
    {
        ::close(directory);
        return false;
    }

    bool written =
        ::fchmod(descriptor, S_IRUSR | S_IWUSR) == 0 &&
        writeAll(descriptor, token) &&
        ::fsync(descriptor) == 0;
    if (::close(descriptor) != 0)
    {
        written = false;
    }

    if (written)
    {
        written = ::renameat(
            directory,
            temporary.c_str(),
            directory,
            filename.c_str()) == 0;
    }
    if (written)
    {
        written = ::fsync(directory) == 0;
    }
    if (!written)
    {
        ::unlinkat(directory, temporary.c_str(), 0);
    }
    ::close(directory);
    return written;
}

bool SeriesArtworkBackendSettingsService::removeManagedTokenLocked(
    const std::string& backendId) const
{
    const int directory = openDirectoryNoFollow(config_.secretRoot);
    if (directory < 0)
    {
        return false;
    }

    const std::string filename = tokenFilename(backendId);
    const int removed = ::unlinkat(directory, filename.c_str(), 0);
    const bool success = removed == 0 || errno == ENOENT;
    const bool synced = !success || ::fsync(directory) == 0;
    ::close(directory);
    return success && synced;
}

SeriesArtworkBackendSettingsSnapshot
SeriesArtworkBackendSettingsService::snapshotLocked(
    const std::string& backendId,
    std::string* effectiveToken) const
{
    SeriesArtworkBackendSettingsSnapshot snapshot;
    snapshot.backendId = backendId;

    std::string managedProvider;
    const bool providerRead =
        readManagedProviderLocked(backendId, managedProvider);
    if (providerRead && !managedProvider.empty() && validProvider(managedProvider))
    {
        snapshot.provider = managedProvider;
        snapshot.configurationSource = "managed";
    }
    else
    {
        snapshot.provider = config_.defaultProvider;
        snapshot.configurationSource = "environment";
    }

    const std::string managedToken = readManagedTokenLocked(backendId);
    if (!managedToken.empty())
    {
        snapshot.tmdbTokenConfigured = true;
        snapshot.tmdbTokenSource = "managed";
        if (effectiveToken != nullptr)
        {
            *effectiveToken = managedToken;
        }
    }
    else if (snapshot.configurationSource == "environment" &&
             tokenSyntaxValid(config_.environmentTmdbReadAccessToken))
    {
        snapshot.tmdbTokenConfigured = true;
        snapshot.tmdbTokenSource = "environment";
        if (effectiveToken != nullptr)
        {
            *effectiveToken = config_.environmentTmdbReadAccessToken;
        }
    }
    else if (effectiveToken != nullptr)
    {
        effectiveToken->clear();
    }

    return snapshot;
}

SeriesArtworkBackendSettingsSnapshot
SeriesArtworkBackendSettingsService::get(
    const std::string& backendId) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!validBackendId(backendId))
    {
        return {};
    }
    return snapshotLocked(backendId);
}

SeriesArtworkBackendSettingsService::TokenValidation
SeriesArtworkBackendSettingsService::validateTmdbToken(
    const std::string& token) const
{
    if (!tokenSyntaxValid(token))
    {
        return TokenValidation::Invalid;
    }

    ExternalArtworkHttpRequest request;
    request.url = "https://api.themoviedb.org/3/configuration";
    request.bearerToken = token;
    request.accept = "application/json";
    request.connectTimeoutMs = config_.tmdb.connectTimeoutMs;
    request.totalTimeoutMs = config_.tmdb.totalTimeoutMs;
    request.maximumResponseBytes = config_.tmdb.maximumJsonBytes;

    const ExternalArtworkHttpResponse response = transport_.perform(request);
    if (response.transportError || response.statusCode == 429L ||
        response.statusCode >= 500L)
    {
        return TokenValidation::Unavailable;
    }
    if (response.statusCode != 200L ||
        response.contentType != "application/json" ||
        response.body.empty() || response.body.front() != '{')
    {
        return TokenValidation::Invalid;
    }
    return TokenValidation::Valid;
}

SeriesArtworkBackendSettingsUpdateResult
SeriesArtworkBackendSettingsService::update(
    const SeriesArtworkBackendSettingsUpdate& request)
{
    SeriesArtworkBackendSettingsUpdateResult result;
    if (!validBackendId(request.backendId))
    {
        result.statusCode = 400;
        result.errorCode = "invalid_backend_id";
        result.message = "The backend ID is invalid";
        return result;
    }
    if (!validProvider(request.provider))
    {
        result.statusCode = 400;
        result.errorCode = "invalid_artwork_provider";
        result.message = "The artwork provider must be none, tvmaze or tmdb";
        return result;
    }
    if (request.clearTmdbReadAccessToken &&
        !request.tmdbReadAccessToken.empty())
    {
        result.statusCode = 400;
        result.errorCode = "conflicting_token_update";
        result.message = "A token cannot be replaced and removed together";
        return result;
    }

    if (!request.tmdbReadAccessToken.empty())
    {
        const TokenValidation validation =
            validateTmdbToken(request.tmdbReadAccessToken);
        if (validation == TokenValidation::Unavailable)
        {
            result.statusCode = 503;
            result.errorCode = "tmdb_validation_unavailable";
            result.message = "TMDB token validation is temporarily unavailable";
            return result;
        }
        if (validation == TokenValidation::Invalid)
        {
            result.statusCode = 400;
            result.errorCode = "invalid_tmdb_read_access_token";
            result.message = "TMDB rejected the API Read Access Token";
            return result;
        }
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (!ensureSchemaLocked())
    {
        result.statusCode = 503;
        result.errorCode = "settings_persistence_unavailable";
        result.message = "Backend settings persistence is unavailable";
        return result;
    }

    std::string effectiveToken;
    SeriesArtworkBackendSettingsSnapshot current =
        snapshotLocked(request.backendId, &effectiveToken);

    if (request.clearTmdbReadAccessToken)
    {
        if (request.provider == "tmdb")
        {
            result.statusCode = 400;
            result.errorCode = "tmdb_token_required";
            result.message = "TMDB requires an API Read Access Token";
            return result;
        }
        if (!removeManagedTokenLocked(request.backendId))
        {
            result.statusCode = 503;
            result.errorCode = "secret_persistence_unavailable";
            result.message = "The TMDB token could not be removed safely";
            return result;
        }
        effectiveToken.clear();
    }
    else if (!request.tmdbReadAccessToken.empty())
    {
        if (!writeManagedTokenLocked(
                request.backendId,
                request.tmdbReadAccessToken))
        {
            result.statusCode = 503;
            result.errorCode = "secret_persistence_unavailable";
            result.message = "The TMDB token could not be stored safely";
            return result;
        }
        effectiveToken = request.tmdbReadAccessToken;
    }
    else if (current.configurationSource == "environment" &&
             current.tmdbTokenSource == "environment" &&
             request.provider == "tmdb")
    {
        if (!writeManagedTokenLocked(request.backendId, effectiveToken))
        {
            result.statusCode = 503;
            result.errorCode = "secret_persistence_unavailable";
            result.message = "The existing TMDB token could not be adopted safely";
            return result;
        }
    }

    if (request.provider == "tmdb" && effectiveToken.empty())
    {
        result.statusCode = 400;
        result.errorCode = "tmdb_token_required";
        result.message = "TMDB requires an API Read Access Token";
        return result;
    }

    if (!storeManagedProviderLocked(request.backendId, request.provider))
    {
        result.statusCode = 503;
        result.errorCode = "settings_persistence_unavailable";
        result.message = "Backend settings could not be stored";
        return result;
    }

    result.success = true;
    result.statusCode = 200;
    result.settings = snapshotLocked(request.backendId);
    return result;
}

SeriesArtworkFallbackResolution
SeriesArtworkBackendSettingsService::resolve(
    const std::string& backendId,
    const VdrEvent& event,
    const EpgScraperMetadata& metadata)
{
    std::string token;
    SeriesArtworkBackendSettingsSnapshot settings;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!validBackendId(backendId))
        {
            return {};
        }
        settings = snapshotLocked(backendId, &token);
    }

    if (settings.provider == "tvmaze")
    {
        TvmazeSeriesArtworkProvider provider(
            transport_,
            cache_,
            config_.tvmaze);
        return provider.resolve(backendId, event, metadata);
    }

    if (settings.provider == "tmdb" && !token.empty())
    {
        TmdbSeriesArtworkProviderConfig providerConfig = config_.tmdb;
        providerConfig.readAccessToken = token;
        TmdbSeriesArtworkProvider provider(
            transport_,
            cache_,
            std::move(providerConfig));
        return provider.resolve(backendId, event, metadata);
    }

    return {};
}
