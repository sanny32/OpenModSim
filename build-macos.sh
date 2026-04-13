#!/usr/bin/env bash
set -e

echo "=================================="
echo " OpenModSim build script (macOS)  "
echo "=================================="
echo ""

# ==========================
# Check macOS
# ==========================
if [[ "$(uname)" != "Darwin" ]]; then
    echo "Error: This script is for macOS only."
    exit 1
fi

# ==========================
# Parse script arguments
# ==========================
QT_CHOICE=""
for arg in "$@"; do
    case "$arg" in
        -qt5|qt5)
            QT_CHOICE="qt5"
            ;;
        -qt6|qt6)
            QT_CHOICE="qt6"
            ;;
        *)
            ;;
    esac
done

if [ -z "$QT_CHOICE" ]; then
    QT_CHOICE="qt6"
fi

# ==========================
# Check Xcode Command Line Tools
# ==========================
echo "Checking for Xcode Command Line Tools..."
if ! xcode-select -p >/dev/null 2>&1; then
    echo "Error: Xcode Command Line Tools not found."
    echo "Install with: xcode-select --install"
    exit 1
fi
echo "  Found: $(xcode-select -p)"

# ==========================
# Check Homebrew
# ==========================
echo "Checking for Homebrew..."
if ! command -v brew >/dev/null 2>&1; then
    echo "Error: Homebrew not found."
    echo "Install from: https://brew.sh"
    exit 1
fi
echo "  Found: $(brew --prefix)"

# ==========================
# Check CMake and Ninja
# ==========================
echo "Checking for CMake..."
if ! command -v cmake >/dev/null 2>&1; then
    echo "CMake not found. Installing..."
    brew install cmake
fi
echo "  Found: $(cmake --version | head -1)"

echo "Checking for Ninja..."
if ! command -v ninja >/dev/null 2>&1; then
    echo "Ninja not found. Installing..."
    brew install ninja
fi
echo "  Found: ninja $(ninja --version)"

# ==========================
# Check Qt
# ==========================
echo "Checking for Qt..."

QT_PREFIX=""
QT_VERSION=""

if [ "$QT_CHOICE" = "qt6" ]; then
    QT_PREFIX="$(brew --prefix qt@6 2>/dev/null || brew --prefix qt 2>/dev/null || true)"
    if [ -z "$QT_PREFIX" ] || [ ! -d "$QT_PREFIX" ]; then
        echo "Qt6 not found. Installing..."
        brew install qt@6
        QT_PREFIX="$(brew --prefix qt@6 2>/dev/null || brew --prefix qt 2>/dev/null)"
    fi
    QT_VERSION=$("${QT_PREFIX}/bin/qmake" -query QT_VERSION 2>/dev/null || true)
elif [ "$QT_CHOICE" = "qt5" ]; then
    QT_PREFIX="$(brew --prefix qt@5 2>/dev/null || true)"
    if [ -z "$QT_PREFIX" ] || [ ! -d "$QT_PREFIX" ]; then
        echo "Qt5 not found. Installing..."
        brew install qt@5
        QT_PREFIX="$(brew --prefix qt@5 2>/dev/null)"
    fi
    QT_VERSION=$("${QT_PREFIX}/bin/qmake" -query QT_VERSION 2>/dev/null || true)
fi

if [ -z "$QT_VERSION" ]; then
    echo "Error: Cannot detect Qt version from ${QT_PREFIX}"
    exit 1
fi

echo "  Found: Qt ${QT_VERSION} at ${QT_PREFIX}"

# ==========================
# Check minimum Qt version
# ==========================
MIN_QT_VERSION="5.15.0"
verlte() {
    [ "$1" = "$(printf '%s\n%s' "$1" "$2" | sort -V | head -n1)" ]
}
verlt() {
    [ "$1" != "$2" ] && verlte "$1" "$2"
}

if verlt "$QT_VERSION" "$MIN_QT_VERSION"; then
    echo "Error: Qt >= $MIN_QT_VERSION is required, but found $QT_VERSION"
    exit 1
fi

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
# Detect architecture
# ==========================
ARCH=$(uname -m)
BUILD_TYPE=Release

# ==========================
# Build project
# ==========================
SANITIZED_QT_VERSION=$(echo "$QT_VERSION" | tr '.' '_')
BUILD_DIR="build-omodsim-Qt_${SANITIZED_QT_VERSION}_clang_${ARCH}-${BUILD_TYPE}"
echo ""
echo "Starting build in: ${BUILD_DIR}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake ../src -GNinja \
    -DCMAKE_PREFIX_PATH="${QT_PREFIX}" \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    ${CMAKE_QT_OPTION}

ninja

echo ""
echo "Build finished successfully!"
echo "Application bundle: ${BUILD_DIR}/omodsim.app"
echo ""
echo "To run:"
echo "    open ${BUILD_DIR}/omodsim.app"
echo ""
