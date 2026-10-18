# Signed game-update catalog — testing bootstrap

This file describes the **testing** trust root, not a completed independent native game release.

The catalog is an intentionally empty, authenticated revision 1: `modules/catalog-v1.json`. Its detached `modules/catalog-v1.sig` is ECDSA P-256 / SHA-256, verified against the bundled **public** key `app/src/main/assets/native/modules/catalog-key.der`. The matching **private** signing key was generated outside this public repository and MUST be retained by the authorized project operator. Do not commit/upload it, its backup, or any passphrase to GitHub.

No `module-*` release assets are advertised. Once a real model/content pack or game APK has been built, uploaded, read back and hashed, increment `catalog_revision`, replace the source manifest with the verified immutable release URL/digest/size, sign exact JSON bytes using the offline private key, verify, then commit catalog and detached signature together.

This bootstrap key is **testing-only**. Before stable/public game releases, provision a new long-lived operator-controlled production publisher key and ship its public key in a newer hub APK. Coordinate a documented trust rotation; do not silently use the testing key indefinitely for production. The private key must remain available to the owner for testing-channel updates. The test-only public key is intentionally safe to store in the APK.

**Verified:** OpenSSL locally verified the detached signature over the exact catalog bytes; the catalog has zero module entries, avoiding false update-available claims. **Not verified:** a fresh installed Android APK, Samsung Galaxy S9 TLS/catalog fetch, user-facing update flow, and standalone companion APK.
