// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <cerrno>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <utility>
#include <stdexcept>
#include <string_view>
#include <system_error>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/sinks/console_sink.hpp>

// HELPERS -----------------------------------------------------------------------------------------------------------------------------------------|
namespace {
    constexpr std::string_view reset_code = "\x1b[0m";

    std::string_view color_code(krumb::level lvl) noexcept {
        switch (lvl) {
            case krumb::level::trace:
                return "\x1b[90m";

            case krumb::level::debug:
                return "\x1b[36m";

            case krumb::level::info:
                return "\x1b[32m";

            case krumb::level::warn:
                return "\x1b[33m";

            case krumb::level::error:
                return "\x1b[31m";

            case krumb::level::critical:
                return "\x1b[1;31m";

            case krumb::level::off:
                break;
        }

        return "";
    }

    std::string read_environment(const char* name) {
        #ifdef _WIN32
            char* value = nullptr;
            std::size_t size = 0;
            std::string result;

            if (_dupnev_s(&value, &size, name) == 0 && value != nullptr) {
                result = value;
            }

            std::free(value);

            return result;
        #else
            const char* value = std::getenv(name);

            return value != nullptr ? std::string(value) : std::string();
        #endif
    }

    bool is_terminal(std::FILE* stream) {
        #ifdef _WIN32
            return _isatty(_fileno(stream)) != 0;
        #else
            return isatty(fileno(stream)) != 0;
        #endif
    }

    bool enable_virtual_terminal(std::FILE* stream) {
        #ifdef _WIN32
            const std::intptr_t raw = _get_osfhandler(_fileno(stream));

            if (raw == -1) {
                return false;
            }

            const HANDLE handle = reinterpret_cast<HANDLE>(raw);
            DWORD mode = 0;

            if (!GetConsoleMode(handle, &mode)) {
                return false;
            }

            return SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
        #else
            static_cast<void>(stream);
            
            return true;
        #endif
    }

    std::FILE* require_stream(std::FILE* stream) {
        if (stream == nullptr) {
            throw std::invalid_argument("krumb: console_sink requires a non-null stream");
        }

        return stream;
    }

    bool decide_color(const krumb::console_options& options) {
        switch (options.color) {
            case krumb::color_mode::never:
                return false;

            case krumb::color_mode::always:
                enable_virtual_terminal(options.stream);

                return true;

            case krumb::color_mode::automatic:
                break;
        }

        if (!is_terminal(options.stream)) {
            return false;
        }

        if (!read_environment("NO COLOR").empty()) {
            return false;
        }

        if (read_environment("TERM") == "dumb") {
            return false;
        }

        return enable_virtual_terminal(options.stream);
    }
}

// CONSOLE SINK ------------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    console_sink::console_sink() : console_sink(console_options{}) {}

    console_sink::console_sink(console_options options) :
        line_sink(),
        stream_(require_stream(options.stream)),
        use_color_(decide_color(options)) {}

    console_sink::console_sink(std::unique_ptr<formatter> fmt, console_options options) :
        line_sink(std::move(fmt)),
        stream_(require_stream(options.stream)),
        use_color_(decide_color(options)) {}

    void console_sink::output(level lvl, std::string_view line) {
        std::string_view data = line;

        if (use_color_) {
            std::string_view text = line;

            if (!text.empty() && text.back() == '\n') {
                text.remove_suffix(1);
            }

            colored_.clear();
            colored_ += color_code(lvl);
            colored_ += text;
            colored_ += reset_code;
            colored_ += '\n';

            data = colored_;
        }

        if (std::fwrite(data.data(), 1, data.size(), stream_) != data.size()) {
            throw std::system_error(errno, std::generic_category(), "krumb: failed to write to the console stream");
        }
    }

    void console_sink::flush_output() {
        if (std::fflush(stream_) != 0) {
            throw std::system_error(errno, std::generic_category(), "krumb: failed to flush the console stream");
        }
    }
}
