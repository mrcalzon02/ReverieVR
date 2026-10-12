# Approved Android Windows SMB dependency — 2026-10-12

This is the normative, exact-version review record for the **optional, authenticated read-only SMB2/SMB3 importer inside the existing ReverieVR APK**. It is not a Windows application, a separately installed Android package, nor a remote emulator runtime.

## Version and identity

- Direct Maven Central coordinate: `com.hierynomus:smbj:0.15.0`, published August 22, 2026; source `https://github.com/hierynomus/smbj`; Apache-2.0.
- Resolved Gradle transitive runtime coordinates advertised in the 0.15.0 upstream Maven POM: `com.hierynomus:asn-one:0.6.0` (Apache-2.0), `net.engio:mbassador:1.3.2` (MIT), `org.slf4j:slf4j-api:2.0.18` (MIT), `org.bouncycastle:bcprov-jdk18on:1.85.2` (Bouncy Castle permissive license).
- These libraries are used only when a user initiates a share connection; no background SMB polling/discovery and no new Android component/service.
- Source/byte provenance: exact Maven coordinates pinned in `app/build.gradle` and `docs/dependencies/BASELINE.json`. Transitive SHA256 checksums and full lock metadata are a **known gap**, not an asserted verified build. Do not downgrade SMBJ to an older vulnerable BC dependency without documenting a security analysis.
- Android: minSdk 26; Java source/target 17 as currently in `app/build.gradle`; SMBJ is Java bytecode (no ABI-specific JNI library). Android 8–10 and S9 compatibility is a **device acceptance requirement**, not inferred from POM.
- Package size: 0.15.0 SMBJ base is on the order of hundreds of KiB, but the Bouncy Castle runtime is much larger; no claim of final dex / unshrunk APK delta until a verified signed build is measured. Time cost is on-demand NTLM/SMB negotiation and file IO off the VR renderer thread; 10-second socket/read-request timeout, one worker executor, 64 KiB file-copy buffer, 8 GiB file bound.
- Connectivity: OS-configured Wi-Fi/hotspot/router (TCP 445), SMB2/SMB3 negotiated by library. Connection asks for explicitly named host, share, account, and password. **SMB signing required**; never use SMB1 or unauthenticated guest fallback. The password is held in memory during active import only and not persisted.
- Reversibility: remove the `LaptopSmbClient` integration and its direct Gradle coordinate if the real APK regression/size/performance footprint fails. Preserve user-owned data and normal SAF/DOS import throughout. Replacing with another compatible, permissively licensed SMB2/3 client requires equivalent security and phone-only UX verification.

## Verification and release status

`scripts/verify-dependency-baseline.py` fail-closed allowlist now explicitly approves the direct dependency and checks the linked source-license record. The signed Android build and runtime connectivity are separate gates. A build failure, checksum drift or unreviewed transitive update must block the APK rather than disable/bypass the guard. Verify complete signed APK packaging, license notices in `app/src/main/assets/licenses/THIRD_PARTY_NOTICES.txt`, and S9 direct wired/Wi-Fi behavior.

Maven Central: https://central.sonatype.com/artifact/com.hierynomus/smbj/0.15.0
