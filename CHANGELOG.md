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
- File logging added to the example.

### Changed
- `logger` takes a `std::shared_ptr<sink>`; sinks format events themselves.
- Locking moved from the logger into the sinks, so one sink can be shared by several loggers.
- Sink and formatting failures no longer propagate to the caller; they go to the error handler.

### Known limitations
- A persistently failing sink reports every failed write to the error handler; there is no rate limiting.
- The error handler can be called from the logger's destructor; its captured state must outlive the logger.
- Timestamps are UTC only.
- Logging from static destructors is unsupported.

## [0.3.0] - 2026-10-31

### Added
- `logger_builder`: configure the logger level, flush level, error handler, and any number of sinks, each with its own threshold.
- `krumb::set_default_logger` replaces the logger behind the free functions.
- Application example (`examples/application`): a logger with console and file sinks, passed to a component, then used as the default logger.
- Tests for the builder, multiple sinks and thresholds, failure isolation, shared sinks, logger handles, and the default logger, including concurrent use.

### Changed
- `logger` is now a copyable handle: copies share level, sinks, and error handler, and the last handle flushes the sinks.
- `krumb::default_logger()` returns a handle instead of a reference.
- A failing sink no longer stops other sinks from receiving a message. Errors name the sink index, for example `sink 1 write failed: disk full`.
- The reliability contract in `logger.hpp` now covers handles, multiple sinks, thresholds, and per-sink failures.

### Known limitations
- A persistently failing sink reports every failed write to the error handler; there is no rate limiting.
- The error handler can be called when the last handle of a logger is destroyed; its captured state must outlive the logger.
- A logger's sinks and their thresholds are fixed when it is built. The logger level and flush level can change at runtime.
- One logger object must not be assigned to while another thread uses that same object; give each thread its own handle.
- Every free-function call takes a short lock to read the default logger.
- Timestamps are UTC only.
- Logging from static destructors is unsupported.
