#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <openssl-source-dir> <install-prefix>" >&2
    exit 1
fi

source_root="$1"
install_prefix="$2"
if [ -e "$install_prefix/lib/libssl.so.3" ] && [ -e "$install_prefix/lib/libcrypto.so.3" ]; then
    exit 0
fi
if [ ! -f "$source_root/Configure" ]; then
    echo "No OpenSSL Configure script in $source_root" >&2
    exit 1
fi
(
    cd "$source_root"
    ./Configure --prefix="$install_prefix" --libdir=lib --openssldir=/etc/ssl shared no-tests
    make -j"$(nproc)"
    make install_sw
)
