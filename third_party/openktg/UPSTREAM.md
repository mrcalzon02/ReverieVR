# OpenKTG provenance

ReverieVR vendors the minimal OpenKTG source needed by the first native
procedural module.

- Upstream repository: https://github.com/jaromil/kkrieger-werkkzeug3
- Pinned revision: `72f7697c8b5be6fadae41f9ca6312cd5f88fdc4c`
- Upstream paths:
  - `ktg/gentexture.cpp`
  - `ktg/gentexture.hpp`
  - `ktg/types.hpp`
  - `ktg/LICENSE`
- License: public domain, per `ktg/LICENSE` and source headers.
- Purpose: CPU-side procedural texture generation for trusted packaged native
  modules.

The wider Werkkzeug3/Kkrieger repository is not vendored by this directory.
Any later source import requires its own platform-dependency and license audit.

Do not replace these files from an unpinned upstream checkout.
