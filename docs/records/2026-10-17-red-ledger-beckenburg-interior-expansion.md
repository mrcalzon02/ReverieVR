# Red Ledger / Between Deliveries — Beckenburg Interior Expansion (2026-10-17)

Source-only GitHub change: expanded **5 unique props to 14** and **25 room placements** in the live native GLES2 draw path. Models from the approved Beckenburg Project Drive collection are selected explicitly. No third-party asset searches and no raw proprietary mesh data added to public GitHub.

**Bar:** concrete modular wall and window surround, an actual door mesh, pendant lamp variants tied to the purchased ceiling tier, chair and table models, and chairs progressively introduced by the five seating tiers. The floor model appears when the floor renovation is purchased.

**Stockroom:** modular wall and floor, genuine shelf, two wood crates, and a metal barrel. Existing owner-exported inventory FBX products and selectable stock cards are unaffected.

**Living quarters:** modular wall, window surround and floor, genuine light switch, mattress, and coffee table; bed and room navigation hitboxes unchanged.

**GLES2 behavior:** unique models are loaded, validated and GPU-uploaded once per context; instances reuse buffers and per-material RGB565 textures. Context abandonment/recreation and invalid model fallback remain intact. The old seating cubes are displayed only when the curated chair model is missing. Transforms use axis-aligned scale and integer 90° yaw, no per-eye trig; renovation tiers are sourced from the already serialized Renovations state.

**Staging only:** the 14 licensed game-embedded `.rlprop` files are not part of public source because commercial game use does not independently establish public raw/derived-asset redistribution permission. Put those files in `app/src/main/assets/native/red-ledger/beckenburg/` in an authorized local APK build. The runtime logs the actual loaded count; without these binaries, existing primitive rooms remain as fallback.

**Verification scope:** C++17 host decoder/unit tests run on the 14 converted local models. Remote readback of GitHub commit confirms SOURCE changes, not that an APK includes the private models. Android NDK build, Galaxy S9 headset screenshot, VR controls, GPU performance and thermal checks remain unverified. No GitHub Actions.
