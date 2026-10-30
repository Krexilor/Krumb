#pragma once

// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <mutex>
#include <cstdio>
#include <string>
#include <filesystem>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/sinks/sink.hpp>
#include <krumb/formatters/text_formatter.hpp>

// FILE SINK ---------------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    // Writes one text line per event to a file. thread-safe: each line is written as a whole.
    // the file is opened in binray mode, so lines end in "\n" on every platform.
    class file_sink : public sink {
        public:
        enum class mode {
            append,
            truncate
        };

        // throws std::system_error if the file connot be opened.
        explicit file_sink(const std::filesystem::path& path, mode open_mode = mode::append);

        ~file_sink() override;

        void write(const event& ev) override;
        void flush() override;

        private:
        std::FILE* file_ = nullptr;
        std::mutex mutex_;
        text_formatter formatter_;
        std::string buffer_;
    };
}
