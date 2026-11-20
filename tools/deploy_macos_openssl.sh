#!/usr/bin/env bash
set -euo pipefail

app="$1"
crypto="$2"
ssl="$3"
frameworks="$app/Contents/Frameworks"
mkdir -p "$frameworks"

for library in "$crypto" "$ssl"; do
    name=$(basename "$library")
    [[ "$name" == *.dylib ]] || { echo "Expected an OpenSSL dylib: $library" >&2; exit 1; }
    cp -L "$library" "$frameworks/$name"
    chmod u+w "$frameworks/$name"
    install_name_tool -id "@rpath/$name" "$frameworks/$name"
done

crypto_name=$(basename "$crypto")
ssl_name=$(basename "$ssl")
while read -r dependency _; do
    if [[ "$dependency" == */libcrypto*.dylib ]]; then
        install_name_tool -change "$dependency" "@loader_path/$crypto_name" "$frameworks/$ssl_name"
    fi
done < <(otool -L "$frameworks/$ssl_name" | tail -n +2)

codesign --force --deep --sign - "$app"
codesign --verify --deep --strict "$app"
