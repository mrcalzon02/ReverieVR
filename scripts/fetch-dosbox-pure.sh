#!/usr/bin/env sh
set -eu

REPO_URL="https://github.com/schellingb/dosbox-pure.git"
PIN="a4a0bab7f8931433588f2fcad9045c85b277373d"
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
DEST="$ROOT/third_party/dosbox-pure/src"

if ! command -v git >/dev/null 2>&1; then
    echo "git is required to fetch DOSBox Pure." >&2
    exit 127
fi

if [ -d "$DEST/.git" ]; then
    if [ -n "$(git -C "$DEST" status --porcelain)" ]; then
        echo "Refusing to modify dirty DOSBox Pure checkout: $DEST" >&2
        exit 1
    fi
    git -C "$DEST" fetch --tags origin
else
    mkdir -p "$(dirname "$DEST")"
    git clone --no-checkout "$REPO_URL" "$DEST"
fi

git -C "$DEST" checkout --detach "$PIN"

ACTUAL="$(git -C "$DEST" rev-parse HEAD)"
if [ "$ACTUAL" != "$PIN" ]; then
    echo "DOSBox Pure pin verification failed." >&2
    echo "Expected: $PIN" >&2
    echo "Actual:   $ACTUAL" >&2
    exit 1
fi

echo "DOSBox Pure ready at $ACTUAL"
