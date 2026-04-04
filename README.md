[![License](https://img.shields.io/github/license/RumenDamyanov/apache-torblocker?label=License)](LICENSE.md)

# Apache TorBlocker

An Apache httpd module that controls access from Tor exit nodes. Supports three modes: block Tor traffic, allow only Tor traffic, or allow all. Port of [nginx-torblocker](https://github.com/RumenDamyanov/nginx-torblocker) for Apache.

**Implementation:** Rust core with thin C FFI shim for Apache module registration.

## Features

- **Three access modes** — `off` (allow all), `on` (block Tor), `only` (allow only Tor)
- **Automatic list fetching** — downloads Tor exit node list without cron jobs
- **HTTPS support** — secure list fetching with TLS verification
- **Per-location configuration** — different policies for different paths
- **O(1) IP lookup** — hash set for efficient IP matching
- **Automatic refresh** — configurable update interval (default: 1 hour)
- **Fail-open by default** — permissive during list fetch failures
- **Memory safe** — Rust core prevents buffer overflows and use-after-free

## Quick Start

### 1. Install

**Debian/Ubuntu:**
```bash
# Add the OBS repository (see Installation Guide for details)
apt install apache-torblocker
```

### 2. Configure Apache

```apache
LoadModule torblocker_module modules/mod_torblocker.so

<VirtualHost *:80>
    ServerName example.com
    DocumentRoot /var/www/html

    # Block Tor exit nodes
    TorBlock on

    # Allow Tor for anonymous tips
    <Location /anonymous-tips>
        TorBlock only
    </Location>
</VirtualHost>
```

### 3. Verify

```bash
apachectl configtest && apachectl graceful
```

## Configuration Reference

| Directive | Context | Default | Description |
|-----------|---------|---------|-------------|
| `TorBlock` | server config, virtual host, directory, location | `off` | `off` / `on` (block Tor) / `only` (allow only Tor) |
| `TorBlockSourceUrl` | server config | torproject.org | Tor exit list URL |
| `TorBlockRefreshInterval` | server config | `3600` | Refresh interval in seconds |

## Building from Source

```bash
cargo build --release
make
sudo make install
```

## Related Projects

| Module | Description | GitHub |
|--------|-------------|--------|
| **apache-gone** | Return HTTP 410 Gone for permanently removed URIs | [GitHub](https://github.com/RumenDamyanov/apache-gone) |
| **apache-cf-remoteip** | Automatic Cloudflare IP list for RemoteIPTrustedProxy | [GitHub](https://github.com/RumenDamyanov/apache-cf-remoteip) |
| **apache-waf** | IP/CIDR-based access control with named lists | [GitHub](https://github.com/RumenDamyanov/apache-waf) |

### nginx Counterpart

| nginx Module | Apache Module |
|-------------|---------------|
| [nginx-torblocker](https://github.com/RumenDamyanov/nginx-torblocker) | **apache-torblocker** (this project) |

## License

Apache License 2.0. See [LICENSE.md](LICENSE.md).
