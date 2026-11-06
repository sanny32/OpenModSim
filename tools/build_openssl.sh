#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 2 ] || [ "$#" -gt 3 ]; then
    echo "Usage: $0 <openssl-source-dir> <install-prefix> [3|1.1]" >&2
    exit 1
fi

source_root="$1"
install_prefix="$2"
openssl_abi="${3:-3}"
if [ -e "$install_prefix/lib/libssl.so.$openssl_abi" ] && [ -e "$install_prefix/lib/libcrypto.so.$openssl_abi" ]; then
    exit 0
fi
if [ ! -f "$source_root/Configure" ]; then
    echo "No OpenSSL Configure script in $source_root" >&2
    exit 1
fi
(
    cd "$source_root"
    ./config --prefix="$install_prefix" --libdir=lib --openssldir=/etc/ssl shared no-tests
    make -j"$(nproc)"
    make install_sw
)
