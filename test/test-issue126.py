#!/usr/bin/env python3
"""
Regression tests for issue #126 (script freeze / stalled input registers).

Install dependencies:
    pip install -r requirements.txt

Usage:
    pytest test-issue126.py -v

Prerequisites (omodsim must be running):
    Load test/test-issue126.omsim in omodsim before running. It configures:
    - TCP server on 0.0.0.0:5020 (unprivileged, no root needed)
    - Slave ID 1
    - Holding registers: protocol addresses 0-299
    - Input registers:   protocol addresses 0-299
    - A script view running test/scripts/test_issue126.js in Once mode
      with RunOnStartup enabled

What is checked:
    1. The initial bulk fill from Script.onInit() actually reached the server,
       i.e. the script did not hang partway through.
    2. Input registers keep updating after init, i.e. the Script.setTimeout()
       chain survives Once mode (this is what regressed in 2.0).
    3. The server stays responsive while the script runs.
"""

import time

import pytest

ModbusTcpClient = pytest.importorskip("pymodbus.client").ModbusTcpClient

# -- Configuration ------------------------------------------------------------

HOST      = "127.0.0.1"
PORT      = 5020
DEVICE_ID = 1
TIMEOUT   = 3  # seconds per request

REGISTER_COUNT = 300

# Script register layout (see test/scripts/test_issue126.js)
REGISTER_HEARTBEAT = 0
REGISTER_SECONDS   = 1
REGISTER_TICKS     = 2

# The script ticks once per second; allow generous slack for slow machines.
TICK_WAIT_SECONDS = 4.0

# A responsive server answers well inside this budget.
RESPONSIVENESS_BUDGET_SECONDS = 1.0


# -- Helpers ------------------------------------------------------------------

def make_client() -> ModbusTcpClient:
    """Connect a client, skipping the test when no omodsim server is reachable."""
    c = ModbusTcpClient(host=HOST, port=PORT, timeout=TIMEOUT)
    if not c.connect():
        pytest.skip(
            f"omodsim TCP server not reachable at {HOST}:{PORT}; "
            "load test-issue126.omsim in omodsim to run this test."
        )
    return c


def ok(rr) -> bool:
    """Return True if the response is not None and contains no Modbus error."""
    return rr is not None and not rr.isError()


def read_input(c, address, count=1):
    rr = c.read_input_registers(address=address, count=count, device_id=DEVICE_ID)
    assert ok(rr), f"failed to read input registers at {address}: {rr}"
    return rr.registers


# -- Initial fill -------------------------------------------------------------

def test_init_fill_completed_holdings():
    """Script.onInit() wrote the whole holding block before yielding."""
    with make_client() as c:
        rr = c.read_holding_registers(address=0, count=125, device_id=DEVICE_ID)
        assert ok(rr), f"failed to read holding registers: {rr}"

        # test_issue126.js writes holdings[i] = i
        assert rr.registers[0] == 0
        assert rr.registers[124] == 124


def test_init_fill_completed_inputs():
    """The tail of the input block proves the fill did not stop halfway."""
    with make_client() as c:
        # test_issue126.js writes inputs[i] = REGISTER_COUNT - i, so the last
        # register is the strongest evidence the whole batch was applied.
        last = REGISTER_COUNT - 1
        values = read_input(c, last - 1, 2)
        assert values[1] == REGISTER_COUNT - last


# -- The setTimeout chain -----------------------------------------------------

def test_input_registers_keep_updating():
    """Regression: the Script.setTimeout() chain must survive Once mode."""
    with make_client() as c:
        first = read_input(c, REGISTER_TICKS)[0]
        time.sleep(TICK_WAIT_SECONDS)
        second = read_input(c, REGISTER_TICKS)[0]

        assert second != first, (
            "tick counter did not advance: the setTimeout chain stopped "
            "(script was torn down after the first evaluate)"
        )


def test_heartbeat_toggles():
    """The heartbeat register alternates, so writes reach the server each tick."""
    with make_client() as c:
        seen = set()
        deadline = time.monotonic() + TICK_WAIT_SECONDS * 2
        while time.monotonic() < deadline and len(seen) < 2:
            seen.add(read_input(c, REGISTER_HEARTBEAT)[0])
            time.sleep(0.25)

        assert seen == {0, 1}, f"heartbeat did not toggle, observed {sorted(seen)}"


# -- Responsiveness -----------------------------------------------------------

def test_server_stays_responsive_while_script_runs():
    """Regression: the UI thread must not block the Modbus worker."""
    with make_client() as c:
        for _ in range(20):
            started = time.monotonic()
            rr = c.read_holding_registers(address=0, count=100, device_id=DEVICE_ID)
            elapsed = time.monotonic() - started

            assert ok(rr), f"request failed while the script was running: {rr}"
            assert elapsed < RESPONSIVENESS_BUDGET_SECONDS, (
                f"response took {elapsed:.2f}s, over the "
                f"{RESPONSIVENESS_BUDGET_SECONDS}s budget"
            )


def test_writes_from_client_still_apply():
    """Client writes are not starved by the running script."""
    with make_client() as c:
        rr = c.write_register(address=250, value=4242, device_id=DEVICE_ID)
        assert ok(rr), f"write failed: {rr}"

        rr = c.read_holding_registers(address=250, count=1, device_id=DEVICE_ID)
        assert ok(rr), f"read-back failed: {rr}"
        assert rr.registers[0] == 4242
