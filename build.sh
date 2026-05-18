#!/usr/bin/env bash
set -e

echo "=================================="
echo " OpenModSim build script (Linux) "
echo "=================================="
echo ""

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$PROJECT_DIR/src"
TOOLS_DIR="$PROJECT_DIR/.build-tools"

# ==========================
# Detect package manager and distro
# ==========================
if [ -f /etc/os-release ]; then
    . /etc/os-release
else
    echo "Cannot detect Linux distribution"
    exit 1
fi

DISTRO=""
CHECK_CMD=""
INSTALL_CMD=""
SEARCH_CMD=""

case "$ID" in
    debian|ubuntu|linuxmint|zorin|astra)
        DISTRO="debian-based"
        CHECK_CMD="dpkg -s"
        INSTALL_CMD="apt install -y"
        SEARCH_CMD="apt-cache search --names-only"
        ;;
    rhel|fedora|rocky|redos)
        DISTRO="rhel-based"
        CHECK_CMD="rpm -q"
        INSTALL_CMD="dnf install -y"
        SEARCH_CMD="dnf list --available"
        ;;
    altlinux)
        DISTRO="altlinux"
        CHECK_CMD="rpm -q"
        INSTALL_CMD="apt-get install -y"
        SEARCH_CMD="apt-cache search --names-only"
        ;;
    suse|opensuse*)
        DISTRO="suse-based"
        CHECK_CMD="rpm -q"
        INSTALL_CMD="zypper install -y"
        SEARCH_CMD="zypper search"
        ;;
    arch|endeavouros|manjaro)
        DISTRO="arch-based"
        CHECK_CMD="pacman -Qq"
        INSTALL_CMD="pacman -S --needed --noconfirm"
        SEARCH_CMD="pacman -Ss"
        ;;
    *)
        echo "Unsupported Linux distribution: $ID"
        exit 1
        ;;
esac

# ==========================
# Check minimum OS version
# ==========================
verlte() {
    # $1 <= $2 ?
    [ "$1" = "$(printf '%s\n%s' "$1" "$2" | sort -V | head -n1)" ]
}
verlt() {
    # $1 < $2 ?
    [ "$1" != "$2" ] && verlte "$1" "$2"
}

get_required_cmake_version() {
    local version
    version=$(grep -oP 'cmake_minimum_required\s*\(\s*VERSION\s+\K[0-9]+(\.[0-9]+){1,3}' "$SOURCE_DIR/CMakeLists.txt" | head -n1)
    if [ -z "$version" ]; then
        echo "Error: Can't detect minimum CMake version from $SOURCE_DIR/CMakeLists.txt" >&2
        exit 1
    fi

    echo "$version"
}

get_cmake_version() {
    local cmake_bin="$1"
    "$cmake_bin" --version 2>/dev/null | head -n1 | grep -oE '[0-9]+(\.[0-9]+){1,3}' || true
}

download_file() {
    local url="$1"
    local output="$2"

    if command -v curl >/dev/null 2>&1; then
        curl -L --fail "$url" -o "$output"
    elif command -v wget >/dev/null 2>&1; then
        wget -O "$output" "$url"
    else
        echo "Error: curl or wget is required to download CMake." >&2
        exit 1
    fi
}

