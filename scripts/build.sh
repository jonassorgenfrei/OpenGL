#!/usr/bin/env bash
set -euo pipefail

root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_type="${BUILD_TYPE:-Release}"
build_dir="${BUILD_DIR:-$root/build/linux}"
jobs="${BUILD_JOBS:-2}"

if [[ "${1:-}" == "--help" ]]; then
    echo "Usage: bash scripts/build.sh [--skip-system-deps] [extra CMake arguments...]"
    echo "Environment: BUILD_TYPE=Release BUILD_DIR=<path> BUILD_JOBS=2"
    exit 0
fi

if [[ "${1:-}" == "--skip-system-deps" ]]; then
    shift
else
    if ! command -v apt-get >/dev/null; then
        echo "Automatic system setup supports Debian/Ubuntu. Install a C/C++ compiler, CMake >=3.24, Git, Python 3, pkg-config, OpenGL and X11 development libraries, then use --skip-system-deps." >&2
        exit 1
    fi
    elevate=()
    if (( EUID != 0 )); then elevate=(sudo); fi
    "${elevate[@]}" apt-get update
    "${elevate[@]}" apt-get install -y build-essential cmake git python3 pkg-config \
        libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
fi

cmake -S "$root" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE="$build_type" -DOPENGL_FETCH_DEPENDENCIES=ON \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 "$@"
cmake --build "$build_dir" --config "$build_type" --parallel "$jobs"
