#pragma once

// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <mutex>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstddef>
#include <stdexcept>
#include <filesystem>
#include <system_error>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/sinks/sink.hpp>

// TEST HELPERS ------------------------------------------------------------------------------------------------------------------------------------|
namespace test {
    // Records every message and level it receives, and counts how often it is flushed.
    class recording_sink : public krumb::sink {
    public:
        void write(const krumb::event& ev) override {
            std::lock_guard lock(mutex_);

            levels_.push_back(ev.lvl);
            messages_.emplace_back(ev.message);
        }

        void flush() override {
            std::lock_guard lock(mutex_);

            flush_count_++;
        }

        std::vector<std::string> messages() const {
            std::lock_guard lock(mutex_);

            return messages_;
        }

        std::vector<krumb::level> levels() const {
            std::lock_guard lock(mutex_);

            return levels_;
        }

        int flush_count() const {
            std::lock_guard lock(mutex_);

            return flush_count_;
        }

    private:
        mutable std::mutex mutex_;
        std::vector<std::string> messages_;
        std::vector<krumb::level> levels_;
        int flush_count_ = 0;
    };

    // A sink whose write and flush always fail.
    class failing_sink : public krumb::sink {
    public:
        void write(const krumb::event&) override {
            throw std::runtime_error("simulated write error");
        }

        void flush() override {
            throw std::runtime_error("simulated flush error");
        }
    };

    // A path in the system temp folder. Any file at that path is removed when the object is created and when it is destroyed.
    // Declare it before the sinks that use it, so the file is closed before it is removed.
    class temp_file {
    public:
        explicit temp_file(const std::string& name)
            : path_(std::filesystem::temp_directory_path() / name) {
            remove();
        }

        ~temp_file() {
            remove();
        }

        temp_file(const temp_file&) = delete;
        temp_file& operator=(const temp_file&) = delete;

        const std::filesystem::path& path() const {
            return path_;
        }

    private:
        void remove() const {
            std::error_code ignored;

            std::filesystem::remove(path_, ignored);
        }

        std::filesystem::path path_;
    };

    inline std::string read_file(const std::filesystem::path& path) {
        std::ifstream stream(path, std::ios::binary);
        std::ostringstream content;

        content << stream.rdbuf();

        return content.str();
    }

    // Splits on '\n'. a final line without a newline is kept, so a half-written line is visible to tests.
    inline std::vector<std::string> split_lines(const std::string& text) {
        std::vector<std::string> lines;
        std::size_t start = 0;

        while (start < text.size()) {
            const std::size_t end = text.find('\n', start);

            if (end == std::string::npos) {
                lines.push_back(text.substr(start));
                break;
            }

            lines.push_back(text.substr(start, end - start));
            start = end + 1;
        }

        return lines;
    }
}
