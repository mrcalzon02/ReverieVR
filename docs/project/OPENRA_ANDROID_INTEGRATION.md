# ReverieVR — OpenRA / Classic RTS Android Integration

**Status (2026-10-15):** OpenRA Android source is admitted to the **default Emulator Modules source catalogue**. **No OpenRA executable, integrated VR runtime, bundled game data, or working PLAY action exists in ReverieVR.** This distinction is a release blocker, not a permission to claim the games are included or working.

**Target:** Red Alert first, then Tiberian Dawn and Dune 2000 after individually verified mod/content compatibility. OpenRA is a reimplementation of the classic RTS engine, **not a DOS emulator or Libretro core**. Do not substitute the obsolete, incomplete `damiencarol/openredalert` C++/SDL engine.

## Upstreams and dependencies

- Authoritative upstream: https://github.com/OpenRA/OpenRA (GPL-3.0).
- Android implementation reference and current default source-catalog entry: https://github.com/franktfye/OpenRA-android (GPL-3.0). Its `android/README.md` reports a working native Android Red Alert UI/skirmish engine using `net10.0-android`, ARM64 native dependencies, .NET/Mono JIT, GLES 3, and an EGL `SurfaceView`; its existing application is **standalone**, not integrated with ReverieVR.
- Reference build requirements documented upstream: .NET 10 Android workload, Android SDK platform 36, Android NDK 29; libfreetype, Lua 5.1 and OpenAL-soft native dependencies. Check actual minSdk, ABI, linkability, APK size, licensing and Galaxy S9/Android-version compatibility **before** committing a runtime dependency or binary.
- OpenRA engine code is GPL-3.0; original Westwood/EA art, audio and game files are separately copyrighted and **not GPL engine assets**. The fork specifically excludes `global mix database.dat` from Git. Do not package original `.mix` archives or that database without verified redistribution rights; an in-app explicit legal content import/acquisition path is required.

## Default-inclusion and hosting contract

`EmulatorModuleCatalog` carries one allowlisted `openra-android` entry with a distinct `OPENRA_ANDROID` host type and **no fake `*_libretro_android.so` filename**. Catalog metadata is local/offline; selecting it uses the existing explicit, pinned, license-checked source-snapshot flow. It is **not** a playable core download or extra APK install. Existing source ZIP size/integrity limits apply: size failure is an honest error, not a reason to bypass integrity checks.

The user requirement is an **actual default, phone-local, single-ReverieVR-APK Red Alert game**. A source link or a shortcut to install the fork's standalone `org.openra.app` APK does not fulfill it. Preserve the shell's headset-only recovery and all offline/native experiences.

## Execution sequence and acceptance gates

1. Build the Android fork's exact pinned source unmodified, on its documented toolchain. Exercise Red Alert menu, skirmish vs AI, touch/mouse/keyboard, pause/resume, audio and content import on a supported Android device; distinguish upstream's tablet/emulator proof from our **unverified S9**.
2. Demonstrate `arm64-v8a` and actual S9 OS/GLES compatibility; determine .NET runtime loading and third-party native `.so` footprint inside one Gradle-produced ReverieVR APK. Produce an artifact-size/resource report and retain GPL source-notice compliance. Do not commit upstream signing keys or game-data files.
3. Integrate **one offscreen RTS framebuffer** into the existing stereo renderer's flat virtual screen, using a compatible GLES3 render path or a verified GPU-sharing bridge. Advance simulation only once per tick; composite the same map framebuffer for both eyes. Protect context loss, pause/exit, memory and audio ownership; never let the engine block the VR render thread.
4. Route gaze/head ray to precise map coordinates, touchpad four-direction hold to scroll/pan, Select/touchpad click to left mouse, Daydream minus/App to right mouse/command and Home/O exclusively to the ReverieVR Quick Menu and recovery. Make selection/command gestures repeat-safe; provide controller-held drag, map-edge movement, zoom and a shell virtual keyboard for RTS text fields without requiring an external mouse. Preserve user-rebindable policies.
5. Add per-mod content checks and a user-authorized in-app import/install process with bounded validation, correct licenses and offline persistence. Test RA first; **never advertise TD/Dune 2000 as working just because the base OpenRA project supports them**.
6. Only expose a real **PLAY** entry under the default native/classic games destination after phone-only install, offline boot, one complete skirmish against AI, a save/return path, controller-only menu/build/selection/command handling, stereo frame pacing, normal/suspended resume, and sustained S9/Daydream thermal tests pass. Mark all remaining gates unverified until direct evidence exists.

**Non-goals:** no remote PC hosting, no separate required application, no mere external webpage launch, no bundled commercial assets, no Libretro-ABI claim, no disabled/fake PLAY button and no GitHub Actions dependency introduced for this work.

Verification of this initial slice is limited to catalogue enumeration, host type, license rejection, immutable source URL rules and remote file read-back. A Java test cannot certify an APK build or a headset play session.
