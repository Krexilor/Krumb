#pragma once

// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <mutex>
#include <string_view>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/sinks/sink.hpp>
#include <krumb/formatters/text_formatter.hpp>

// CONSOLE SINK ------------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    // Writes one tex line per event to stdout. thread-safe: each line is written as a whole.
    class console_sink : public sink {
        public:
        void write(const event& ev) override;
        void flush() override;

        private:
        std::mutex mutex_;
        text_formatter formatter_;
        std::string buffer_;
    };
}