ensure_cmake_version() {
    local required_version="$1"
    local cmake_bin=""
    local cmake_version=""

    if command -v cmake >/dev/null 2>&1; then
        cmake_bin=$(command -v cmake)
        cmake_version=$(get_cmake_version "$cmake_bin")
        if [ -n "$cmake_version" ] && ! verlt "$cmake_version" "$required_version"; then
            echo "Using CMake $cmake_version from: $cmake_bin"
            CMAKE_BIN="$cmake_bin"
            return 0
        fi

        echo "Found CMake ${cmake_version:-unknown} at $cmake_bin, but version $required_version or newer is required."
    else
        echo "CMake not found in PATH."
    fi

    local machine
    local cmake_arch
    machine=$(uname -m)
    case "$machine" in
        x86_64|amd64)
            cmake_arch="x86_64"
            ;;
        aarch64|arm64)
            cmake_arch="aarch64"
            ;;
        *)
            echo "Error: unsupported architecture for Kitware CMake binary: $machine" >&2
            exit 1
            ;;
    esac

    local install_dir="$TOOLS_DIR/cmake-$required_version-linux-$cmake_arch"
    local local_cmake="$install_dir/bin/cmake"
    if [ -x "$local_cmake" ]; then
        cmake_version=$(get_cmake_version "$local_cmake")
        if [ -n "$cmake_version" ] && ! verlt "$cmake_version" "$required_version"; then
            echo "Using local CMake $cmake_version from: $local_cmake"
            CMAKE_BIN="$local_cmake"
            return 0
        fi
    fi

    mkdir -p "$TOOLS_DIR" "$install_dir"
    local installer="$TOOLS_DIR/cmake-$required_version-linux-$cmake_arch.sh"
    local url="https://github.com/Kitware/CMake/releases/download/v$required_version/cmake-$required_version-linux-$cmake_arch.sh"

    echo "Downloading CMake $required_version from Kitware..."
    download_file "$url" "$installer"

    echo "Installing CMake $required_version into $install_dir..."
    sh "$installer" --skip-license --prefix="$install_dir"

    cmake_version=$(get_cmake_version "$local_cmake")
    if [ -z "$cmake_version" ] || verlt "$cmake_version" "$required_version"; then
        echo "Error: downloaded CMake version check failed." >&2
        exit 1
    fi

    echo "Using local CMake $cmake_version from: $local_cmake"
    CMAKE_BIN="$local_cmake"
}

check_min_os_version() {
    local MIN_VERSION="$1"
    if [ -z "$VERSION_ID" ]; then
        echo "Error: Cannot detect OS version."
        exit 1
    fi

    local OS_VER="${VERSION_ID//\"/}"

    if verlt "$OS_VER" "$MIN_VERSION"; then
        echo "Error: $NAME $OS_VER is too old. Minimum required version is $MIN_VERSION." >&2
        exit 1
    fi
}

case "$ID" in
    ubuntu)
        check_min_os_version "22"
        ;;
    linuxmint)
        check_min_os_version "22"
        ;;
    debian)
        check_min_os_version "11"
        ;;
    fedora)
        check_min_os_version "42"
        ;;
    rhel)
        check_min_os_version "8"
        ;;
    rocky)
        check_min_os_version "9.7"
        ;;
    redos)
        check_min_os_version "8"
        ;;
    astra)
        check_min_os_version "1.7.3"
        ;;
    altlinux)
        check_min_os_version "11.0"
        ;;
    suse|opensuse*)
        check_min_os_version "15.5"
        ;;
esac

# ==========================
# Check other requirements
# ==========================
case "$ID" in
    rocky)
        if ! dnf repolist enabled | grep -q "crb"; then
            echo -e "\033[31mError: CRB repository is not enabled in Rocky Linux.\033[0m"
            echo "Please enable it with root privileges:"
            echo "  dnf config-manager --set-enabled crb"
            echo ""
            exit 1
        fi
        ;;
esac

# ==========================
# Parse script arguments
# ==========================
QT_CHOICE=""
USE_QLEMENTINE_APP_STYLE=OFF
for arg in "$@"; do
    case "$arg" in
        -qt5|qt5)
            QT_CHOICE="qt5"
            shift
            ;;
        -qt6|qt6)
            QT_CHOICE="qt6"
            shift
            ;;
        --qlementine|-qlementine)
            USE_QLEMENTINE_APP_STYLE=ON
            shift
            ;;
        *)
            ;;
    esac
done

# ==========================
# Can use sudo?
# ==========================
CAN_SUDO=0
if command -v sudo >/dev/null 2>&1; then
    if sudo -v 2>/dev/null; then
        CAN_SUDO=1
    fi
fi

# ==========================
# Get Qt5 packages
# ==========================
get_qt5_packages() {
    case "$DISTRO" in
        debian-based)
            echo "qtbase5-dev qtbase5-dev-tools qttools5-dev qttools5-dev-tools qtdeclarative5-dev libqt5serialport5-dev libqt5serialbus5-dev"
            ;;
        rhel-based)
            echo "qt5-qtbase-devel qt5-qttools-devel qt5-qtdeclarative-devel qt5-qtserialport-devel qt5-qtserialbus-devel"
            ;;
        altlinux)
            echo "qt5-base-devel qt5-tools-devel qt5-declarative-devel qt5-serialport-devel qt5-serialbus-devel"
            ;;
        suse-based)
            echo "libqt5-qtbase-devel libqt5-qttools-devel libqt5-qttools-qhelpgenerator libqt5-qtdeclarative-devel libqt5-qtserialport-devel libqt5-qtserialbus libqt5-qtserialbus-devel"
            ;;
        arch-based)
            echo "qt5-base qt5-tools qt5-declarative qt5-serialport qt5-serialbus"
            ;;
    esac
}

