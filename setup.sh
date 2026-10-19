#!/bin/sh
# Castle Crashers DS setup: finds your own copy of Castle Crashers, converts its assets and builds castle-crashers-ds.nds.
set -e
cd "$(dirname "$0")"
PY=$(command -v python3 || command -v python || true)
if [ -z "$PY" ]; then
  echo "Python 3 is required (https://www.python.org/downloads/)." >&2
  exit 1
fi
exec "$PY" tools/setup_assets.py "$@"
