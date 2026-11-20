#!/usr/bin/env bash
set -eu

command -v pkg-config >/dev/null || exit 77
. "$1/tools/check_openssl_version.sh"

fixture=$(mktemp -d)
trap 'rm -f -- "$fixture/openssl.pc"; rmdir -- "$fixture"' EXIT
export PKG_CONFIG_LIBDIR="$fixture"
export PKG_CONFIG_PATH=

expect_version_check() {
    local qt_major="$1" expected="$2" actual
    if check_openssl_version "$qt_major"; then
        actual=accept
    else
        actual=reject
    fi
    if [ "$actual" != "$expected" ]; then
        printf 'Qt %s, OpenSSL %s: expected %s, got %s\n' "$qt_major" "$version" "$expected" "$actual" >&2
        exit 1
    fi
}

version=missing
expect_version_check 5 reject
expect_version_check 6 reject

while read -r version qt5 qt6; do
    printf 'Name: OpenSSL\nDescription: OpenSSL version fixture\nVersion: %s\n' "$version" > "$fixture/openssl.pc"
    expect_version_check 5 "$qt5"
    expect_version_check 6 "$qt6"
done <<'VERSIONS'
1.1.0 reject reject
1.1.1w accept reject
3.0.22 reject accept
4.1.0 reject reject
VERSIONS
