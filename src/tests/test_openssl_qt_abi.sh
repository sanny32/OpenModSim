#!/usr/bin/env bash
set -eu

fixture=$(mktemp -d)
trap 'rm -f -- "$fixture"/*; rmdir -- "$fixture"' EXIT

cmake="$1"
test_script="$2"
compiler="$3"
readelf="$4"

check_abi() {
    "$cmake" -DCMAKE_SYSTEM_NAME=Linux "-DCMAKE_READELF=$readelf" \
        "-DQT_BINARY=$fixture/qt-network" "-DEXPECTED_ABI=$1" -P "$test_script"
}

printf 'O\0p\0e\0n\0S\0S\0L\0 \0\064\0.\0\061\0.\0\060\0\0\0' > "$fixture/qt-network"
check_abi 4

printf 'void crypto() {}\n' > "$fixture/crypto.cpp"
printf 'void crypto(); void network() { crypto(); }\n' > "$fixture/network.cpp"
for abi in 1.1 3; do
    "$compiler" -shared -fPIC "-Wl,-soname,libcrypto.so.$abi" -o "$fixture/libcrypto.so" "$fixture/crypto.cpp"
    "$compiler" -shared -fPIC -o "$fixture/qt-network" "$fixture/network.cpp" -L"$fixture" -lcrypto
    check_abi "$abi"
done
