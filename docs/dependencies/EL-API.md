# javax.el Expression Language 3.0 API — Play-only dependency admission

- **Exact coordinate:** `javax.el:javax.el-api:3.0.1-b06`
- **Origin:** [Maven Central](https://central.sonatype.com/artifact/javax.el/javax.el-api/3.0.1-b06), Oracle/GlassFish.
- **Why required:** The existing `com.hierynomus:smbj:0.15.0` dependency pulls MBassador, whose `StandardELResolutionContext` references `javax.el.BeanELResolver`. Minified Play builds fail R8 missing-class verification without a real defining library. This is a dependency-resolution repair, not a `-dontwarn` waiver.
- **Scope:** Direct, exact-version application runtime dependency on the Google Play branch, for correctness of the R8 classpath. No network access, Android permission, emulator ABI, native module or user-facing capability is introduced by this Java API alone.
- **Declared upstream license:** CDDL plus GPLv2 with classpath exception; see Maven Central POM. Record and include required legal texts/notices before commercial distribution. Neither the application's MIT license nor this note relicenses the dependency.
- **License requirement:** Verify the exact resolved POM/JAR notices, document the option used and satisfy notice/source obligations as applicable. Until confirmed, keep the Play Console / intellectual-property release gate pending.
- **Reproduction:** `./gradlew --no-daemon :app:assemblePlay :app:bundlePlay`. Do not disable minification or R8 missing-class checks.
- **Maintenance and size:** Historical stable 2018 API (~79 KB published JAR); low expected runtime overhead after shrinking, no runtime daemon. Review transitive dependency and availability on any SMBJ upgrade.
- **Removal plan:** Remove this pin only after the SMBJ/MBassador dependency graph no longer references `javax.el`, and a complete R8 Play artifact build proves that without suppressions.
