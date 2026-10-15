#!/usr/bin/env sh
set -eu
# Author-hosted original DOS freeware package, never repackaged.
SOURCE_URL="https://advsys.net/ken/klab/labfull.zip"
EXPECTED_SHA256="5baf67ed3795be0dff3f32819f0c20fa84d3a00d667215d7a1146a7971c81072"
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
DEST="$ROOT/third_party/kens-labyrinth/payload/default-content/labfull.zip"
TMP="$DEST.tmp"
if ! command -v curl >/dev/null 2>&1; then
    echo "curl is required to fetch Ken's Labyrinth." >&2
    exit 127
fi
mkdir -p "$(dirname "$DEST")"
rm -f "$TMP"
curl --fail --location --proto '=https' --tlsv1.2 "$SOURCE_URL" --output "$TMP"
if command -v sha256sum >/dev/null 2>&1; then
    ACTUAL="$(sha256sum "$TMP" | awk '{print $1}')"
elif command -v shasum >/dev/null 2>&1; then
    ACTUAL="$(shasum -a 256 "$TMP" | awk '{print $1}')"
else
    rm -f "$TMP"
    echo "SHA-256 tool is required." >&2
    exit 127
fi
if [ "$ACTUAL" != "$EXPECTED_SHA256" ]; then
    rm -f "$TMP"
    echo "Ken's Labyrinth SHA-256 mismatch. Expected $EXPECTED_SHA256; actual $ACTUAL" >&2
    exit 1
fi
mv "$TMP" "$DEST"
echo "Verified Ken's Labyrinth v2.1 original archive: $DEST"
