# Modbus TCP over TLS

Open **Modbus Definitions → Certificates** before starting a TLS server.
This page manages the application-wide certificate store, as in OpenUaSim.

## Generate a server certificate

Enter the server name, the DNS names and IP addresses clients will use, and the
validity period. Select **Generate…** to create a self-signed RSA-2048 certificate
with the `serverAuth` purpose and a matching private key. DNS names and IP addresses
are recorded in the certificate's Subject Alternative Name extension.

Use **Export certificate…** to give clients a copy of the public PEM certificate.
Clients must explicitly trust this certificate. Export never includes the private key.
Replacing the server certificate requires clients to trust the new certificate.
No running CA service is needed for this workflow.

## Connect

Choose **Connect → Modbus/TCP Srv** and enable **Modbus TCP Security (TLS)**.
Enabling TLS changes the default port from 502 to 802; a custom port is preserved.
There are no certificate file selectors in the connection dialog. Every TLS endpoint
uses the shared server identity.

TLS 1.2 or later is required. All TLS clients are accepted without requesting or
verifying client certificates. Clients can still verify the server certificate.
Plain TCP connections are rejected. Modbus registers,
scripts, and error simulations also work over the encrypted connection.
RTU over TCP does not offer this TLS option.

Certificate operations are saved immediately, independently of the settings dialog's
Apply or Cancel buttons. Reconnect running TLS servers to apply certificate
changes. Existing connections retain the configuration they were started with.

## Storage and build requirements

The store is in `pki` under Qt's application data directory. The server certificate
and key are saved together in `own/identity.pem` using atomic replacement. On Unix,
the identity file is readable and writable only by its owner. Projects store the TLS
selection, while the server identity belongs to the local application installation.
Any existing files in the old `trusted` directory are left untouched and are not used.

OpenSSL 3 `libcrypto` is a build and runtime dependency for certificate generation.
The build scripts install it through Homebrew on macOS, Qt's OpenSSL package on
Windows, or distribution packages/source builds on Linux. Runtime libraries and the
OpenSSL license are included in packaged distributions. The application does not
invoke the `openssl` command-line tool. Integration tests use that utility to create
client test fixtures and skip when it or a Qt TLS backend is unavailable.

The simulator uses server-authenticated TLS. It does not require the mutual client
authentication described by [Modbus Security](https://www.modbus.org/modbus-specifications).
Certificate role extensions and role-based access rules are not interpreted.
