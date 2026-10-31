#pragma once

// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <string>
#include <vector>
#include <string_view>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/formatters/formatter.hpp>

// PATTERN FORMATTER -------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    // Formats events from a pattern. Text is copied as it is, and {name} or {name:spec} fields are replaced:
    //   {time}      timestamp (UTC). Default spec is %F %T. a spec is a standard chrono format, for example {time:%H:%M:%S}.
    //               %S and %T print milliseconds.
    //   {level}     level name, lowercase: info
    //   {LEVEL}     level name, uppercase: INFO
    //   {file}      source file name without its directory
    //   {path}      source file path as the compiler recorded it
    //   {line}      source line
    //   {function}  source function
    //   {message}   the log message
    // A spec after ':' is a standard format spec for the field.
    // use {{ and }} for literal braces.
    class pattern_formatter : public formatter {
        public:
        static constexpr std::string_view default_pattern = "[{time:%F %T}] [{level}] [{file}:{line}] {message}";

        explicit pattern_formatter(std::string_view pattern = default_pattern);
        void format(const event& ev, std::string& out) override;

        private:
        enum class field {
            literal,
            time,
            level,
            level_upper,
            file,
            path,
            line,
            function,
            message
        };

        struct segment {
            field kind;
            std::string text;
        };

        static std::vector<segment> parse(std::string_view pattern);
        static bool lookup_field(std::string_view name, field& result);
        
        void render(const event& ev, std::string& out) const;

        std::vector<segment> segments_;
    };
}
