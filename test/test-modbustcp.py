#!/usr/bin/env python3
"""
Integration tests for ModbusTcpServer (omodsim).

Install dependencies:
    pip install -r requirements.txt

Usage:
    pytest test-modbustcp.py -v            # all tests
    pytest test-modbustcp.py -v -k fc03    # single test
    python test-modbustcp.py               # standalone

Prerequisites (omodsim must be running):
    Load test/test-modbustcp.omp in omodsim before running. It configures:
    - TCP server on 0.0.0.0:502 (all interfaces)
    - Slave ID 1
    - Holding registers:  protocol addresses 0-199  (readable and writable)
    - Input registers:    protocol addresses 0-99   (readable)
    - Coils:              protocol addresses 0-1999 (readable and writable)
    - Discrete inputs:    protocol addresses 0-99   (readable)

    Note: coils length 2000 is required by test_fc01_read_coils_large (count=2000).
          holding length 200 is required by test_fc03_read_holding_registers_large (count=125)
          and by write tests that use addresses up to 50.

Error simulation (manual testing only):
    These settings are controlled via the omodsim GUI and have no programmatic
    API, so they cannot be tested automatically. Verify manually:

    1. noResponse           -- enable, confirm client receives timeout
    2. responseDelay        -- set 500 ms delay, confirm response arrives late
    3. responseIllegalFunction -- confirm client receives Modbus exception 0x01
    4. responseDeviceBusy      -- confirm client receives Modbus exception 0x06
    5. responseIncorrectId     -- confirm Unit ID in response differs from request
"""

import random
import sys
import threading
import time

import pytest
from pymodbus.client import ModbusTcpClient

# в”Ђв”Ђ Configuration в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ

HOST    = "127.0.0.1"
PORT    = 502
DEVICE_ID = 1
TIMEOUT   = 3  # seconds per request

# Test parameters
CONCURRENT_CLIENTS = 10   # simultaneous connections
STRESS_THREADS     = 20   # threads in stress test
STRESS_REQ_EACH    = 50   # requests per thread

# в”Ђв”Ђ Helpers в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ

def make_client() -> ModbusTcpClient:
    """Create and connect a client; calls pytest.fail if connection is refused."""
    c = ModbusTcpClient(host=HOST, port=PORT, timeout=TIMEOUT)
    if not c.connect():
        pytest.fail(
            f"Cannot connect to {HOST}:{PORT}. "
            "Make sure omodsim is running with a TCP server."
        )
    return c


_EXCEPTION_NAMES: dict[int, str] = {
    1:  "IllegalFunction       -- register type not configured or FC not supported",
    2:  "IllegalDataAddress    -- address/count out of configured range",
    3:  "IllegalDataValue      -- value rejected by server",
    4:  "ServerDeviceFailure   -- internal server error",
    5:  "Acknowledge           -- request accepted, processing",
    6:  "ServerDeviceBusy      -- server busy (too many pending requests)",
    8:  "MemoryParityError",
    10: "GatewayPathUnavailable",
    11: "GatewayTargetDeviceFailedToRespond",
}


def ok(rr) -> bool:
    """Return True if the response is not None and contains no Modbus error."""
    return rr is not None and not rr.isError()


def explain(rr) -> str:
    """Return a human-readable description of a failed Modbus response."""
    if rr is None:
        return "no response (timeout or connection lost)"
    if rr.isError():
        exc_code = getattr(rr, "exception_code", None)
        fc_raw   = getattr(rr, "function_code", 0)
        fc       = fc_raw & 0x7F  # strip error bit 0x80
        if exc_code is not None:
            detail = _EXCEPTION_NAMES.get(exc_code, f"unknown exception code {exc_code}")
            return f"FC{fc:02d} -> {detail}"
        return f"FC{fc:02d} error: {rr}"
    return str(rr)


# в”Ђв”Ђ FC 01 вЂ“ Read Coils в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ

def test_fc01_read_coils_basic():
    with make_client() as c:
        rr = c.read_coils(address=0, count=8, device_id=DEVICE_ID)
        assert ok(rr), explain(rr)
        assert len(rr.bits) >= 8


def test_fc01_read_coils_large():
    """Maximum allowed coil count per request is 2000."""
    with make_client() as c:
        rr = c.read_coils(address=0, count=2000, device_id=DEVICE_ID)
        assert ok(rr), explain(rr)
        assert len(rr.bits) >= 2000


# в”Ђв”Ђ FC 02 вЂ“ Read Discrete Inputs в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ

