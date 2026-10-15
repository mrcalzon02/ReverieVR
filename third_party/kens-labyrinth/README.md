# Ken's Labyrinth v2.1 — freeware DOS media

Ken Silverman's official game and download:
https://advsys.net/ken/klab.htm
https://advsys.net/ken/klab/labfull.zip

The game is copyrighted **freeware**, not open source/public domain.
Retain the original complete archive, attribution and distribution files.
A right to sell the game or license its data as MIT is **not** asserted.

* Expected original archive SHA-256: `643aa0ca8d1a7d2c88afecd952da00f3b912be8b83c4c1d2f4ab9b50491fadfd`
* Reported archive size: 940,242 bytes; executable: `KEN.EXE`
* Official author-hosted archive SHA-256 observed in GitHub build log for
  phone test #38684850929 (2026-10-15); the older Softonic mirror archives
  a different checksum and is NOT the release payload.
* Official page lists 940,242 bytes; fetch script verifies that exact size,
  SHA-256 and ZIP structure before admitting the archive into the APK.
* Source archive not tracked in Git. Linux/macOS and Windows fetch scripts
  verify SHA-256 before packaging the original untouched ZIP.
* Free/noncommercial APK content only; `-PexcludeKensLabyrinth` omits it
  when third-party freeware rights would be inappropriate.
* Runtime verification and bounded ZIP extraction occur on the Android phone,
  inside its private DOS-module directory. `DOS.YML` launches KEN.EXE
  directly without relying on an interactive installer or laptop.

Controls: touchpad directions are held arrows; touchpad click is Ctrl/fire;
physical minus/App is Space/use; volume+ Enter, volume- Escape. Circle/Home
is always ReverieVR Quick Menu and never mapped into the DOS guest.
Other original keys (running, additional weapons, strafing) require an
optional customized binding/profile; do not advertise them as available.
