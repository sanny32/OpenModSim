#!/usr/bin/env bash
set -eu

script="$1"
fixture=$(mktemp -d)
trap 'rm -rf "$fixture"' EXIT
mkdir -p "$fixture/bin" "$fixture/qt/bin"
cat > "$fixture/bin/mock" <<'MOCK'
#!/usr/bin/env bash
set -eu
case "${0##*/}" in
    uname) echo Darwin ;;
    xcode-select|actool) echo /mock/xcode ;;
    ninja) echo ninja ;;
    qmake) echo 6.12.0 ;;
    brew)
        echo "$*" >> "$FIXTURE/brew.log"
        case "$*" in
            '--prefix qt@6') echo "$FIXTURE/qt" ;;
            '--prefix openssl'*) echo "/mock/$2" ;;
            '--prefix') echo /mock/brew ;;
            'list --versions openssl'*) exit 1 ;;
            'install openssl'*) exit "$INSTALL_STATUS" ;;
            *) exit 1 ;;
        esac
        ;;
    cmake)
        if [ "${1:-}" = --version ]; then
            echo 'cmake version 3.30'
            exit
        fi
        echo "$*|${OPENSSL_ROOT_DIR:-}" >> "$FIXTURE/cmake.log"
        for arg in "$@"; do
            case "$arg" in
                -DOMODSIM_OPENSSL_PROBE=*) printf '%s' "$PROBE_RESULT" > "${arg#*=}" ;;
            esac
        done
        ;;
esac
MOCK
chmod +x "$fixture/bin/mock"
for command in uname xcode-select actool ninja cmake brew; do
    ln -s mock "$fixture/bin/$command"
done
ln -s "$fixture/bin/mock" "$fixture/qt/bin/qmake"
ln -s "$fixture/bin/mock" "$fixture/qt/bin/qhelpgenerator"
export FIXTURE="$fixture" PATH="$fixture/bin:$PATH"
unset OPENSSL_ROOT_DIR

for result in 3 4 "" found; do
    export PROBE_RESULT="$result" INSTALL_STATUS=0
    : > "$fixture/brew.log"
    : > "$fixture/cmake.log"
    (cd "$fixture" && bash "$script") > "$fixture/output" 2>&1
    if [ "$result" = found ]; then
        if grep -q openssl "$fixture/brew.log"; then
            cat "$fixture/brew.log"
            exit 1
        fi
    else
        formula=openssl
        if [ -n "$result" ]; then
            formula="openssl@$result"
        fi
        grep -qx "install $formula" "$fixture/brew.log"
        tail -1 "$fixture/cmake.log" | grep -q "|/mock/$formula$"
    fi
    tail -1 "$fixture/cmake.log" | grep -q -- '-UOMODSIM_OPENSSL_PROBE'
done

export PROBE_RESULT=4 INSTALL_STATUS=1
: > "$fixture/cmake.log"
if (cd "$fixture" && bash "$script") > "$fixture/output" 2>&1; then
    exit 1
fi
[ "$(wc -l < "$fixture/cmake.log" | tr -d ' ')" = 1 ]
