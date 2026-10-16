# Gumball Armada — Named Lane Hazards and Paid Ally Contracts

**State (2026-10-16):** Native-game **source integrated, only isolated support contract C++ tests locally verified**. Full integrated native C++ runner, Android/NDK APK, physical Galaxy S9 Daydream controller, stereo rendering and sustained performance remain pending. This is not a released/accepted game system. No portrait images, sprite files or voice recordings are included.

**Related:** [Character Bible](GUMBALL_ARMADA_CAST_BIBLE.md), [staged plan](GUMBALL_ARMADA_STAGED_PLAN.md), [content matrix](GUMBALL_ARMADA_CONTENT_MATRIX.md), [main game status](GUMBALL_ARMADA.md). New original characters supplement the original 14 (none is a replacement for Pip/Lolly/Blort or the four main Sour generals).

## 1. Route identity: choose which menace you hate least

The existing chart has three forward route destinations. Each currently predicts one deterministic seeded cameo chosen from *none*, Pip Parfait, Lolly Wren, Blort Blister, Nib Noxious, the Crumbclank twins, **Cinnamon Charles**, **Sydney Slipstream**, or **Jawbreaker Jules**. New worlds/forward progress can yield different selections. The **first sortie has no cameo**, making the starter learning run predictable.

An enemy's name is a real threat forecast, not decorative text. The preview derives from the same `ChooseCastGuest(seed, destination ID, completed visits, risk)` function used by the launched combat simulation. The three candidate cards show lane address, risk, reward, mission class and forecast name. This does **not** yet constitute a freely traversable, revisitable node graph (S10 remains open), and there are no permanent named-boss territory claims.

### CAST-CHARLES — Cinnamon Charles, zoned barrage bombardier
**Character:** a charmingly insufferable cinnamon-fireworks showman who announces bombing patterns as if selling premium tickets. His brown/red spice bomber has twin fuse-turrets and a distinctive little pilot hat; he calls his artillery a "delightful sprinkle" regardless of how thoroughly it ruins someone's afternoon.

**Implemented source behavior:** one named mid-wave enemy replaces an ordinary craft in eligible lanes. At a bounded firing cadence it seeds three shots across one of three left/center/right lanes, chosen by deterministic sortie RNG. Each bullet begins with a **stationary, visible 0.68-second danger telegraph**, during which it cannot inflict damage; after warning it descends rapidly, with a visible distinct orange marker and actual damage/collision. Players can change lanes to avoid the predicted pattern. These are real finite `Shot` entities, not a post-processed full-screen animation.

**Future work:** original bomb-hat portrait, authored flare sprites/audio, spoken threat warnings, late-game layered salvo patterns, accessibility/headset tuning. Preserve telegraph-to-hit interval and bounded projectile counts.

**Suggested radio:** "Mes amis! A little cinnamon, and a magnificent mess!"

### CAST-SYDNEY — Sydney Slipstream, speed-field trickster
**Character:** bright, smug and infinitely athletic, pilots an aquamarine arrowhead racer fitted with syrup-flow vanes. Sydney thinks the laws of motion are a polite recommendation. Hates heavy vessels, loves making the player choose the inconvenient half of the screen.

**Implemented source behavior:** one named mid-wave enemy maintains alternating **left-fast/right-slow** and **left-slow/right-fast** regions every three seconds while alive, with a central neutral strip. Flight speed changes based on the player ship's *actual current horizontal position* and the active fast side (approximately 1.48× or 0.52× base control movement, 1× in center). The status text and left/right field indicators change with the alternation; movement remains based on touchpad contact rather than requiring click/hold. The effect ceases when Sydney is no longer active, subject to the next logical update.

**Future work:** visible scrolling current/ribbon effects, optional resistance parts, a stronger field transition tell, motion comfort/headset proof and challenge missions where the fastest route isn't always safest.

**Suggested radio:** "Oh, darling, you seem to be moving at yesterday's speed."

### CAST-JULES — Jawbreaker Jules, armored hazard bruiser
**Character:** a hulking lilac jawbreaker gangster who compensates for poor aim by carrying a cannon approximately the size of a bus. Talks like a cheerful prizefighter and believes every encounter should be settled with the heaviest possible projectile.

**Implemented source behavior:** eleven-hit-point, distinct heavy hull inserted mid-wave on eligible seeded routes. Fires **two physically diverging**, slow, high-damage bomb-like projectiles in the existing interceptable fixed pool; the large orbs can be shot down using the established bomb defusal mechanics. Not merely a Bomber recolor: meaningful survival time, slower shot geometry and high damage create another prioritization problem.

**Future work:** hull weak points, staged armor break effects, player countermeasures, unique encounter contracts, original art and a vocal performance.

**Suggested radio:** "No hard feelings! They call me HARD CANDY!"

## 2. Two purchasable French-themed support pilots

The original random allies **Pip Parfait** and **Lolly Wren** remain possible natural lane guests. Paid support is a **separate reserved escort slot** that can deploy on the same sortie; no retcon or replacement is intended. Both paid escorts currently appear automatically after a short deterministic stagger inside their deployment window, rather than binding unavailable extra controller buttons. No external marketplace, online authentication or real-money purchases are involved.

### CAST-ECLAIR — Capitaine Éclair, Royal lightning escort
**Character:** glorious overconfident French-flavored dragoon pilot; tiny moustache, sweeping blue scarf, sugar-crystal epaulettes and an immaculately polished indigo strike skiff. Calls the player *mon capitaine* and treats every shootout like a grand air parade. Unlike Pip, the Capitaine is a professional available on contract for a fee.

