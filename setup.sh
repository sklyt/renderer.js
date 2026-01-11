#!/usr/bin/env bash
set -e

echo "=== raylib-napi setup script ==="

if command -v brew >/dev/null 2>&1; then
  echo "Detected Homebrew: installing raylib..."
  brew install raylib
  echo "raylib installed via Homebrew."
elif [ -f /etc/debian_version ]; then
  echo "Detected Debian/Ubuntu. Trying apt-get (may not have raylib packaged)."
  echo "You may need to build raylib from source if unavailable."
  sudo apt-get update
  # package name may vary by distribution
  sudo apt-get install -y libraylib-dev || echo "libraylib-dev not available via apt. Please build raylib from source: https://www.raylib.com/"
else
  echo "Could not detect package manager. Please install raylib manually:"
  echo "  macOS: brew install raylib"
  echo "  Linux: build from source or use your distro package manager"
  echo "See https://www.raylib.com/ for details."
fi

echo "Installing npm dependencies..."
npm install

echo "Running npm build (node-gyp configure build)..."
npm run build

echo "Done. Try: node index.js (this will open a raylib window)."
