#!/usr/bin/env python3
"""Cross-platform test-coverage runner for Open ModSim.

Configures a dedicated coverage build, runs the CTest suite and produces a
uniform report on any OS. The instrumentation backend is chosen automatically
from the compiler:

    * GCC / Clang  ->  gcovr            (pip install gcovr)
    * MSVC         ->  OpenCppCoverage  (winget install OpenCppCoverage)

Output (under <build-dir>/coverage/):
    coverage.xml    Cobertura report (CI-friendly)
    index.html      Browsable HTML report
and a one-line "Line coverage: NN.N%" summary on the console.

Examples
--------
    python tools/coverage.py
    python tools/coverage.py --open
    # Forward extra options to the CMake configure step (e.g. Qt location):
    python tools/coverage.py -- -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import webbrowser
import xml.etree.ElementTree as ET
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC_DIR = REPO_ROOT / "src"

# Qt GUI tests run headless in CI and on build servers without a display.
CTEST_ENV = {**os.environ, "QT_QPA_PLATFORM": "offscreen"}


def run(cmd: list[str], check: bool = True, **kwargs) -> subprocess.CompletedProcess:
    print(">>", " ".join(str(c) for c in cmd), flush=True)
    return subprocess.run([str(c) for c in cmd], check=check, **kwargs)


def tool_path(name: str) -> str | None:
    return shutil.which(name)


def read_cache(build_dir: Path, key: str) -> str | None:
    cache = build_dir / "CMakeCache.txt"
    if not cache.exists():
        return None
    for line in cache.read_text(encoding="utf-8", errors="replace").splitlines():
        if line.startswith(f"{key}:"):
            return line.split("=", 1)[1].strip() if "=" in line else None
    return None


def detect_compiler(build_dir: Path) -> str:
    """Return 'msvc', 'clang' or 'gcc' based on the configured compiler."""
    compiler = (read_cache(build_dir, "CMAKE_CXX_COMPILER") or "").lower()
    base = Path(compiler).name
    if base in ("cl.exe", "cl") or "clang-cl" in base:
        return "msvc"
    if "clang" in base:
        return "clang"
    return "gcc"


def configure(build_dir: Path, config: str, extra: list[str]) -> None:
    args = [
        "cmake",
        "-S", SRC_DIR,
        "-B", build_dir,
        "-DOMODSIM_ENABLE_COVERAGE=ON",
        f"-DCMAKE_BUILD_TYPE={config}",
        # Coverage and LTO are incompatible; force it off for this build.
        "-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF",
        *extra,
    ]
    run(args)


def build(build_dir: Path, config: str) -> None:
    run(["cmake", "--build", build_dir, "--config", config, "--parallel"])


def report_gcov(build_dir: Path, out_dir: Path, compiler: str) -> Path:
    if not tool_path("gcovr"):
        sys.exit("error: 'gcovr' not found. Install it with: pip install gcovr")
    xml = out_dir / "coverage.xml"
    cmd = [
        "gcovr",
        "--root", SRC_DIR,
        str(build_dir),
        "--exclude", r".*/tests/.*",
        "--exclude", r".*/build.*",
        "--print-summary",
        "--cobertura", xml,
        "--cobertura-pretty",
        "--html-details", out_dir / "index.html",
    ]
    if compiler == "clang":
        # Apple's clang ships llvm-cov only behind xcrun, not on PATH.
        llvm_cov = "llvm-cov gcov" if tool_path("llvm-cov") else "xcrun llvm-cov gcov"
        cmd += ["--gcov-executable", llvm_cov]
    run(cmd)
    return xml


def report_msvc(build_dir: Path, out_dir: Path, config: str) -> tuple[Path, int]:
    occ = tool_path("OpenCppCoverage") or tool_path("OpenCppCoverage.exe")
    if not occ:
        sys.exit("error: 'OpenCppCoverage' not found. Install it with: "
                 "winget install OpenCppCoverage")
    xml = out_dir / "coverage.xml"
    cmd = [
        occ,
        "--quiet",
        "--cover_children",
        "--sources", SRC_DIR,
        "--excluded_sources", SRC_DIR / "tests",
        "--export_type", f"cobertura:{xml}",
        "--export_type", f"html:{out_dir}",
        "--",
        "ctest", "--test-dir", build_dir, "-C", config, "--output-on-failure",
    ]
    # Don't raise on a failing suite: surface the exit code but keep the report.
    rc = run(cmd, check=False, env=CTEST_ENV).returncode
    return xml, rc


def print_summary(xml: Path) -> None:
    if not xml.exists():
        sys.exit(f"error: expected report not produced: {xml}")
    root = ET.parse(xml).getroot()
    rate = root.get("line-rate")
    covered = root.get("lines-covered")
    valid = root.get("lines-valid")
    print("\n" + "=" * 48)
    if rate is not None:
        line = f"Line coverage: {float(rate) * 100:.1f}%"
        if covered and valid:
            line += f"  ({covered}/{valid} lines)"
        print(line)
    branch = root.get("branch-rate")
    if branch is not None and branch != "0":
        print(f"Branch coverage: {float(branch) * 100:.1f}%")
    print(f"Report: {xml.parent / 'index.html'}")
    print("=" * 48)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Cross-platform test-coverage runner.",
        epilog="Pass extra CMake configure args after a literal '--'.",
    )
    parser.add_argument("--build-dir", type=Path,
                        default=REPO_ROOT / "build-coverage",
                        help="build directory (default: ./build-coverage)")
    parser.add_argument("--config", default="RelWithDebInfo",
                        help="build configuration (default: RelWithDebInfo). "
                             "Coverage instrumentation forces -O0/-Od + debug "
                             "info on the test targets regardless, so a release "
                             "config keeps Qt's debug/release plugin loading "
                             "happy while line mapping stays accurate.")
    parser.add_argument("--open", action="store_true",
                        help="open the HTML report when finished")
    parser.add_argument("--skip-configure", action="store_true",
                        help="reuse an existing configured build directory")
    parser.add_argument("--strict", action="store_true",
                        help="exit non-zero if any test failed (for CI)")
    parser.add_argument("cmake_args", nargs="*",
                        help="extra args forwarded to CMake configure")
    args = parser.parse_args()

    # argparse keeps the leading '--' separator in the remainder; drop it.
    extra = [a for a in args.cmake_args if a != "--"]

    build_dir: Path = args.build_dir
    out_dir = build_dir / "coverage"
    out_dir.mkdir(parents=True, exist_ok=True)

    if not args.skip_configure:
        configure(build_dir, args.config, extra)
    build(build_dir, args.config)

    compiler = detect_compiler(build_dir)
    print(f"Detected compiler: {compiler}")

    if compiler == "msvc":
        # OpenCppCoverage runs the tests itself (wrapping ctest).
        xml, test_rc = report_msvc(build_dir, out_dir, args.config)
    else:
        # gcov: run the tests first, then collect the .gcda counters.
        test_rc = run(["ctest", "--test-dir", build_dir, "-C", args.config,
                       "--output-on-failure"], check=False, env=CTEST_ENV).returncode
        xml = report_gcov(build_dir, out_dir, compiler)

    print_summary(xml)

    if test_rc != 0:
        print("WARNING: some tests failed; coverage reflects this run anyway.")

    if args.open:
        webbrowser.open((out_dir / "index.html").as_uri())

    return 1 if (test_rc != 0 and args.strict) else 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except subprocess.CalledProcessError as exc:
        sys.exit(f"error: command failed with exit code {exc.returncode}")