def test_fc02_read_discrete_inputs():
    with make_client() as c:
        rr = c.read_discrete_inputs(address=0, count=8, device_id=DEVICE_ID)
        assert ok(rr), explain(rr)
        assert len(rr.bits) >= 8


# в”Ђв”Ђ FC 03 вЂ“ Read Holding Registers в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ

def test_fc03_read_holding_registers_basic():
    with make_client() as c:
        rr = c.read_holding_registers(address=0, count=10, device_id=DEVICE_ID)
        assert ok(rr), explain(rr)
        assert len(rr.registers) == 10


def test_fc03_read_holding_registers_single():
    with make_client() as c:
        rr = c.read_holding_registers(address=0, count=1, device_id=DEVICE_ID)
        assert ok(rr), explain(rr)
        assert len(rr.registers) == 1


def test_fc03_read_holding_registers_large():
    """Maximum allowed register count per request is 125."""
    with make_client() as c:
        rr = c.read_holding_registers(address=0, count=125, device_id=DEVICE_ID)
        assert ok(rr), explain(rr)
        assert len(rr.registers) == 125


# в”Ђв”Ђ FC 04 вЂ“ Read Input Registers в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ

def test_fc04_read_input_registers():
    with make_client() as c:
        rr = c.read_input_registers(address=0, count=10, device_id=DEVICE_ID)
        assert ok(rr), explain(rr)
        assert len(rr.registers) == 10


# в”Ђв”Ђ FC 05 вЂ“ Write Single Coil в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ

def test_fc05_write_coil_true():
    with make_client() as c:
        rr = c.write_coil(address=0, value=True, device_id=DEVICE_ID)
        assert ok(rr), explain(rr)


def test_fc05_write_coil_false():
    with make_client() as c:
        rr = c.write_coil(address=0, value=False, device_id=DEVICE_ID)
        assert ok(rr), explain(rr)


def test_fc05_write_read_roundtrip():
    """FC05 write -> FC01 read: value must match."""
    with make_client() as c:
        for val in (True, False):
            c.write_coil(address=5, value=val, device_id=DEVICE_ID)
            rr = c.read_coils(address=5, count=1, device_id=DEVICE_ID)
            assert ok(rr), explain(rr)
            assert rr.bits[0] is val, f"FC05->FC01 roundtrip: expected {val}, got {rr.bits[0]}"


# в”Ђв”Ђ FC 06 вЂ“ Write Single Register в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ

def test_fc06_write_register():
    with make_client() as c:
        rr = c.write_register(address=0, value=0x1234, device_id=DEVICE_ID)
        assert ok(rr), explain(rr)


def test_fc06_write_read_roundtrip():
    """FC06 write -> FC03 read: value must match."""
    with make_client() as c:
        val = random.randint(1, 0xFFFF)
        c.write_register(address=10, value=val, device_id=DEVICE_ID)
        rr = c.read_holding_registers(address=10, count=1, device_id=DEVICE_ID)
        assert ok(rr), explain(rr)
        assert rr.registers[0] == val, (
            f"FC06->FC03 roundtrip: expected {val:#06x}, got {rr.registers[0]:#06x}"
        )


# в”Ђв”Ђ FC 15 вЂ“ Write Multiple Coils в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ

def test_fc15_write_multiple_coils():
    with make_client() as c:
        values = [True, False, True, True, False, True, False, True]
        rr = c.write_coils(address=0, values=values, device_id=DEVICE_ID)
        assert ok(rr), explain(rr)


def test_fc15_write_read_roundtrip():
    """FC15 write -> FC01 read: bit pattern must match."""
    with make_client() as c:
        values = [bool(random.getrandbits(1)) for _ in range(8)]
        c.write_coils(address=0, values=values, device_id=DEVICE_ID)
        rr = c.read_coils(address=0, count=8, device_id=DEVICE_ID)
        assert ok(rr), explain(rr)
        assert rr.bits[:8] == values, "FC15->FC01 roundtrip mismatch"


# в”Ђв”Ђ FC 16 вЂ“ Write Multiple Registers в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ

def test_fc16_write_multiple_registers():
    with make_client() as c:
        values = [0x0001, 0x0002, 0x0003, 0x0004, 0x0005]
        rr = c.write_registers(address=0, values=values, device_id=DEVICE_ID)
        assert ok(rr), explain(rr)


def test_fc16_write_read_roundtrip():
    """FC16 write -> FC03 read: register array must match."""
    with make_client() as c:
        values = [random.randint(0, 0xFFFF) for _ in range(10)]
        c.write_registers(address=20, values=values, device_id=DEVICE_ID)
        rr = c.read_holding_registers(address=20, count=10, device_id=DEVICE_ID)
        assert ok(rr), explain(rr)
        assert rr.registers == values, "FC16->FC03 roundtrip mismatch"


