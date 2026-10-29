// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <chrono>
#include <format>
#include <iterator>
#include <string_view>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/formatters/text_formatter.hpp>

// HELPERS -----------------------------------------------------------------------------------------------------------------------------------------|
namespace {
    std::string_view base_name(std::string_view path) noexcept {
        const auto pos = path.find_last_of("/\\");

        if (pos == std::string_view::npos) {
            return path;
        }

        return path.substr(pos + 1);
    }
}

// TEXT FORMATTER ----------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    void text_formatter::format(const event& ev, std::string& out) const {
        const auto time = std::chrono::floor<std::chrono::milliseconds>(ev.timestamp);

        std::format_to(
            std::back_inserter(out),
            "[{:%F %T}] [{}] [{}:{}] {}",
            time,
            to_string(ev.lvl),
            base_name(ev.location.file_name()),
            ev.location.line(),
            ev.message
        );
    }
}
