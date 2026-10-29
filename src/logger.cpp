// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <chrono>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/logger.hpp>
#include <krumb/core/event.hpp>

// LOGGER ------------------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    void logger::write(level lvl, const std::source_location& location, std::string_view message) {
        const event ev{
            .lvl = lvl,
            .timestamp = std::chrono::system_clock::now(),
            .location = location,
            .message = message
        };

        std::lock_guard lock(mutex_);

        buffer_.clear();
        formatter_.format(ev, buffer_);
        sink_.write(buffer_);
    }
}

// DEFAULT LOGGER ----------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    logger& default_logger() {
        static logger instance;

        return instance;
    }

    void set_level(level lvl) noexcept {
        default_logger().set_level(lvl);
    }

    level get_level() noexcept {
        return default_logger().get_level();
    }
}
