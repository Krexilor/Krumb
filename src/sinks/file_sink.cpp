// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <cerrno>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <system_error>

#ifdef _WIN32
#include <share.h>
#endif

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/sinks/file_sink.hpp>

// FILE SINK ---------------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    file_sink::file_sink(const std::filesystem::path& path, mode open_mode) : line_sink() {
        open(path, open_mode);
    }

    file_sink::file_sink(const std::filesystem::path& path, std::unique_ptr<formatter> fmt, mode open_mode) : line_sink(std::move(fmt)) {
        open(path, open_mode);
    }

    file_sink::~file_sink() {
        std::fclose(file_);
    }

    void file_sink::open(const std::filesystem::path& path, mode open_mode) {
        #ifdef _WIN32
            const wchar_t* flags = (open_mode == mode::append) ? L"ab" : L"wb";
            file_ = _wfsopen(path.c_str(), flags, _SH_DENYNO);
        #else
            const char* flags = (open_mode == mode::append) ? "ab" : "wb";
            file_ = std::fopen(path.c_str(), flags);
        #endif
            if (file_ == nullptr) {
                throw std::system_error(errno, std::generic_category(), "krumb: cannot open log file '" + path.string() + "'");
            }
    }

    void file_sink::output(level, std::string_view line) {
        if (std::fwrite(line.data(), 1, line.size(), file_) != line.size()) {
            throw std::system_error(errno, std::generic_category(), "krumb: failed to write to log file");
        }
    }

    void file_sink::flush_output() {
        if (std::fflush(file_) != 0) {
            throw std::system_error(errno, std::generic_category(), "krumb: failed to flush log file");
        }
    }
}