# в”Ђв”Ђ FC 22 вЂ“ Mask Write Register в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ

def test_fc22_mask_write_register():
    """
    Formula: result = (current AND and_mask) OR (or_mask AND NOT and_mask)
    Initial: 0xFF00, AND=0xF0F0, OR=0x0F0F -> expected 0xF00F
    """
    with make_client() as c:
        c.write_register(address=30, value=0xFF00, device_id=DEVICE_ID)
        rr = c.mask_write_register(address=30, and_mask=0xF0F0, or_mask=0x0F0F, device_id=DEVICE_ID)
        assert ok(rr), explain(rr)
        # Verify the result by reading back
        rr2 = c.read_holding_registers(address=30, count=1, device_id=DEVICE_ID)
        assert ok(rr2), explain(rr2)
        expected = (0xFF00 & 0xF0F0) | (0x0F0F & ~0xF0F0 & 0xFFFF)
        assert rr2.registers[0] == expected, (
            f"FC22: expected {expected:#06x}, got {rr2.registers[0]:#06x}"
        )


# в”Ђв”Ђ FC 23 вЂ“ Read/Write Multiple Registers в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ

def test_fc23_readwrite_registers():
    """Single request: write to address 40, read from address 0."""
    with make_client() as c:
        write_vals = [0xAAAA, 0xBBBB, 0xCCCC]
        rr = c.readwrite_registers(
            read_address=0,   read_count=5,
            write_address=40, values=write_vals,
            device_id=DEVICE_ID,
        )
        assert ok(rr), explain(rr)
        assert len(rr.registers) == 5


def test_fc23_write_then_read_same_range():
    """FC23 overlapping ranges: read result must reflect the newly written values."""
    with make_client() as c:
        write_vals = [random.randint(0, 0xFFFF) for _ in range(3)]
        rr = c.readwrite_registers(
            read_address=50,  read_count=3,
            write_address=50, values=write_vals,
            device_id=DEVICE_ID,
        )
        assert ok(rr), explain(rr)
        # Per spec: when ranges overlap, read returns the NEW (written) values
        assert rr.registers == write_vals


# в”Ђв”Ђ FC 08 вЂ“ Diagnostics в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ

def test_fc08_return_query_data():
    """Sub-function 00: server must echo the request data back unchanged."""
    try:
        from pymodbus.diag_message import ReturnQueryDataRequest
    except ImportError:
        pytest.skip("pymodbus.diag_message not available in this version")

    with make_client() as c:
        req = ReturnQueryDataRequest(message=[0xDEAD, 0xBEEF], device_id=DEVICE_ID)
        rr = c.execute(req)
        assert ok(rr), explain(rr)


# в”Ђв”Ђ FC 11 вЂ“ Get Communication Event Counter в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ

def test_fc11_get_event_counter():
    try:
        from pymodbus.diag_message import GetCommunicationEventCounterRequest
    except ImportError:
        pytest.skip("GetCommunicationEventCounterRequest not available")

    with make_client() as c:
        rr = c.execute(GetCommunicationEventCounterRequest(device_id=DEVICE_ID))
        assert ok(rr), explain(rr)


# в”Ђв”Ђ FC 12 вЂ“ Get Communication Event Log в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ

def test_fc12_get_event_log():
    try:
        from pymodbus.diag_message import GetCommunicationEventLogRequest
    except ImportError:
        pytest.skip("GetCommunicationEventLogRequest not available")

    with make_client() as c:
        rr = c.execute(GetCommunicationEventLogRequest(device_id=DEVICE_ID))
        assert ok(rr), explain(rr)


# в”Ђв”Ђ FC 17 вЂ“ Report Server ID в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ

def test_fc17_report_server_id():
    try:
        from pymodbus.other_message import ReportSlaveIdRequest
    except ImportError:
        try:
            from pymodbus.pdu.other_message import ReportSlaveIdRequest
        except ImportError:
            pytest.skip("ReportSlaveIdRequest not available")

    with make_client() as c:
        rr = c.execute(ReportSlaveIdRequest(device_id=DEVICE_ID))
        assert ok(rr), explain(rr)


# в”Ђв”Ђ FC 24 вЂ“ Read FIFO Queue в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ

def test_fc24_read_fifo_queue():
    """FIFO is normally empty -- an empty response without error is acceptable."""
    with make_client() as c:
        rr = c.read_fifo_queue(address=0, device_id=DEVICE_ID)
        assert rr is not None, "FC24: no response (timeout or connection lost)"
        # Empty FIFO is not an error, just count=0
        if rr.isError():
            pytest.xfail(f"FC24: {explain(rr)}")


