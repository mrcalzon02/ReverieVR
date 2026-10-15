# Daydream Doom guest-control arrangement — 2026-10-15

Scope: DOS Doom profile only. The Cardboard-based head input and physical
headset camera remain unchanged. Touchpad movement produces held arrow
keys until the finger leaves the directional sector or touchpad;
releasing a DOS-held direction suppresses shell swipe navigation.
Touchpad click = held DOS mouse button 1 (fire); separate Daydream
App/minus key = held DOS mouse button 2 (strafe per bundled DOOM.CFG).
Daydream Home/zero opens the universal Quick Menu instead of recentering
during DOS gameplay; the usual shell/recovery menu works outside gameplay.

The two *physical Daydream controller volume rocker* keys no longer
inject Enter and Escape in Doom. They request previous/next weapon.
Original Doom v1.9 uses numbered weapon slots 1..7, not next/previous
commands: on a new rocker press, issue one queued numeric key pulse,
starting at slot 2 (pistol), wrapping across seven slots. It does not
know the player's current equipped weapon/available inventory, so
selecting an unowned slot can be ignored by vanilla Doom. On-screen
Quick Settings still provides volume adjustment. Nothing changes
handset side volume-key filtering or other DOS/native profiles.

Existing module-local copies of precisely the old stock Doom binding
layout migrate to the new stock mapping. Profiles with user changes are
not overwritten. Gamepad semantics remain separate from shell recovery.

User requested a later pull-up ghost keyboard for typing-heavy DOS
games, using controller-directed hunt-and-peck typing; deferred in
BACKLOG.md and not mixed into the Doom profile implementation.

Regression source tests cover held mouse buttons, directional holds,
rocker edge-vs-hold, stock profile upgrade and Home action policy.
Source is not a physical Galaxy S9 acceptance result. No APK released
by this source-only change.
