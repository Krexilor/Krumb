#pragma once

// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <cstdio>
#include <memory>
#include <filesystem>
#include <string_view>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/sinks/line_sink.hpp>
#include <krumb/formatters/formatter.hpp>

// FILE SINK ---------------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    // Writes one text line per event to a file. thread-safe: each line is written as a whole.
    // the file is opened in binray mode, so lines end in "\n" on every platform.
    class file_sink : public line_sink {
        public:
        enum class mode {
            append,
            truncate
        };

        // Default pattern. Throws std::system_error if the file cannot be opened.
        explicit file_sink(const std::filesystem::path& path, mode open_mode = mode::append);

        file_sink(const std::filesystem::path& path, std::unique_ptr<formatter> fmt, mode open_mode = mode::append);
        ~file_sink() override;

        private:
        void open(const std::filesystem::path& path, mode open_mode);
        void output(level lvl, std::string_view line) override;
        void flush_output() override;

        std::FILE* file_ = nullptr;
    };
}
