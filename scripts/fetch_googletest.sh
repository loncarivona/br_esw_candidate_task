#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEST="$ROOT/third_party/googletest"
VERSION="v1.15.2"

if [[ -f "$DEST/CMakeLists.txt" ]]; then
  echo "googletest already present at $DEST"
  exit 0
fi

mkdir -p "$ROOT/third_party"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

curl -L -o "$TMP/googletest.zip" \
  "https://github.com/google/googletest/archive/refs/tags/${VERSION}.zip"
unzip -q "$TMP/googletest.zip" -d "$TMP"
mv "$TMP/googletest-"* "$DEST"

echo "Installed googletest ${VERSION} to $DEST"
