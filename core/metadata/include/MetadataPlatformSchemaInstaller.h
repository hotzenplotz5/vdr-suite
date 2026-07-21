#pragma once

class Database;

class MetadataPlatformSchemaInstaller
{
public:
    explicit MetadataPlatformSchemaInstaller(Database& database);

    bool ensureSchema() const;

private:
    Database& database_;
};
