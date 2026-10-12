# ReverieVR public distribution and license boundaries

**Status:** in-use policy, 2026-10-11. **Repository:** public `mrcalzon02/ReverieVR`. Treat public commits, Git clones, source archives, and published APKs as real redistribution. These are separate obligations: publishing our source files is not proof that the source, licenses and build instructions corresponding to each distributed third-party executable have been supplied.

## Governing preference

The default source tree and the ordinary APK should use freely redistributable software and artwork where possible. Prefer self-authored, public-domain, permissive-license or free/open-source replacements. Keep restrictive artwork optional, never a hidden hard dependency for hardware input, and never automatically fetch it to evade its license. Users must have the practical freedom to omit it.

**The owner has approved MIT for independently authored ReverieVR software.**
`LICENSE` now contains the MIT grant, with full text at `LICENSES/MIT.txt`.
Original creative work is eligible for **CC BY 4.0** after ownership/rights
are confirmed; **CC0** may be used only through an explicit dedication for a
specific work. Unverified existing images have **no new license grant**,
even if they appear in the public tree. The owner confirms commercial-use rights
for the bundled Suno `Starry Cereal` music. Its
`LicenseRef-Suno-Commercial-Use` is **not** a CC BY/CC0/MIT license: other
published songs may be included only after official Suno paid-plan downloads
and track-specific provenance checks. File-level metadata lives in
`REUSE.toml`, adjacent asset `.license` records where compatible with
Android packaging, and `docs/licensing/ASSET_REGISTRY.json`. This does not
override a third-party rightsholder's conditions or adjudicate the combined
GPL-covered distribution.

## Actual included/potentially included components

| Component | Current distribution treatment | Licensing obligations / unresolved issues |
|---|---|---|
| Original ReverieVR code and independently authored procedural geometry | Public source under MIT | Grant limited to original code and authored runtime geometry; package includes separately licensed components |
| Google Cardboard v1.35.0 | Fixed Git submodule and built SDK | Primarily Apache-2.0; preserve notices and review upstream components separately |
| DOSBox Pure 1.0-preview6 | Pinned source fetched at build; `libretro.so` packaged by full builds | GPL-2.0-or-later: complete corresponding source, notices and reproducible rebuild material for distributed binary; examine whether combined host/application forms a single GPL-covered work |
| OpenKTG | Small vendored procedural texture library | Public-domain statement recorded in `third_party/openktg/LICENSE` |
| Daydream controller OBJ/MTL | Currently tracked and packaged with adjacent SPDX sidecars | Original source identified as CC Attribution without precise version; derivative carries GPL-3.0 text. Rights/compatibility review remains explicit |
| Valve Steam Controller CAD and reduced mesh | **Not in the default current source tree or APK after the policy change**. A low-poly procedural controller fallback remains functional. | Valve original CAD derivatives recorded as CC BY-NC-SA 4.0. Preserve provenance separately. A public, noncommercial-only archive is not a generally open-source/free-culture dependency. Ask permission for commercial use or use independent artwork |
| Original DOOM Shareware v1.9 | Free phone-test build includes unchanged SHA-pinned archive when fetched; source ZIP not tracked | Copyrighted non-open shareware, not covered by owner MIT. Phone-test manifests label it noncommercial; a commercially open release must omit or obtain permission |
| Android SMBJ 0.15.0 Windows-share file importer | Read-only SMB2/SMB3 client directly in the single APK; Stage A import, no PC companion | Apache-2.0; transitive asn-one (Apache-2.0), Mbassador and SLF4J (MIT), Bouncy Castle permissive notice; exact coordinates and S9/build acceptance in `docs/dependencies/SMBJ.md` |
| Optional emulator-source catalog | Downloads on user request, no playable emulator core from those downloads | Each approved repository has its own license. No BIOS, ROM or commercial game media should be automatically redistributed |
| Android Gradle/SDK/NDK, CMake, Java and offline mesh tooling | Build/construction dependencies | Tool redistributability differs from compiled app. Record approved versions; do not publicly vendor a complete Android SDK or tools under blanket rights assumptions |