# ==========================
# Get Qt6 packages
# ==========================
get_qt6_packages() {
    case "$DISTRO" in
        debian-based)
            case "$ID-${VERSION_ID%%.*}" in
                ubuntu-22)
                    echo "qt6-base-dev qt6-base-dev-tools qt6-declarative-dev qt6-tools-dev qt6-tools-dev-tools qt6-l10n-tools libqt6serialport6-dev \
                            libqt6serialbus6-bin libqt6serialbus6-dev libqt6core5compat6-dev qt6-documentation-tools libqt6svg6"
                ;;
                *)
                    echo "qt6-base-dev qt6-base-dev-tools qt6-declarative-dev qt6-tools-dev qt6-tools-dev-tools qt6-serialport-dev qt6-serialbus-dev \
                            qt6-5compat-dev qt6-documentation-tools"
                ;;
            esac
            ;;
        rhel-based)
            echo "qt6-qtbase-devel qt6-qttools-devel qt6-qtdeclarative-devel qt6-qtserialport-devel qt6-qtserialbus-devel qt6-qt5compat-devel"
            ;;
        altlinux)
            echo "qt6-base-devel qt6-tools-devel qt6-declarative-devel qt6-serialport-devel qt6-serialbus-devel qt6-5compat-devel qt6-sql"
            ;;
        suse-based)
            echo "qt6-base-devel qt6-tools-devel qt6-declarative-devel qt6-serialport-devel qt6-serialbus-devel qt6-qt5compat-devel qt6-help-devel qt6-linguist-devel"
            ;;
        arch-based)
            echo "qt6-base qt6-declarative qt6-tools qt6-serialport qt6-serialbus qt6-5compat qt6-svg"
            ;;
    esac
}

# ==========================
# Function to get packages
# Returns combined general + Qt package list
# ==========================
get_packages() {
    local general_packages=""
    local qt_packages=""

    case "$DISTRO" in
        debian-based)
            general_packages="build-essential cmake ninja-build curl libxcb-cursor-dev libgl1-mesa-dev pkg-config libcups2-dev"
            ;;
        rhel-based)
            general_packages="gcc gcc-c++ libstdc++-static cmake ninja-build curl pkgconf-pkg-config xcb-util-cursor-devel"
            ;;
        altlinux)
            general_packages="gcc gcc-c++ libstdc++-devel-static cmake ninja-build curl pkg-config libxcbutil-cursor libcups-devel"
            ;;
        suse-based)
            general_packages="gcc gcc-c++ cmake ninja curl pkg-config libxcb-cursor0"
            ;;
        arch-based)
            general_packages="base-devel cmake ninja curl pkgconf libxcb libcups"
            ;;
    esac

    case "$QT_CHOICE" in
        qt5)
            qt_packages=$(get_qt5_packages) || return 1
            ;;
        qt6)
            qt_packages=$(get_qt6_packages) || return 1
            ;;
    esac

    echo "$general_packages $qt_packages"
}

