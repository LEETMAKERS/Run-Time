#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
test_dir="$(mktemp -d)"
trap 'rm -rf -- "$test_dir"' EXIT
for suite in rules game; do
  "${CXX:-g++}" -std=c++11 -Wall -Wextra -Werror -pedantic \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    -I tests/fakes "tests/${suite}_test.cpp" -o "$test_dir/$suite"
  "$test_dir/$suite"
done
