// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <cstdio>
#include <cerrno>
#include <system_error>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/sinks/console_sink.hpp>

// CONSOLE SINK ------------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    void console_sink::write(const event& ev) {
        std::lock_guard lock(mutex_);

        // The newline is part of the buffer, so the line goes out in a single fwrite call.
        buffer_.clear();
        formatter_.format(ev, buffer_);
        buffer_.push_back('\n');

        if (std::fwrite(buffer_.data(), 1, buffer_.size(), stdout) != buffer_.size()) {
            throw std::system_error(errno, std::generic_category(), "krumb: failed to write to stdout");
        }
    }

    void console_sink::flush() {
        std::lock_guard lock(mutex_);

        if (std::fflush(stdout) != 0) {
            throw std::system_error(errno, std::generic_category(), "krumb: failed to flush stdout");
        }
    }
}
