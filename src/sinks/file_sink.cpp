// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <cerrno>
#include <cstdio>
#include <system_error>

#ifdef _WIN32
#include <share.h>
#endif

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/sinks/file_sink.hpp>

// FILE SINK ---------------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    file_sink::file_sink(const std::filesystem::path& path, mode open_mode) {
        #ifdef _WIN32
            // _wfopen_s accepts the native wide path, so non-ASCII file names work, and it avoids MSVC's deprecation warning for fopen.
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

    file_sink::~file_sink() {
        std::fclose(file_);
    }

    void file_sink::write(const event& ev) {
        std::lock_guard lock(mutex_);

        buffer_.clear();
        formatter_.format(ev, buffer_);
        buffer_.push_back('\n');

        if (std::fwrite(buffer_.data(), 1, buffer_.size(), file_) != buffer_.size()) {
            throw std::system_error(errno, std::generic_category(), "krumb: failed to write to log file");
        }
    }

    void file_sink::flush() {
        std::lock_guard lock(mutex_);

        if (std::fflush(file_) != 0) {
            throw std::system_error(errno, std::generic_category(), "krumb: failed to flush log file");
        }
    }
}