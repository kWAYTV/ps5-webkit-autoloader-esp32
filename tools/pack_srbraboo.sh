#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WEB="${WEBKIT_AUTOLOADER:-$ROOT/webkit-autoloader}"
if [ ! -f "$WEB/Makefile" ]; then
    echo "Error: srbraboo tree missing at $WEB" >&2
    echo "Add the submodule: git submodule update --init --recursive webkit-autoloader" >&2
    exit 1
fi
python3 "$ROOT/tools/pack_srbraboo.py" --web "$WEB" --out "$ROOT/autoloader"
