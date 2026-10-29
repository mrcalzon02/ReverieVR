# Native Games gallery — live Home-room correction (2026-10-28)

State: **source implemented, Android compilation and physical Galaxy S9 verification pending**.

The former Native Games action switched to `MODE_NATIVE_GAMES`, disabling actual Home locomotion and suppressing the physical keyboard render and ray/physical HID input. That was a regression, not the intended in-room window. The Home Native Games launcher now toggles `nativeGamesPanelVisible` while keeping `MODE_HOME`; it draws a four-card-per-page browsing window **only inside the existing center-wall Home panel texture** and handles selection through the exact same panel's world-space pointer ray. The left and right Home walls, ambience and geometry, movement, 3D desk keyboard, Home controllers, gaze recovery and global Quick Menu remain as before.

Pointer and keyboard IDs are distinct; hover/dwell cannot treat a gallery card as a desk key. Back/Close dismiss the gallery while staying inside Home; a successful selection alone enters the chosen real bundled module with existing native lifecycle and failure checks. Native Tools is a separately requested destination and can still open a tool mode. No Manage Games button is introduced.

Tests cover all 15 registered titles over 4 pages, disabled empty slots and the non-overlapping in-wall hit boxes. Build/runtime/S9 acceptance is not claimed until verified.