**Base hire price:** **45 Gumballs for one mission**, debited immediately from campaign cash. Only one pending paid support booking is allowed at a time; it is consumed at the next mission launch, not every time the game renders a chart.

**Own upgrade tree (levels 0–3 each):**
- **Éclair Volley**: allied shot damage 2, 3, 4, 5 by level 0–3. Its shots actually attack active enemy health and share the normal kill/reward pipeline without counting toward the player's personal shots-fired/hit statistics.
- **Éclair Duration**: maximum escort volleys 3, 4, 5, 6 by level 0–3; a bounded window and ordinary projectile pool still apply.

The escort finds a live opponent below the top lane and fires actual projectile instances, approximately one per 1.8 seconds when a legal target/shot slot exists. It consumes a pulse only after it spawns a real projectile; empty lanes don't fabricate success.

**Suggested radio:** "En garde, mon capitaine! The sky belongs to us!"

### CAST-MACARON — Mademoiselle Macaron, airborne pâtisserie rescue
**Character:** charming, resourceful French rescue/salvage pilot with a lavender macaroon rescue tug, beret tilted toward the engine and a tiny folding teacup balanced beside the instruments. Sweetly disapproves of the player's habit of treating enemy cannons as an invitation. Her voice is sunny, but her repairs are exceptionally practical.

**Base hire price:** **55 Gumballs for one mission**, debited immediately. She occupies the same single pending paid-support slot as Éclair, so they cannot both be pre-booked for the same run; naturally occurring Pip or Lolly encounters can still coexist.

**Own upgrade tree (levels 0–3 each):**
- **Macaron Relief**: restores up to 7, 10, 13, 16 **missing shield points** per successful rescue pulse, never exceeding the equipped ship's shield capacity.
- **Macaron Tractor**: salvage reach 1.00, 1.18, 1.36, 1.54 world units per pulse (Euclidean radius), consuming exactly one *existing* field pickup. Its Gumball value is transferred to campaign and sortie totals once; no fake currency is minted.

Rescue pulses occur only when missing shield capacity and/or a real salvage object within range make the action useful. Up to four successful pulses during the deployment window. If the ship is undamaged and no salvage appears nearby, there may be no actual relief action; this is a meaningful strategic purchase rather than a guaranteed free-money grant.

**Suggested radio:** "Bonjour, monsieur! You brought the holes; I brought the crème."

## 3. Support purchase, UI, economics and persistence

The compact in-VR Chart has a **SUPPORT** entry next to SHIPYARD. The support screen lists both hires, the four named skill upgrades, their current levels/costs and one Return option. Navigate with the existing touchpad up/down and click to choose; no fictitious second gameplay trigger or Shell/Home appropriation.

**Upgrade pricing:** 90 Gumballs to go 0→1, 135 for 1→2, 180 for 2→3, separately per ability. Maximum level 3; attempts beyond cap, purchases with inadequate cash or purchases while in a mission are rejected. **There are exactly two tracks per paid companion**. Upgrades are permanent once bought, while hire charges are single-sortie consumables.

**Single-slot save transaction:** Campaign save **version 3** (56-byte `Snapshot`) holds campaign cash, weapon ownership, rank progression, **packed four two-bit support levels and one two-bit pending support booking**. Hiring or upgrading writes one existing campaign save rather than splitting a cash debit and ownership credit across unsynchronized files. The booking is cleared/consumed when a valid ship launches on the chosen route, and the module writes the campaign save immediately on launch before completing the sortie. The separate `shipyard.v1` layout remains unchanged.

**Compatibility:** `SnapshotV1` (44 bytes) and `SnapshotV2` (52 bytes) retain frozen checksummed readers and map into the new V3 save with support levels zero/unbooked. Restores reject unknown upper support bits, invalid booking ID, corrupt checksums, unrealistic balances/levels and illegal weapon ownership without mutating the ongoing Simulation. Data loss on a failed host write remains surfaced to the user via existing campaign-save error feedback; this is not a verified crash-consistent database.

**Balancing/acceptance obligations:** support purchases cannot create unlimited cash; real ally bullets, kills, salvage pickups and shields must be observed; no assistance should guarantee boss deletion; mission failure does not refund the cost; no support shot can overrun the fixed 48-projectile pool. Player should be able to recognize their hired escort using a compact silhouette and name/HUD without overlapping core mission information. Test the start-of-mission save and migration on a real phone before calling support purchases durable under process kill or Android updates.

## 4. Verification and remaining gates

**Verified locally:** exact source of `gumball_support_contracts.h` and `gumball_support_contracts_test.cpp` matched committed GitHub blob SHAs and passed strict C++17 `g++`, `clang++` plus Clang AddressSanitizer/UndefinedBehaviorSanitizer runs. Tests cover cost debits, duplicate hire refusal, insufficient funds, upgrade cap and isolation, skill effects and state reconstitution. Source integration tests have been extended for V2→V3 migrations, corrupted support-state rejection, paid mission consumption, real escort fire and all three new route threat identities. **Those integrated tests have not yet been executed against an exact checkout**; native shell ABI/NDK compilation, APK install, controller navigation, both-eye visuals and extended thermals remain pending.

**Still planned:** region reputation and relationship unlocks, purchasable temporary fighter squadrons beyond Éclair/Macaron, special-use hull/health booster *consumable items* (Macaron currently restores existing shield instead), Extra Lime continues, portraits, original recorded French-tinged voices, authored audio/SFX, shop operators, full S10 revisitable graph and complete Candy King campaign. Avoid claiming these from the two base contracts.

**Art provenance:** do not fabricate portrait paths or add image assets during this pass. Future illustrations can follow the cast bible without changing gameplay IDs.
