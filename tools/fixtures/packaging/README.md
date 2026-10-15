# Historical P7 schema fixture

- File: `v1.60.15m-vdr-suite.sql`
- Original repository: `hotzenplotz5/vdr-suite`
- Git tag: `v1.60.15m-recording-actions-cache-consistency`
- Annotated tag object: `50e699956dbd4407a57680b6359dca54f000ee74`
- Peeled commit: `646020746bc449b220ff4f24b80992e5f38f0162`
- Original path: `database/schema/vdr-suite.sql`
- Original Git blob SHA-1: `af0dd4265ca37ce8ef34f9ff1fbac18e079cf0ac`

This is a byte-for-byte historical schema fixture, **not** a database dump
from a user machine and **not** an earlier Debian/Ubuntu binary package.
`tools/test_packaging_p7_historical_schema.py` validates its Git object hash
before testing. Do not apply this file to a production SQLite database.

The separate `p7-pre-revision-security-operations.sql` is **synthetic**:
its two missing columns are modeled from the current C++ ALTER guards.
It is not part of the tagged historical Git SQL, contains no genuine
credentials, and is only for isolated CI SQLite migration testing.

## Historical C++ owner provenance for older column shapes

The synthetic `p7-pre-revision-security-operations.sql` fixture now has
an **independent historical-source gate**, not merely a manually reconstructed
shape. Its `security_human_accounts` and `mutation_operations` CREATE TABLE
statements (including constraints) are whitespace-normalized equivalents of
the original source-owned DDL in these two **byte-exact Git blob snapshots**:

- `historical/HumanAccountRepository-20260930.cpp`: commit
  `e63623d80be9b9d69f5219edd379a973968cd282`, original source
  `core/security/src/HumanAccountRepository.cpp`, Git blob
  `9ad863d9af74ba725436db782c0ae88e192d45ed` (before `revision`)
- `historical/MutationOperationRepository-20260811.cpp`: commit
  `6df73c326f32ae8ddd739caf1452b6ccddc16b1e`, original source
  `core/operations/src/MutationOperationRepository.cpp`, Git blob
  `ac4106441bce3fbe77cd26921c01805fccf73429`
  (before `expected_resource_fingerprint`)

`tools/test_packaging_p7_schema_provenance.py` checks Git blob hashes,
extracts each earlier C++ `ensureSchema()` table definition, verifies all
CREATE TABLE constraints against the synthetic fixture, and rejects
alterations. The fixture combines table shapes originating at **two different
historic commits** with fake records; it is still **not** a byte-exact
snapshot of a single old database, old package, or previously released
binary. No actual password hashes or user state are included.

## Additional P7 device-pairing fixture (synthetic)

`p7-pre-revision-device-pairing.sql` exercises the real
`DevicePairingRequestRepository::ensureSchema()` ALTER guards for the
`revision`, `decided_by_actor_id` and `decided_at` columns. This is a
**reconstructed test-only schema**, not provenance-attested historical DDL or
a database produced by a released binary. All values are fake. The same C++
rehearsal verifies persisted pairing records and VDR channel-cache records
across a fresh process and confirms that the offline backup restores the
original pairing table shape without those columns.
