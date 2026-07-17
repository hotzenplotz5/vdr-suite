#include "Database.h"

#include <sqlite3.h>

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace
{

std::string readFile(const std::string& path)
{
    std::ifstream input(path);
    assert(input.good());

    std::ostringstream content;
    content << input.rdbuf();
    return content.str();
}

int scalarInt(
    Database& database,
    const std::string& sql)
{
    sqlite3_stmt* stmt = nullptr;

    assert(sqlite3_prepare_v2(
               database.handle(),
               sql.c_str(),
               -1,
               &stmt,
               nullptr) == SQLITE_OK);

    int value = 0;
    assert(sqlite3_step(stmt) == SQLITE_ROW);
    value = sqlite3_column_int(stmt, 0);
    assert(sqlite3_finalize(stmt) == SQLITE_OK);
    return value;
}

void assertRejected(
    Database& database,
    const std::string& sql)
{
    assert(!database.execute(sql));
}

void test_schema_coexists_with_legacy_tables_and_is_idempotent()
{
    const std::string databasePath =
        "/tmp/vdr-suite-metadata-schema-contract.db";
    std::remove(databasePath.c_str());

    Database database;
    assert(database.open(databasePath));

    const std::string legacySchema =
        readFile("database/schema/vdr-suite.sql");
    const std::string metadataSchema =
        readFile("database/schema/metadata-platform-v1.sql");

    assert(database.execute(legacySchema));
    assert(database.execute(metadataSchema));
    assert(database.execute(metadataSchema));

    assert(database.tableExists("metadata"));
    assert(database.tableExists("artwork"));
    assert(database.tableExists("suite_metadata_schema_versions"));
    assert(database.tableExists("suite_metadata_entities"));
    assert(database.tableExists("suite_metadata_targets"));
    assert(database.tableExists("suite_metadata_providers"));
    assert(database.tableExists("suite_metadata_provider_scopes"));
    assert(database.tableExists("suite_metadata_evidence"));
    assert(database.tableExists("suite_metadata_assignments"));
    assert(database.tableExists("suite_metadata_assignment_evidence"));
    assert(database.tableExists("suite_metadata_entity_external_ids"));

    assert(scalarInt(
               database,
               "SELECT COUNT(*) "
               "FROM suite_metadata_schema_versions "
               "WHERE version = 1;") == 1);

    database.close();
    std::remove(databasePath.c_str());
}

void test_schema_enforces_identity_scope_evidence_and_assignment_rules()
{
    const std::string databasePath =
        "/tmp/vdr-suite-metadata-schema-rules.db";
    std::remove(databasePath.c_str());

    Database database;
    assert(database.open(databasePath));
    assert(database.execute(
        readFile("database/schema/metadata-platform-v1.sql")));

    const std::string entityOne =
        "mdent_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    const std::string entityTwo =
        "mdent_bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    const std::string target =
        "mdtgt_cccccccccccccccccccccccccccccccc";
    const std::string evidence =
        "mdevd_dddddddddddddddddddddddddddddddd";
    const std::string assignmentOne =
        "mdasg_eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
    const std::string assignmentTwo =
        "mdasg_ffffffffffffffffffffffffffffffff";
    const std::string disputedAssignment =
        "mdasg_11111111111111111111111111111111";

    assert(database.execute(
        "INSERT INTO suite_metadata_providers ("
        "provider_id, provider_kind, display_name"
        ") VALUES ("
        "'restfulapi-scraper-bridge', 'plugin', 'TVScraper bridge'"
        ");"));

    assert(database.execute(
        "INSERT INTO suite_metadata_provider_scopes ("
        "provider_id, scope_type, backend_id, priority"
        ") VALUES ("
        "'restfulapi-scraper-bridge', 'global', '', 10"
        ");"));

    assert(database.execute(
        "INSERT INTO suite_metadata_provider_scopes ("
        "provider_id, scope_type, backend_id, priority"
        ") VALUES ("
        "'restfulapi-scraper-bridge', 'backend', 'house-a', 20"
        ");"));

    assert(database.execute(
        "INSERT INTO suite_metadata_targets ("
        "metadata_target_id, target_type"
        ") VALUES ('" + target + "', 'recording');"));

    assert(database.execute(
        "INSERT INTO suite_metadata_entities ("
        "metadata_entity_id, media_type"
        ") VALUES ('" + entityOne + "', 'movie');"));
    assert(database.execute(
        "INSERT INTO suite_metadata_entities ("
        "metadata_entity_id, media_type"
        ") VALUES ('" + entityTwo + "', 'movie');"));

    assert(database.execute(
        "INSERT INTO suite_metadata_evidence ("
        "metadata_evidence_id, metadata_target_id, provider_id, "
        "backend_id, source_entity_type, source_external_id, "
        "observed_at, language, normalized_payload, "
        "payload_fingerprint, confidence"
        ") VALUES ("
        "'" + evidence + "', '" + target + "', "
        "'restfulapi-scraper-bridge', 'house-a', 'movie', '13', "
        "'2026-07-17T05:00:00Z', 'de', "
        "'{\"title\":\"Example Movie\"}', 'sha256:example', 0.92"
        ");"));

    assert(database.execute(
        "INSERT INTO suite_metadata_assignments ("
        "metadata_assignment_id, metadata_target_id, "
        "metadata_entity_id, assignment_state, confidence"
        ") VALUES ("
        "'" + assignmentOne + "', '" + target + "', "
        "'" + entityOne + "', 'selected', 0.92"
        ");"));

    assert(database.execute(
        "INSERT INTO suite_metadata_assignment_evidence ("
        "metadata_assignment_id, metadata_evidence_id, evidence_role"
        ") VALUES ("
        "'" + assignmentOne + "', '" + evidence + "', 'supporting'"
        ");"));

    assert(database.execute(
        "INSERT INTO suite_metadata_entity_external_ids ("
        "metadata_entity_id, provider_id, external_namespace, "
        "external_id, metadata_evidence_id"
        ") VALUES ("
        "'" + entityOne + "', 'restfulapi-scraper-bridge', "
        "'tmdb', '13', '" + evidence + "'"
        ");"));

    assert(scalarInt(
               database,
               "SELECT COUNT(*) FROM suite_metadata_entities;") == 2);
    assert(scalarInt(
               database,
               "SELECT COUNT(*) FROM suite_metadata_evidence;") == 1);
    assert(scalarInt(
               database,
               "SELECT COUNT(*) FROM suite_metadata_assignments;") == 1);
    assert(scalarInt(
               database,
               "SELECT COUNT(*) FROM pragma_foreign_key_check;") == 0);

    assertRejected(
        database,
        "INSERT INTO suite_metadata_entities ("
        "metadata_entity_id, media_type"
        ") VALUES ('13', 'movie');");

    assertRejected(
        database,
        "INSERT INTO suite_metadata_providers ("
        "provider_id, provider_kind"
        ") VALUES ('https://provider.invalid', 'external-catalog');");

    assertRejected(
        database,
        "INSERT INTO suite_metadata_provider_scopes ("
        "provider_id, scope_type, backend_id"
        ") VALUES ("
        "'restfulapi-scraper-bridge', 'global', 'house-a'"
        ");");

    assertRejected(
        database,
        "UPDATE suite_metadata_evidence "
        "SET confidence = 0.5 "
        "WHERE metadata_evidence_id = '" + evidence + "';");

    assertRejected(
        database,
        "DELETE FROM suite_metadata_evidence "
        "WHERE metadata_evidence_id = '" + evidence + "';");

    assertRejected(
        database,
        "INSERT INTO suite_metadata_assignments ("
        "metadata_assignment_id, metadata_target_id, "
        "metadata_entity_id, assignment_state, confidence"
        ") VALUES ("
        "'" + assignmentTwo + "', '" + target + "', "
        "'" + entityTwo + "', 'selected', 0.75"
        ");");

    assert(database.execute(
        "INSERT INTO suite_metadata_assignments ("
        "metadata_assignment_id, metadata_target_id, "
        "metadata_entity_id, assignment_state, confidence"
        ") VALUES ("
        "'" + disputedAssignment + "', '" + target + "', "
        "'" + entityTwo + "', 'disputed', 0.75"
        ");"));

    assertRejected(
        database,
        "INSERT INTO suite_metadata_evidence ("
        "metadata_evidence_id, metadata_target_id, provider_id, observed_at"
        ") VALUES ("
        "'mdevd_22222222222222222222222222222222', '" + target + "', "
        "'missing-provider', '2026-07-17T05:00:00Z'"
        ");");

    assertRejected(
        database,
        "DELETE FROM suite_metadata_providers "
        "WHERE provider_id = 'restfulapi-scraper-bridge';");

    assert(scalarInt(
               database,
               "SELECT COUNT(*) FROM suite_metadata_assignments;") == 2);
    assert(scalarInt(
               database,
               "SELECT COUNT(*) FROM pragma_foreign_key_check;") == 0);

    database.close();
    std::remove(databasePath.c_str());
}

}

int main()
{
    test_schema_coexists_with_legacy_tables_and_is_idempotent();
    test_schema_enforces_identity_scope_evidence_and_assignment_rules();

    std::cout << "test_metadata_schema_contract passed" << std::endl;
    return 0;
}
