#!/usr/bin/env bash
set -euo pipefail

if [[ -z "${EMSDK:-}" ]]; then
  if [[ -f "${HOME}/Programs/emsdk/emsdk_env.sh" ]]; then
    source "${HOME}/Programs/emsdk/emsdk_env.sh"
  else
    source "${HOME}/emsdk/emsdk_env.sh"
  fi
fi

cd "$(dirname "$0")/.."
premake5 gmake
emmake make config=web core main
echo "Open http://127.0.0.1:8080/traitvis.html"
python3 -m http.server 8080 --directory bin/Web