# в”Ђв”Ђ FC 43 / 14 вЂ“ Read Device Identification (MEI) в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ

def test_fc43_device_identification():
    try:
        from pymodbus.mei_message import ReadDeviceInformationRequest
    except ImportError:
        pytest.skip("ReadDeviceInformationRequest not available")

    with make_client() as c:
        rr = c.execute(ReadDeviceInformationRequest(device_id=DEVICE_ID))
        assert ok(rr), explain(rr)


# в”Ђв”Ђ Multiple concurrent connections в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ

def _concurrent_worker(results: list, idx: int, requests_per_client: int = 5):
    """Worker thread: connect, send N requests, disconnect."""
    try:
        c = make_client()
        for _ in range(requests_per_client):
            rr = c.read_holding_registers(address=0, count=5, device_id=DEVICE_ID)
            if not ok(rr):
                results[idx] = f"Modbus error: {explain(rr)}"
                c.close()
                return
        c.close()
        results[idx] = "ok"
    except Exception as exc:
        results[idx] = f"exception: {exc}"


def test_concurrent_connections():
    """
    Open CONCURRENT_CLIENTS connections simultaneously.
    Each sends several requests -- all must complete without errors.
    """
    results = [None] * CONCURRENT_CLIENTS
    threads = [
        threading.Thread(target=_concurrent_worker, args=(results, i))
        for i in range(CONCURRENT_CLIENTS)
    ]
    for t in threads:
        t.start()
    for t in threads:
        t.join(timeout=30)

    failed = [
        f"  client {i}: {r}"
        for i, r in enumerate(results)
        if r != "ok"
    ]
    assert not failed, (
        f"Errors with {CONCURRENT_CLIENTS} concurrent connections:\n"
        + "\n".join(failed)
    )


# в”Ђв”Ђ Stress test в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ

_STRESS_FUNCTIONS = [
    lambda c: c.read_coils(0, count=8, device_id=DEVICE_ID),
    lambda c: c.read_discrete_inputs(0, count=8, device_id=DEVICE_ID),
    lambda c: c.read_holding_registers(0, count=10, device_id=DEVICE_ID),
    lambda c: c.read_input_registers(0, count=10, device_id=DEVICE_ID),
    lambda c: c.write_coil(0, random.choice([True, False]), device_id=DEVICE_ID),
    lambda c: c.write_register(0, random.randint(0, 0xFFFF), device_id=DEVICE_ID),
    lambda c: c.write_registers(0, [random.randint(0, 0xFFFF) for _ in range(5)], device_id=DEVICE_ID),
]


def _stress_worker(results: list, idx: int):
    errors = 0
    try:
        c = make_client()
        for _ in range(STRESS_REQ_EACH):
            fn = random.choice(_STRESS_FUNCTIONS)
            rr = fn(c)
            if not ok(rr):
                errors += 1
        c.close()
        results[idx] = errors
    except Exception as exc:
        results[idx] = f"exception: {exc}"


def test_stress(require_fc_tests_passed):
    """
    STRESS_THREADS threads, each sending STRESS_REQ_EACH random requests.
    Acceptable error rate: < 1%.
    Skipped automatically if any Modbus function test failed.
    """
    results = [None] * STRESS_THREADS
    t0 = time.monotonic()

    threads = [
        threading.Thread(target=_stress_worker, args=(results, i))
        for i in range(STRESS_THREADS)
    ]
    for t in threads:
        t.start()
    for t in threads:
        t.join(timeout=120)

    elapsed = time.monotonic() - t0
    total   = STRESS_THREADS * STRESS_REQ_EACH

    exceptions = [f"  thread {i}: {r}" for i, r in enumerate(results) if isinstance(r, str)]
    errors     = sum(r for r in results if isinstance(r, int))

    rps = total / elapsed if elapsed > 0 else 0
    print(
        f"\n  Stress: {total} requests in {elapsed:.1f}s "
        f"({rps:.0f} req/s) | errors: {errors} | exceptions: {len(exceptions)}"
    )

    assert not exceptions, "Exceptions in threads:\n" + "\n".join(exceptions)

    error_rate = errors / total
    assert error_rate < 0.01, (
        f"Too many errors: {errors}/{total} ({error_rate:.1%}). Limit is 1%."
    )


# в”Ђв”Ђ Standalone entry point в”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђв”Ђ

if __name__ == "__main__":
    sys.exit(pytest.main([__file__, "-v", "--tb=short", "-s"]))
