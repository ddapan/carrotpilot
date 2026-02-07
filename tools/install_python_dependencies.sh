#!/usr/bin/env bash
set -e

# Increase the pip timeout to handle TimeoutError
export PIP_DEFAULT_TIMEOUT=200

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
ROOT="$DIR"/../
cd "$ROOT"

if ! command -v "uv" > /dev/null 2>&1; then

  ARCH=$(uname -m)

  if [ "$ARCH" = "x86_64" ]; then
    echo "Detected x86_64, copying uv from predefined path..."
    cd $(dirname "$(readlink -f "$0")")
    UV_BIN="$HOME/.local/bin"
    mkdir -p "$UV_BIN"
    cp  uv/* "$UV_BIN/"
  else
    echo "Installing uv via astral.sh..."
    curl -LsSf https://astral.sh/uv/install.sh | sh
    UV_BIN="$HOME/.local/bin"
  fi

  PATH="$UV_BIN:$PATH"
fi

echo "updating uv..."
# ok to fail, can also fail due to installing with brew
#uv self update || true

echo "installing python packages..."
uv sync --frozen --all-extras
uv pip install xattr flask
source "$ROOT/.venv/bin/activate" || source "$ROOT/.venv/Scripts/activate" || echo "Virtual environment activated"

echo "PYTHONPATH=${PWD}" > "$ROOT"/.env
if [[ "$(uname)" == 'Darwin' ]]; then
  echo "# msgq doesn't work on mac" >> "$ROOT"/.env
  echo "export ZMQ=1" >> "$ROOT"/.env
  echo "export OBJC_DISABLE_INITIALIZE_FORK_SAFETY=YES" >> "$ROOT"/.env
fi
