// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <chrono>
#include <format>
#include <string>
#include <utility>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <string_view>
#include <source_location>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/formatters/pattern_formatter.hpp>

// HELPERS -----------------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    std::string_view base_name(std::string_view path) noexcept {
        const auto pos = path.find_last_of("/\\");

        if (pos == std::string_view::npos) {
            return path;
        }

        return path.substr(pos + 1);
    }

    std::string_view upper_name(krumb::level lvl) noexcept {
        switch (lvl) {
            case krumb::level::trace:
                return "TRACE";

            case krumb::level::debug:
                return "DEBUG";

            case krumb::level::info:
                return "INFO";

            case krumb::level::warn:
                return "WARN";

            case krumb::level::error:
                return "ERROR";

            case krumb::level::critical:
                return "CRITICAL";

            case krumb::level::off:
                return "OFF";
        }

        return "UNKNOWN";
    }

    [[noreturn]] void reject(const std::string& reason) {
        throw std::invalid_argument("krumb: invalid pattern: " + reason);
    }
}

// PATTERN FORMATTER -------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    pattern_formatter::pattern_formatter(std::string_view pattern) : segments_(parse(pattern)) {
        const event sample{
            .lvl = level::info,
            .timestamp = std::chrono::system_clock::now(),
            .location = std::source_location::current(),
            .message = "sample"
        };
        std::string scratch;

        try {
            render(sample, scratch);
        }
        catch (const std::format_error& ex) {
            reject(ex.what());
        }
    }

    void pattern_formatter::format(const event& ev, std::string& out) {
        render(ev, out);
    }

    bool pattern_formatter::lookup_field(std::string_view name, field& result) {
        if (name == "time") {
            result = field::time;
        }
        else if (name == "level") {
            result = field::level;
        }
        else if (name == "LEVEL") {
            result = field::level_upper;
        }
        else if (name == "file") {
            result = field::file;
        }
        else if (name == "path") {
            result = field::path;
        }
        else if (name == "line") {
            result = field::line;
        }
        else if (name == "function") {
            result = field::function;
        }
        else if (name == "message") {
            result = field::message;
        }
        else {
            return false;
        }

        return true;
    }

    std::vector<pattern_formatter::segment> pattern_formatter::parse(std::string_view pattern) {
        std::vector<segment> segments;
        std::string literal;

        auto flush_literal = [&] {
            if (!literal.empty()) {
                segments.push_back(segment{field::literal, literal});
                literal.clear();
            }
        };

        for (std::size_t i = 0; i < pattern.size(); i++) {
            const char current = pattern[i];
            
            if (current == '{') {
                if (i + 1 < pattern.size() && pattern[i + 1] == '{') {
                    literal.push_back('{');
                    i++;
                    continue;
                }

                const std::size_t close = pattern.find('}', i + 1);

                if (close == std::string_view::npos) {
                    reject("'{' without a closing '}'");
                }

                const std::string_view body = pattern.substr(i + 1, close - i - 1);
                const std::size_t colon = body.find(':');
                const std::string_view name = body.substr(0, colon);
                const std::string_view spec = (colon == std::string_view::npos) ? std::string_view{} : body.substr(colon + 1);

                if (spec.find('{') != std::string_view::npos) {
                    reject("a field spec cannot contain '{'");
                }

                field kind = field::literal;

                if (!lookup_field(name, kind)) {
                    reject(std::format("unknown field '{}'", name));
                }

                flush_literal();

                std::string text = "{";

                if (colon != std::string_view::npos) {
                    text += ':';
                    text += spec;
                }
                else if (kind == field::time) {
                    text += ":%F %T";
                }

                text += '}';

                segments.push_back(segment{kind, std::move(text)});
                i = close;
            }
            else if (current == '}') {
                if (i + 1 < pattern.size() && pattern[i + 1] == '}') {
                    literal.push_back('}');
                    i++;
                }
                else {
                    reject("'}' without an opening '{'");
                }
            }
            else {
                literal.push_back(current);
            }
        }

        flush_literal();

        return segments;
    }

    void pattern_formatter::render(const event& ev, std::string& out) const {
        auto put = [&out](const segment& seg, auto& value) {
            std::vformat_to(std::back_inserter(out), seg.text, std::make_format_args(value));
        };

        for (const segment& seg : segments_) {
            switch (seg.kind) {
                case field::literal:
                    out += seg.text;
                    break;

                case field::time: {
                    auto value = std::chrono::floor<std::chrono::milliseconds>(ev.timestamp);

                    put(seg, value);
                    break;
                }

                case field::level: {
                    std::string_view value = to_string(ev.lvl);

                    put(seg, value);
                    break;
                }

                case field::level_upper: {
                    std::string_view value = upper_name(ev.lvl);

                    put(seg, value);
                    break;
                }

                case field::file: {
                    std::string_view value = base_name(ev.location.file_name());

                    put(seg, value);
                    break;
                }

                case field::path: {
                    std::string_view value = ev.location.file_name();

                    put(seg, value);
                    break;
                }

                case field::line: {
                    auto value = ev.location.line();

                    put(seg, value);
                    break;
                }

                case field::function: {
                    std::string_view value = ev.location.function_name();

                    put(seg, value);
                    break;
                }

                case field::message: {
                    std::string_view value = ev.message;

                    put(seg, value);
                    break;
                }
            }
        }
    }
}
