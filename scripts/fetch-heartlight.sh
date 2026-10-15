#!/usr/bin/env sh
set -eu
# Co-author Maciej Miasik's CC BY-SA 2.5 registered Heartlight PC DOS build,
# hosted through DOS Games Archive. Preserve ZIP bytes and all README terms.
URL="https://www.dosgamesarchive.com/file.php?id=2352"
EXPECTED_BYTES=466104
EXPECTED_MD5="df6e57581ad95255fc457ab2815616be"
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
DEST="$ROOT/third_party/heartlight/payload/default-content/hl.zip"
TMP="$DEST.tmp"
mkdir -p "$(dirname "$DEST")"
trap 'rm -f "$TMP"' EXIT
curl --fail --location --proto '=https' --tlsv1.2 "$URL" --output "$TMP"
size="$(wc -c < "$TMP" | tr -d ' ')"
if [ "$size" != "$EXPECTED_BYTES" ]; then
    echo "Heartlight archive size mismatch: expected $EXPECTED_BYTES, got $size" >&2
    exit 1
fi
if command -v md5sum >/dev/null 2>&1; then
    md5="$(md5sum "$TMP" | awk '{print $1}')"
else
    md5="$(md5 -q "$TMP")"
fi
if [ "$md5" != "$EXPECTED_MD5" ]; then
    echo "Heartlight original archive MD5 mismatch; reject download." >&2
    echo "Expected $EXPECTED_MD5, actual $md5" >&2
    exit 1
fi
unzip -tq "$TMP" >/dev/null
if ! unzip -Z -1 "$TMP" | grep -Eiq '(^|/)HL\.EXE$'; then
    echo "Heartlight archive missing the directly executable HL.EXE." >&2
    exit 1
fi
mv "$TMP" "$DEST"
sha="$(sha256sum "$DEST" | awk '{print $1}')"
echo "Verified Heartlight PC original CC BY-SA 2.5 archive."
echo "SHA256 $sha  hl.zip"
