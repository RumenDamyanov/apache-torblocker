# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-04-04

### Added
- Initial project scaffolding
- Rust + C FFI Apache module structure

## [0.1.0] - Unreleased

### Added
- Three access modes: off, on (block Tor), only (allow only Tor)
- Automatic Tor exit node list fetching
- HTTPS support with TLS verification
- Per-location configuration
- O(1) IP hash set lookup
- Configurable refresh interval
- Fail-open during list fetch failures
- Port of nginx-torblocker functionality
