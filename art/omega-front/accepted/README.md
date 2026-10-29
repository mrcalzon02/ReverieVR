# Omega Front accepted painterly tactical icon art

The **primary visual guide** is the original gold/steel-blue symbol artwork at `art/omega-front/style-guide/omega_primary_symbols.png`. It is a style reference, not another army.

**Four faction-specific icon sets (80 PNGs total)** are checked in at `app/src/main/assets/native/omega-front/ui/icons/f06` through `f09`. Each has **20 square 256×256 PNG icons**. The separate 1280×1024 reference atlases and file-by-file SHA-256 manifests are in `art/omega-front/accepted/f06` … `f09`.

F06 Lost Ninth (Roman), F07 Ashen Crown (medieval feudal), F08 Copper Steppe Confederation (Bronze Age nomads), and F09 Brassline Republic (rail-and-trench industrial era) follow their existing candidate dossiers in `docs/project/OMEGA_FRONT_FACTIONS.md`. Role labels and named factions are *provisional*. These visuals are menu/roster portraits, not battlefield top-down sprites.

**Do not include** rejected generated sheets of Iron Syndicate, Skyward Dominion, Northern Alliance, unrelated armies, or rectangular preview cards as unit icons.

The icons are Android assets under the native Omega directory; that does **not** by itself imply menu code or the native texture renderer selects them. Verify in-game integration separately. Don't alter tactical rules to fit artwork.
