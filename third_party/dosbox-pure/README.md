# DOSBox Pure dependency

ReverieVR's primary embedded DOS runtime target is **DOSBox Pure**.

## Pinned baseline

- Upstream project: `schellingb/dosbox-pure`
- Release tag used for the first integration baseline: `1.0-preview6`
- Pinned commit: `a4a0bab7f8931433588f2fcad9045c85b277373d`
- License: GNU General Public License, version 2 or later
- Local source directory after fetch: `third_party/dosbox-pure/src`

The historical GitHub repository states that the project moved to Codeberg. The
first ReverieVR integration pin is nevertheless expressed as an immutable commit
that is present in the GitHub mirror and can be verified before build use.

Do not replace the pin with a floating `main` checkout.

## Why DOSBox Pure

The pinned core already:

- targets the libretro frontend API;
- publishes Android NDK build files;
- supports `armeabi-v7a` and `arm64-v8a`;
- has ARM64 dynamic-recompiler support in its Android build;
- accepts DOS content including ZIP/DOSZ, EXE, COM, BAT, ISO, CHD, CUE,
  IMG/IMA/VHD/JRC, M3U/M3U8, CONF and directories;
- exposes keyboard, mouse, pointer, joypad and analog input through libretro.

That is a substantially better fit than launching a separate Android DOSBox
application and attempting cross-application input injection.

## GPL distribution boundary

The project owner has explicitly accepted GPLv2/GPLv2+ compatibility for this
runtime integration.

When ReverieVR distributes a DOSBox Pure binary, the release process must also:

1. preserve DOSBox Pure copyright and license notices;
2. identify the exact source revision used;
3. make the complete corresponding source and build material available in the
   manner required by GPLv2;
4. mark ReverieVR modifications to GPL-covered files if such modifications are
   ever made;
5. avoid bundling proprietary DOS game data merely because the runtime can load
   it.

The fetch scripts acquire unmodified pinned upstream source. Any future patch
set must be stored explicitly under this repository and documented rather than
being an unrecorded edit inside the ignored checkout.
