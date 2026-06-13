#!/usr/bin/env python3
"""
Integration tests for Modbus RTU over TCP/IP.

Load test-modbusrtutcp.omsim in Open ModSim before running:

    pytest test-modbusrtutcp.py -v

The transport is a raw Modbus RTU ADU (unit ID, PDU, CRC16) carried by a TCP
stream. It has no MBAP header.
"""

from concurrent.futures import ThreadPoolExecutor
import os
import socket
import struct
import time

import pytest


HOST = os.environ.get("OMODSIM_RTU_TCP_HOST", "127.0.0.1")
PORT = int(os.environ.get("OMODSIM_RTU_TCP_PORT", "502"))
UNIT_ID = 1
TIMEOUT = 2.0


def crc16(data: bytes) -> int:
    """Return the Modbus CRC16 value for data."""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc


def make_frame(unit_id: int, function_code: int, payload: bytes = b"") -> bytes:
    """Build an RTU request frame."""
    body = bytes((unit_id, function_code)) + payload
    return body + struct.pack("<H", crc16(body))


def expected_response_size(buffer: bytes) -> int | None:
    """Return a complete response size or None when more bytes are needed."""
    if len(buffer) < 2:
        return None

    function_code = buffer[1]
    if function_code & 0x80:
        return 5
    if function_code in (0x01, 0x02, 0x03, 0x04, 0x0C, 0x11, 0x17):
        return 5 + buffer[2] if len(buffer) >= 3 else None
    if function_code in (0x05, 0x06, 0x08, 0x0B, 0x0F, 0x10):
        return 8
    if function_code == 0x07:
        return 5
    if function_code == 0x16:
        return 10
    return None


class RtuTcpClient:
    """Small buffered raw-socket client for RTU over TCP/IP."""

    def __init__(self, timeout: float = TIMEOUT):
        try:
            self.socket = socket.create_connection((HOST, PORT), timeout=timeout)
        except OSError:
            pytest.skip(
                f"omodsim RTU-over-TCP server not reachable at {HOST}:{PORT}; "
                "load test-modbusrtutcp.omsim in omodsim to run this test."
            )
        self.socket.settimeout(timeout)
        self.buffer = bytearray()

    def close(self) -> None:
        """Close the TCP socket."""
        self.socket.close()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()

    def send(self, data: bytes) -> None:
        """Send all supplied bytes."""
        self.socket.sendall(data)

    def receive(self) -> bytes:
        """Receive one complete RTU response and validate its CRC."""
        while True:
            size = expected_response_size(self.buffer)
            if size is not None and len(self.buffer) >= size:
                frame = bytes(self.buffer[:size])
                del self.buffer[:size]
                assert crc16(frame[:-2]) == int.from_bytes(frame[-2:], "little")
                return frame
            data = self.socket.recv(4096)
            if not data:
                raise ConnectionError("Socket closed while reading an RTU response")
            self.buffer.extend(data)


def read_holding_request(address: int, count: int = 1, unit_id: int = UNIT_ID) -> bytes:
    """Build an FC03 request."""
    return make_frame(unit_id, 0x03, struct.pack(">HH", address, count))


def write_register_request(address: int, value: int, unit_id: int = UNIT_ID) -> bytes:
    """Build an FC06 request."""
    return make_frame(unit_id, 0x06, struct.pack(">HH", address, value))


def response_registers(frame: bytes) -> list[int]:
    """Decode register values from an FC03 response."""
    assert frame[1] == 0x03
    byte_count = frame[2]
    return list(struct.unpack(f">{byte_count // 2}H", frame[3:3 + byte_count]))


def test_fc03_read_holding_registers():
    with RtuTcpClient() as client:
        client.send(read_holding_request(0, 4))
        response = client.receive()
        assert response[:3] == bytes((UNIT_ID, 0x03, 8))
        assert len(response_registers(response)) == 4


def test_fc06_write_and_read_back():
    with RtuTcpClient() as client:
        request = write_register_request(10, 0x1234)
        client.send(request)
        assert client.receive() == request

        client.send(read_holding_request(10))
        assert response_registers(client.receive()) == [0x1234]


def test_fragmented_request():
    request = read_holding_request(0, 2)
    with RtuTcpClient() as client:
        client.send(request[:3])
        time.sleep(0.05)
        client.send(request[3:6])
        time.sleep(0.05)
        client.send(request[6:])
        assert len(response_registers(client.receive())) == 2


def test_multiple_requests_in_one_send_preserve_order():
    first = write_register_request(20, 0x1111)
    second = write_register_request(21, 0x2222)
    with RtuTcpClient() as client:
        client.send(first + second)
        assert client.receive() == first
        assert client.receive() == second


def test_bad_crc_is_ignored_and_stream_recovers():
    bad_request = bytearray(read_holding_request(0))
    bad_request[-1] ^= 0xFF

    with RtuTcpClient(timeout=0.2) as client:
        client.send(bytes(bad_request))
        with pytest.raises(socket.timeout):
            client.receive()

        client.socket.settimeout(TIMEOUT)
        client.send(read_holding_request(0))
        assert len(response_registers(client.receive())) == 1


def test_oversized_garbage_is_skipped():
    with RtuTcpClient() as client:
        client.send(b"\xff" * 300 + read_holding_request(0))
        assert len(response_registers(client.receive())) == 1


def test_illegal_address_exception():
    with RtuTcpClient() as client:
        client.send(read_holding_request(60000))
        response = client.receive()
        assert response[:3] == bytes((UNIT_ID, 0x83, 0x02))


def test_broadcast_write_has_no_response_and_updates_unit():
    broadcast = write_register_request(30, 0x3456, unit_id=0)
    with RtuTcpClient(timeout=0.2) as client:
        client.send(broadcast)
        with pytest.raises(socket.timeout):
            client.receive()

        client.socket.settimeout(TIMEOUT)
        client.send(read_holding_request(30))
        assert response_registers(client.receive()) == [0x3456]


def test_client_disconnect_with_pending_data_does_not_stop_server():
    client = RtuTcpClient()
    client.send(read_holding_request(0)[:4])
    client.close()

    with RtuTcpClient() as second_client:
        second_client.send(read_holding_request(0))
        assert len(response_registers(second_client.receive())) == 1


def test_multiple_clients():
    def read_once(index: int) -> int:
        with RtuTcpClient() as client:
            client.send(read_holding_request(index % 20))
            return response_registers(client.receive())[0]

    with ThreadPoolExecutor(max_workers=8) as executor:
        values = list(executor.map(read_once, range(24)))
    assert len(values) == 24
