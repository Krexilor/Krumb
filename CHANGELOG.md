# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Krumb is in early development: any 0.x release may contain breaking changes.
Semantic Versioning applies from 1.0.0.

## [0.1.0] - 2026-10-29

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

## [0.2.0] - 2026-10-30

### Added
- `sink` base class for custom destinations.
- `file_sink` with append and truncate modes.
- `logger::flush`, `set_flush_level` (default `error`), and `krumb::flush()`.
- The logger flushes its sink when destroyed.
- `logger::set_error_handler`; the default handler prints to stderr.
- Written reliability contract (thread safety, failures, flushing, shutdown) in `logger.hpp`.
- Tests for file I/O, flushing, error handling, and concurrency, plus a recording test sink.
- `KRUMB_ENABLE_TSAN` option and a ThreadSanitizer CI job.

### Changed
- `logger` takes a `std::shared_ptr<sink>`; sinks format events themselves.
- Locking moved from the logger into the sinks, so one sink can be shared by several loggers.
- Sink and formatting failures no longer propagate to the caller; they go to the error handler.

### Known limitations
- A persistently failing sink reports every failed write to the error handler; there is no rate limiting.
- The error handler can be called from the logger's destructor; its captured state must outlive the logger.
- Timestamps are UTC only.
- Logging from static destructors is unsupported.