# ==========================
# Install packages
# ==========================
install_pkg() {
    local pkg_groups=("$@")
    local missing=()

    for group in "${pkg_groups[@]}"; do
        printf "Checking for %-30s... " "$group"
        if $CHECK_CMD "$group" >/dev/null 2>&1; then
            echo "yes"
        else
            echo "no"
            missing+=("$group")
        fi
    done

    if [ ${#missing[@]} -gt 0 ]; then
        echo "Installing missing packages: ${missing[*]}"
        if [ "$EUID" -eq 0 ]; then
            $INSTALL_CMD "${missing[@]}"
        else
            if command -v sudo >/dev/null 2>&1; then
                trap 'echo "Installation cancelled by user."; exit 1' INT
                if [ "$CAN_SUDO" -eq 1 ]; then
                    if sudo $INSTALL_CMD "${missing[@]}"; then
                        :
                    else
                        status=$?
                        err=$(sudo -n true 2>&1)
                        if echo "$err" | grep -Eq "not in the sudoers file|may not run sudo"; then
                            CAN_SUDO=0
                            echo "Using su (user may not run sudo)..."
                            su -c "$INSTALL_CMD ${missing[*]}"
                        else
                            echo "sudo failed with code $status"
                            exit $status
                        fi
                    fi
                else
                    echo "Using su (user may not run sudo)..."
                    su -c "$INSTALL_CMD ${missing[*]}"
                fi
                trap - INT
            else
                echo "Using su (sudo not installed)..."
                su -c "$INSTALL_CMD ${missing[*]}"
            fi
        fi
    fi
}

# ==========================
# Install prerequisites
# ==========================
install_prereqs() {
    local pkgs
    pkgs=$(get_packages) || return 1
    install_pkg $pkgs
}

# ==========================
# Always check/install prereqs first
# ==========================
echo "Checking prerequisites for $ID..."
if [ -z "$QT_CHOICE" ]; then
    if $SEARCH_CMD '^qt6-' 2>/dev/null | head -n30 | grep -q "qt6-"; then
        QT_CHOICE="qt6"
    elif $SEARCH_CMD '^libqt6' 2>/dev/null | head -n30 | grep -q "libqt6"; then
        QT_CHOICE="qt6"
    elif $SEARCH_CMD 'qt6-*' 2>/dev/null | head -n30 | grep -q "qt6-"; then
        QT_CHOICE="qt6"
    else
        QT_CHOICE="qt5"
    fi
fi
install_prereqs
echo ""

# ==========================
# Check CMake version
# ==========================
MIN_CMAKE_VERSION=$(get_required_cmake_version)
ensure_cmake_version "$MIN_CMAKE_VERSION"
echo ""

# ==========================
# Get Qt version string
# ==========================
get_qt_version() {
    local ver=""
    local probes=()

    if [ "$QT_CHOICE" = "qt6" ]; then
        probes=(qmake6 qmake-qt6 qtpaths6 qmake)
    elif [ "$QT_CHOICE" = "qt5" ]; then
        probes=(qmake-qt5 qt5-qmake qtpaths-qt5 qmake)
    fi

    for p in "${probes[@]}"; do
        if command -v "$p" >/dev/null 2>&1; then
            case "$p" in
                qtpaths* )
                    ver=$("$p" --version 2>/dev/null | grep -oE '[0-9]+(\.[0-9]+)+' || true)
                    ;;
                *)
                    ver=$("$p" -query QT_VERSION 2>/dev/null || true)
                    ;;
            esac

            if [ -n "$ver" ]; then
                local major="${ver%%.*}"
                case "$QT_CHOICE" in
                    qt6)
                        if [ "$major" -eq 6 ]; then
                            echo "$ver"
                            return 0
                        fi
                        ;;
                    qt5)
                        if [ "$major" -eq 5 ]; then
                            echo "$ver"
                            return 0
                        fi
                        ;;
                esac
            fi
        fi
    done

    echo "Error: Can't detect installed Qt version." >&2
    exit 1
}

# ==========================
# Get cmake prefix for Qt
# ==========================
get_cmake_prefix() {
    local config_file=""
    local prefix=""

    if [ "$QT_CHOICE" = "qt6" ]; then
        config_file="Qt6CoreConfig.cmake"
    elif [ "$QT_CHOICE" = "qt5" ]; then
        config_file="Qt5CoreConfig.cmake"
    fi

    if command -v pkg-config >/dev/null 2>&1; then
        case "$QT_CHOICE" in
            qt6) prefix=$(pkg-config --variable=prefix Qt6Core 2>/dev/null || true) ;;
            qt5) prefix=$(pkg-config --variable=prefix Qt5Core 2>/dev/null || true) ;;
        esac
        if [ -n "$prefix" ]; then
            echo "$prefix"
            return 0
        fi
    fi

    local probes=()
    if [ "$QT_CHOICE" = "qt6" ]; then
        probes=(qmake6 qmake-qt6 qtpaths6 qmake)
    elif [ "$QT_CHOICE" = "qt5" ]; then
        probes=(qmake-qt5 qt5-qmake qtpaths-qt5 qmake)
    fi

    for q in "${probes[@]}"; do
        if command -v "$q" >/dev/null 2>&1; then
            case "$q" in
                qtpaths* )
                    prefix=$("$q" --install-prefix 2>/dev/null || true)
                    ;;
                *)
                    prefix=$("$q" -query QT_INSTALL_PREFIX 2>/dev/null || true)
                    ;;
            esac
            if [ -n "$prefix" ]; then
                if [ -n "$config_file" ]; then
                    if [ -f "$prefix/lib/cmake/$config_file" ] || [ -f "$prefix/lib64/cmake/$config_file" ]; then
                        echo "$prefix"
                        return 0
                    fi
                else
                    echo "$prefix"
                    return 0
                fi
            fi
        fi
    done

    local found=""
    if [ -n "$config_file" ]; then
        found=$(find /usr /usr/local -type f -name "$config_file" 2>/dev/null | head -n1)
    else
        found=$(find /usr /usr/local -type f \( -name "Qt6CoreConfig.cmake" -o -name "Qt5CoreConfig.cmake" \) 2>/dev/null | head -n1)
    fi

    if [ -n "$found" ]; then
        echo "$(dirname "$found")/.."
        return 0
    fi

    echo "Error: Can't detect cmake prefix path for $REQ." >&2
    exit 1
}

