"""
pytest configuration for ModbusTcpServer tests.

- Prints ✓ / ✗ / ○ with color on the LEFT of each test name.
- Tracks Modbus function test failures so the stress test is skipped
  automatically if any of them failed.
"""

import pytest

# ── Color codes ───────────────────────────────────────────────────────────────

_GREEN  = "\033[32m"
_RED    = "\033[31m"
_YELLOW = "\033[33m"
_GRAY   = "\033[90m"
_RESET  = "\033[0m"

# ── Terminal writer (set in pytest_configure) ─────────────────────────────────

_tw = None
_current_fspath: str | None = None


def pytest_sessionstart(session):
    """Get the terminal writer after all plugins (including terminalreporter) are registered."""
    global _tw
    _tw = session.config.get_terminal_writer()


@pytest.hookimpl(trylast=True)
def pytest_runtest_logstart(nodeid, location):
    """Add a newline after the filename header so the first test starts on its own line."""
    global _current_fspath
    fspath = nodeid.split("::")[0]
    if fspath != _current_fspath:
        _current_fspath = fspath
        if _tw is not None:
            _tw.write("\n")


# ── Per-test symbol output ────────────────────────────────────────────────────

def pytest_report_teststatus(report, config):
    """Suppress default dots and PASSED/FAILED words.
    pytest_runtest_logreport prints our own formatted lines instead."""
    if report.when in ("call",) or (report.when == "setup" and report.skipped):
        return report.outcome, "", ""


@pytest.hookimpl(trylast=True)
def pytest_runtest_logreport(report):
    """Print  ✓ / ✗ / ○  with test name after each test completes."""
    if _tw is None:
        return

    name = report.nodeid.split("::")[-1]

    if report.when == "call":
        if report.passed:
            _tw.write(f"  {_GREEN}✓{_RESET}  {name}\n")
        elif report.failed:
            _tw.write(f"  {_RED}✗{_RESET}  {name}\n")
        elif report.skipped:                         # xfail inside test body
            reason = _skip_reason(report)
            _tw.write(f"  {_YELLOW}○{_RESET}  {name}  {_GRAY}({reason}){_RESET}\n")

    elif report.when == "setup" and report.skipped:  # pytest.skip() or marker
        reason = _skip_reason(report)
        _tw.write(f"  {_YELLOW}○{_RESET}  {name}  {_GRAY}({reason}){_RESET}\n")


def _skip_reason(report) -> str:
    """Extract a short skip/xfail reason from the report."""
    if hasattr(report, "wasxfail"):
        return report.wasxfail
    if isinstance(report.longrepr, tuple) and len(report.longrepr) >= 3:
        return str(report.longrepr[2]).replace("Skipped: ", "").strip()
    return str(report.longrepr).strip()


# ── FC test failure tracking ──────────────────────────────────────────────────

_NON_FC_TESTS = {"test_stress", "test_concurrent_connections"}

_failed_fc_tests: list[str] = []


@pytest.hookimpl(hookwrapper=True)
def pytest_runtest_makereport(item, call):
    outcome = yield
    report = outcome.get_result()
    if report.when == "call" and report.failed:
        if item.name not in _NON_FC_TESTS:
            _failed_fc_tests.append(item.name)


@pytest.fixture
def require_fc_tests_passed():
    """Skip the test if any Modbus function test failed earlier in the session."""
    if _failed_fc_tests:
        pytest.skip(
            f"{len(_failed_fc_tests)} Modbus function test(s) failed -- "
            "stress test skipped: " + ", ".join(_failed_fc_tests)
        )
