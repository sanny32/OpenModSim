"""
pytest configuration for ModbusTcpServer tests.

Tracks Modbus function test failures so that the stress test can be
skipped automatically if any of them failed.
"""

import pytest

# Names of tests that are NOT Modbus function tests (excluded from tracking)
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