# ==========================
# Detect Qt version and cmake prefix
# ==========================
QT_VERSION=$(get_qt_version)
CMAKE_PREFIX=$(get_cmake_prefix)

# ==========================
# Check minimal Qt version
# ==========================
MIN_QT_VERSION="5.15.0"
if verlt "$QT_VERSION" "$MIN_QT_VERSION"; then
    echo "Error: Qt >= $MIN_QT_VERSION is required, but found $QT_VERSION" >&2
    exit 1
else
    echo "Using Qt $QT_VERSION from: $CMAKE_PREFIX"
fi

# ==========================
# Detect architecture
# ==========================
ARCH=$(uname -m)

# ==========================
# Detect compiler
# ==========================
select_max_gpp() {
    local candidates max_ver="" max_bin=""
    candidates=$(compgen -c g++ | sort -u)

    for bin in $candidates; do
        if command -v "$bin" >/dev/null 2>&1; then
            ver=$("$bin" -dumpfullversion -dumpversion 2>/dev/null | head -n1)
            if [[ -n "$ver" ]]; then
                if [[ -z "$max_ver" ]] || verlt "$max_ver" "$ver"; then
                    max_ver=$ver
                    max_bin=$bin
                fi
            fi
        fi
    done

    if [[ -z "$max_bin" ]]; then
        echo "Error: Can't find g++ compiller" >&2
        exit 1
    fi

    echo "$max_bin"
}
CXX_COMPILER=$(select_max_gpp)

# ==========================
# Build type
# ==========================
BUILD_TYPE=Release

# ==========================
# Setup cmake options
# ==========================
CMAKE_QT_OPTION="-DUSE_QT5=OFF -DUSE_QT6=OFF"
if [ "$QT_CHOICE" = "qt5" ]; then
    CMAKE_QT_OPTION="-DUSE_QT5=ON"
elif [ "$QT_CHOICE" = "qt6" ]; then
    CMAKE_QT_OPTION="-DUSE_QT6=ON"
fi

# ==========================
# Build project
# ==========================
SANITIZED_QT_VERSION=$(echo "$QT_VERSION" | tr '.' '_' | tr ' ' '_')
BUILD_DIR="build-omodsim-Qt_${SANITIZED_QT_VERSION}_g++_${ARCH}-${BUILD_TYPE}"
echo "Starting build in: $BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
"$CMAKE_BIN" "$SOURCE_DIR" -GNinja -DCMAKE_PREFIX_PATH="${CMAKE_PREFIX}" \
      -DCMAKE_CXX_COMPILER=${CXX_COMPILER} \
      -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
      -DUSE_QLEMENTINE_APP_STYLE=${USE_QLEMENTINE_APP_STYLE} \
      ${CMAKE_QT_OPTION}
ninja
echo "Build finished successfully in $BUILD_DIR."
echo ""

NINJA_VER=$(ninja --version)
if verlt "1.11.0" "$NINJA_VER"; then
    echo "To install or uninstall Open ModSim, run:"
    echo ""
    echo -e "    cd $BUILD_DIR"
    echo ""
    if [ "$EUID" -eq 0 ]; then
        echo -e "    ninja install"
        echo -e "    ninja uninstall"
    else
        if [ "$CAN_SUDO" -eq 1 ]; then
            echo -e "    sudo ninja install"
            echo -e "    sudo ninja uninstall"
        else
            echo -e "    su -c 'ninja install'"
            echo -e "    su -c 'ninja uninstall'"
        fi
    fi
    echo ""
fi
