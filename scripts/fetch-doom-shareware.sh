#!/usr/bin/env sh
set -eu

SOURCE_URL="https://www.gamers.org/pub/idgames/idstuff/doom/doom19s.zip"
EXPECTED_SHA256="cacf0142b31ca1af00796b4a0339e07992ac5f21bc3f81e7532fe1b5e1b486e6"
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
DEST="$ROOT/third_party/doom-shareware/payload/default-content/doom19s.zip"
TMP="$DEST.tmp"

if ! command -v curl >/dev/null 2>&1; then
    echo "curl is required to fetch DOOM Shareware." >&2
    exit 127
fi

mkdir -p "$(dirname "$DEST")"
rm -f "$TMP"

curl --fail --location --proto '=https' --tlsv1.2     "$SOURCE_URL"     --output "$TMP"

if command -v sha256sum >/dev/null 2>&1; then
    ACTUAL="$(sha256sum "$TMP" | awk '{print $1}')"
elif command -v shasum >/dev/null 2>&1; then
    ACTUAL="$(shasum -a 256 "$TMP" | awk '{print $1}')"
else
    echo "sha256sum or shasum is required to verify DOOM Shareware." >&2
    rm -f "$TMP"
    exit 127
fi

if [ "$ACTUAL" != "$EXPECTED_SHA256" ]; then
    echo "DOOM Shareware checksum verification failed." >&2
    echo "Expected: $EXPECTED_SHA256" >&2
    echo "Actual:   $ACTUAL" >&2
    rm -f "$TMP"
    exit 1
fi

mv "$TMP" "$DEST"
echo "Verified original DOOM Shareware v1.9 archive:"
echo "  $DEST"
echo "  SHA-256 $ACTUAL"
