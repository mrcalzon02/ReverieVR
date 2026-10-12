# ReverieVR public distribution and license boundaries

**Status:** in-use policy, 2026-10-11. **Repository:** public `mrcalzon02/ReverieVR`. Treat public commits, Git clones, source archives, and published APKs as real redistribution. These are separate obligations: publishing our source files is not proof that the source, licenses and build instructions corresponding to each distributed third-party executable have been supplied.

## Governing preference

The default source tree and the ordinary APK should use freely redistributable software and artwork where possible. Prefer self-authored, public-domain, permissive-license or free/open-source replacements. Keep restrictive artwork optional, never a hidden hard dependency for hardware input, and never automatically fetch it to evade its license. Users must have the practical freedom to omit it.

**No root ReverieVR license has been granted or selected yet.** Public GitHub visibility is not an open-source license for original code. Select an explicit software license only after the owner approves it and the GPL composition/linking boundaries have been reviewed. Do not apply one blanket Creative Commons license to software, artwork and independently licensed third-party assets.

## Actual included/potentially included components

| Component | Current distribution treatment | Licensing obligations / unresolved issues |
|---|---|---|
| Original ReverieVR code and authored procedural 3D | Public source, no declared repository-level license | Owner must approve source license; don't imply proprietary third-party rights are sublicensed |
| Google Cardboard v1.35.0 | Fixed Git submodule and built SDK | Primarily Apache-2.0; preserve notices and review upstream components separately |
| DOSBox Pure 1.0-preview6 | Pinned source fetched at build; `libretro.so` packaged by full builds | GPL-2.0-or-later: complete corresponding source, notices and reproducible rebuild material for distributed binary; examine whether combined host/application forms a single GPL-covered work |
| OpenKTG | Small vendored procedural texture library | Public-domain statement recorded in `third_party/openktg/LICENSE` |
| Daydream controller OBJ | Currently tracked and packaged | Upstream model described as CC Attribution (version needs verification), derivative repository includes GPL-3.0 text: retain attribution/notice, inspect combination and upgrade alternatives |
| Valve Steam Controller CAD and reduced mesh | **Not in the default current source tree or APK after the policy change**. A low-poly procedural controller fallback remains functional. | Valve original CAD derivatives recorded as CC BY-NC-SA 4.0. Preserve provenance separately. A public, noncommercial-only archive is not a generally open-source/free-culture dependency. Ask permission for commercial use or use independent artwork |
| Original DOOM Shareware v1.9 | **Currently bundled by free build when fetched**; source archive not tracked | Copyrighted shareware, not open source. Original-archive provenance and noncommercial distribution controls are in `third_party/doom-shareware/README.md`. Needs a separate decision on making it optional without breaking the published sample-game path |
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
