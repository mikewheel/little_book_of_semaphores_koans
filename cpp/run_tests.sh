#!/usr/bin/env bash
# Configure, build, and run the C++ koan tests inside the container.
#   run_tests.sh            build + run everything
#   run_tests.sh 05         build + run koans matching "05"
set -uo pipefail

BUILD_DIR="${BUILD_DIR:-/build/default}"
PATTERN="${1:-}"

cmake -S /work -B "$BUILD_DIR" -G Ninja >/dev/null || exit 2

if [[ -n "$PATTERN" ]]; then
  targets=()
  for d in /work/koans/*"$PATTERN"*/; do
    [[ -d "$d" ]] || continue
    compgen -G "${d}test_*.cpp" >/dev/null || continue
    targets+=("$(basename "$d")")
  done
  if [[ ${#targets[@]} -eq 0 ]]; then
    echo "run_tests: no C++ koan matches '$PATTERN'" >&2
    exit 2
  fi
  cmake --build "$BUILD_DIR" --target "${targets[@]}" || exit 1
  regex="^($(IFS='|'; echo "${targets[*]}"))$"
  ctest --test-dir "$BUILD_DIR" -R "$regex" --output-on-failure
else
  cmake --build "$BUILD_DIR" || exit 1
  ctest --test-dir "$BUILD_DIR" --output-on-failure
fi
