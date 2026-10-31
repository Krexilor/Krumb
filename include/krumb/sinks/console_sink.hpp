#pragma once

// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/sinks/line_sink.hpp>
#include <krumb/formatters/formatter.hpp>

// CONSOLE SINK ------------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    // When the console sink colors its lines.
    //   automatic: Only if the stream is a terminal, the NO_COLOR environment variable is not set (or empty),
    //              and TERM is not "dumb". This is the default.
    //   always:    always, even when the output is redirected.
    //   never:     never.
    enum class color_mode {
        automatic,
        always,
        never
    };

    struct console_options {
        std::FILE* stream = stdout;
        color_mode color = color_mode::automatic;
    };

    class console_sink : public line_sink {
        public:
        console_sink();

        explicit console_sink(console_options options);
        explicit console_sink(std::unique_ptr<formatter> fmt, console_options options = {});

        private:
        void output(level lvl, std::string_view line) override;
        void flush_output() override;

        std::FILE* stream_;
        bool use_color_;
        std::string colored_;
    };
}