## CC asset boundaries

The Valve original controller workshop CAD record identifies **CC BY-NC-SA 4.0**. The license allows covered noncommercial sharing subject to attribution and other obligations, and ShareAlike governs distributed *adaptations* of the covered work, not automatically every independent line of the host app. Mere presence in an openly readable GitHub repository does **not** make it usable in a commercial/open-source distribution. Removing an asset from the latest tree does not erase its old commits, forks, third-party clones, or previously published APKs. Review historical public release contents separately; do not rewrite public `main` history destructively to conceal it.

The older Daydream visual derives from a CC-attributed upstream 3D model and a repo with GPL-3.0 licensing. This can remain in the default tree during a precise license review, but its current attribution is not a substitute for confirming the exact original CC license version and derivative-license interaction.

**Runtime download is not a licensing exception.** A project-hosted mirror, automated downloader, or in-app installer that supplies noncommercial or GPL materials is still distribution of those materials. A future optional *user-initiated local file import* could separate the independent program from third-party art; it must validate input, identify its source/license, preserve attribution and avoid claiming missing rights. Such an importer is not implemented yet.

## GPL distribution obligations

ReverieVR's public GitHub repository improves source accessibility; it is not by itself a guaranteed complete corresponding-source package for every published APK. In particular, DOSBox Pure source is fetched separately at an immutable commit. Release checks need to identify the exact built revision, provide all required unmodified/modified GPL-covered source and build scripts/notices, and check whether JNI/native-module linking or bundling triggers broader GPL terms. GPL access obligations attach to distribution, not only to commercial releases. Publishing binary and source on GitHub is compatible with these requirements only if source is actually complete and accessible on the applicable terms.

The LGPL/GPL/Apache/open art combination should be reviewed as distinct components rather than treating "open" as one universal license. Public GitHub does not establish that the owner's original code is GPL or CC BY-SA.

## Release guard

* The legacy `app/src/main/assets/steam_controller_2015/handoff.zip` is not shipped or tracked in the current tree.
* `app/build.gradle` fails with a clear diagnostic if a developer has copied that restricted archive back into the automatically packaged Android assets path.
* The renderer keeps a controller-specific **procedural/primitive 3D fallback**, so Steam HID control must continue functioning without Valve's CAD asset.
* No automatic runtime download of Valve CAD is approved.
* The existing complete Maven dependency graph, runner-image patch versions, notices in APK binary, GPL corresponding-source archive and legacy releases still require further validation.
* Releases claiming universal commercial reuse must exclude/relicense noncommercial and restricted material and pass a proper GPL/source review.

## Selected license and reproducible release record (2026-10-11)

* **MIT** grants broad reuse of original source code and self-authored
  procedural 3D. It does not relicense GPL, Apache, or imported artistic work.
* **CC BY 4.0** is the default for **verified first-party creative assets**;
  **CC0** requires explicit file-level dedication. Original source rights
  for the logo/icon images and Daydream visual remain unresolved. Owner
  confirms a paid commercial-use grant for bundled **Starry Cereal**, not
  CC BY/CC0 rights for its audio or automatic commercial rights for every
  published song at the same account.
* `REUSE.toml` supplies scoped SPDX copyright/license annotations and
  explicitly marks unverified Android resource binaries. Adjacent
  `.license` files document the Daydream model. Android `res/` does not
  admit arbitrary `.license` sidecars, so those are not committed there.
* `docs/licensing/ASSET_REGISTRY.json` remains a reference inventory,
  not a build or release gate.
* The phone-test release supplies DOSBox Pure corresponding source and
  third-party notices, alongside SHA-256 checksums for published artifacts.
* Corresponding source and notices do not resolve legal questions about
  DOSBox Pure's GPL linking scope. A review of the combined executable,
  complete corresponding source and historical APK releases is still required.
  Public GitHub visibility by itself does not complete those obligations.

Treat every GPL covered dependency and every asset license as a separately
verified responsibility. In particular, a noncommercial Valve CAD download is
not made freely licensed by placing it behind a runtime request.
