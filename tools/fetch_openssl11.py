#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 OpenModSim contributors
# SPDX-License-Identifier: MIT
"""Fetch the pinned OpenSSL 1.1 source used by Qt 5 builds."""

import hashlib
from pathlib import Path
import sys
import tarfile
import tempfile
import urllib.request

VERSION = "1.1.1w"
SHA256 = "cf3098950cb4d853ad95c0841f1f9c6d3dc102dccfcacd521d93925208b76ac8"
URL = f"https://github.com/openssl/openssl/releases/download/OpenSSL_1_1_1w/openssl-{VERSION}.tar.gz"


def main():
    if len(sys.argv) != 2:
        raise SystemExit("Usage: fetch_openssl11.py <destination>")
    destination = Path(sys.argv[1]).resolve()
    source = destination / f"openssl-{VERSION}"
    if not (source / "Configure").is_file():
        destination.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory() as temporary:
            archive = Path(temporary) / "openssl.tar.gz"
            urllib.request.urlretrieve(URL, archive)
            if hashlib.sha256(archive.read_bytes()).hexdigest() != SHA256:
                raise SystemExit("OpenSSL source checksum mismatch")
            with tarfile.open(archive) as package:
                package.extractall(destination)
    print(source)


if __name__ == "__main__":
    main()
