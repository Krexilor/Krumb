# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Krumb is in early development: any 0.x release may contain breaking changes.
Semantic Versioning applies from 1.0.0.

## [Unreleased]

### Added
- Severity levels (`krumb::level`) with `to_string`.
- Log event (`krumb::event`) carrying level, timestamp, source location, and message.
- Compile-time checked `std::format` messages with automatic source-location capture.
- Default text formatter: `[YYYY-MM-DD HH:MM:SS.mmm] [level] [file:line] message` (UTC).
- Console sink writing to stdout.
- `krumb::logger` with runtime level filtering and per-write locking.
- Default logger with free functions: `trace`, `debug`, `info`, `warn`, `error`, `critical`, `log`, `set_level`, `get_level`.
- Unit tests (doctest) and a tour example.

### Known limitations
- No flush control, error policy, or formal thread-safety contract (planned for 0.2).
- Timestamps are UTC only.
- Logging from static destructors is unsupported.

