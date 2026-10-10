# ReverieVR original Sierra-era MIDI style standard

**Design rule for all native game projects. Status: not yet implemented as an audible native service.**

Use compact composed note-event sequences with melodic identity, intentional intro/loop/outro points, 1980s/1990s PC-adventure influenced GM/MT-32-like roles, original motifs and intentionally small instrument/voice budgets. Never reproduce proprietary Sierra arrangements, songs, patches, banks or MT-32 ROM images.

Implement a deterministic ABI-safe song representation: ticks-per-quarter, tempo map, note on/off, velocity, melodic/percussion instrument role, cue name and loop markers. Validate note-off balance, pitch/event bounds, event ordering and repeatability with host-native tests. A small polyphonic synthesis service must be owned by the ReverieVR shell (pause/resume, audio focus, volume, lifecycle and source arbitration); avoid allocations in the audio callback and oversize streamed music bundles.

**Breakwater Battery:** coastal brass march, shop respite, day engagement, tense nighttime attack, aircraft warning, victory/defeat. **Red Ledger:** smoky off-kilter tavern noir. **Lantern Desk:** restrained mysterious piano/woodwinds. **Forward Detachment:** low snare and field brass/woodwind.

Current shell menu MP3 and controller spring sound are not a native-game MIDI renderer. Do not claim audible MIDI until implemented, tested and physically heard on Galaxy S9.