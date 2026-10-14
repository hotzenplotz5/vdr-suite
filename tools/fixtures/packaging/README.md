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
