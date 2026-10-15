#!/usr/bin/env sh
set -eu
# Author-hosted original DOS freeware package, never repackaged.
SOURCE_URL="https://advsys.net/ken/klab/labfull.zip"
EXPECTED_SHA256="643aa0ca8d1a7d2c88afecd952da00f3b912be8b83c4c1d2f4ab9b50491fadfd"
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
ACTUAL_BYTES="$(wc -c < "$TMP" | tr -d ' ')"
if [ "$ACTUAL_BYTES" != "940242" ]; then
    echo "Ken's Labyrinth author ZIP size mismatch: $ACTUAL_BYTES bytes." >&2
    rm -f "$TMP"
    exit 1
fi
if ! unzip -tq "$TMP" >/dev/null; then
    echo "Ken's Labyrinth archive failed ZIP integrity check." >&2
    rm -f "$TMP"
    exit 1
fi
if [ "$ACTUAL" != "$EXPECTED_SHA256" ]; then
    rm -f "$TMP"
    echo "Ken's Labyrinth SHA-256 mismatch. Expected $EXPECTED_SHA256; actual $ACTUAL" >&2
    exit 1
fi
mv "$TMP" "$DEST"
echo "Verified Ken's Labyrinth v2.1 original archive: $DEST"
