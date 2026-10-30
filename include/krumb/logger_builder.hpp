#pragma once

// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <memory>
#include <vector>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/logger.hpp>
#include <krumb/core/level.hpp>
#include <krumb/sinks/sink.hpp>

// LOGGER BUILDER ----------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    // Configures and creates loggers. Every setter returns the builder, so calls can be chained.
    // build() can be called more than once; each call creates an independent logger.
    class logger_builder {
        public:
        // Minimum level of the logger. Messages below it are dropped.
        logger_builder& min_level(level lvl) noexcept {
            min_level_ = lvl;

            return *this;
        }

        // After a message at or above this level, the sinks that received it are flushed.
        logger_builder& flush_level(level lvl) noexcept {
            flush_level_ = lvl;

            return *this;
        }

        logger_builder& add_sink(std::shared_ptr<sink> target, level threshold = level::trace);
        logger_builder& on_error(error_handler handler);
        logger build() const;

        private:
        level min_level_ = level::info;
        level flush_level_ = level::error;
        std::vector<detail::sink_entry> sinks_;
        error_handler handler_;
    };
}
