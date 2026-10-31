// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <array>
#include <mutex>
#include <deque>
#include <format>
#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <exception>
#include <string_view>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/krumb.hpp>

// LOGFMT FORMATTER --------------------------------------------------------------------------------------------------------------------------------|
namespace {
    // A custom formatter: one "key=value" line per event.
    // It only appends text. The sink adds the newline, and it calls the formatter one thread at a time, so no locking is needed.
    class logfmt_formatter : public krumb::formatter {
        public:
        void format(const krumb::event& ev, std::string& out) override {
            const auto time = std::chrono::floor<std::chrono::milliseconds>(ev.timestamp);

            std::format_to(std::back_inserter(out), "time={:%FT%T}Z level={} line={} msg=\"", time, krumb::to_string(ev.lvl), ev.location.line());
            append_escaped(ev.message, out);
            out += '"';
        }

        private:
        static void append_escaped(std::string_view text, std::string& out) {
            for (const char c : text) {
                if (c == '\n') {
                    out += "\\n";
                    continue;
                }

                if (c == '"' || c == '\\') {
                    out += '\\';
                }

                out += c;
            }
        }
    };
}

// RECENT LINES SINK -------------------------------------------------------------------------------------------------------------------------------|
namespace {
    // A custom line sink: it keeps the most recent lines in memory, for example to attach them to a crash report.
    // line_sink supplies the formatter, the lock and the buffer. This class only says where a finished line goes.
    // output() runs with the sink's own lock held, but a reader on another thread is not covered by that lock,
    // so this class guards its list with a second mutex.
    class recent_lines_sink : public krumb::line_sink {
        public:
        explicit recent_lines_sink(std::size_t capacity) : line_sink(std::make_unique<krumb::pattern_formatter>("[{LEVEL}] {message}")), capacity_(capacity) {}

        std::vector<std::string> snapshot() const {
            std::lock_guard lock(mutex_);

            return std::vector<std::string>(lines_.begin(), lines_.end());
        }

        private:
        void output(krumb::level, std::string_view line) override {
            std::lock_guard lock(mutex_);

            // The line ends with a newline, which is not wanted here.
            if (!line.empty() && line.back() == '\n') {
                line.remove_suffix(1);
            }

            lines_.emplace_back(line);

            if (lines_.size() > capacity_) {
                lines_.pop_front();
            }
        }

        void flush_output() override {
        }

        std::size_t capacity_;
        mutable std::mutex mutex_;
        std::deque<std::string> lines_;
    };
}

// LEVEL COUNTER SINK ------------------------------------------------------------------------------------------------------------------------------|
namespace {
    // A custom sink on the raw interface: it never formats anything, it only counts events per level.
    // It derives from krumb::sink directly because it needs the event, not a text line. A sink on this interface
    // must be thread-safe itself, which is why the counters are atomic.
    class level_counter_sink : public krumb::sink {
        public:
        void write(const krumb::event& ev) override {
            counts_[static_cast<std::size_t>(ev.lvl)].fetch_add(1, std::memory_order_relaxed);
        }

        void flush() override {
        }

        std::size_t count(krumb::level lvl) const {
            return counts_[static_cast<std::size_t>(lvl)].load(std::memory_order_relaxed);
        }

        private:
        std::array<std::atomic<std::size_t>, static_cast<std::size_t>(krumb::level::off) + 1> counts_{};
    };
}

// APPLICATION -------------------------------------------------------------------------------------------------------------------------------------|
int run() {
    auto counter = std::make_shared<level_counter_sink>();
    auto recent = std::make_shared<recent_lines_sink>(3);

    // One logger, four destinations, each with its own format and threshold. Built-in and custom sinks mix freely,
    // because they all implement the same interface.
    krumb::logger log = krumb::logger_builder()
        .min_level(krumb::level::trace)

        // Built-in console sink with a custom pattern. It is colored automatically when the output is a terminal.
        .add_sink(std::make_shared<krumb::console_sink>(std::make_unique<krumb::pattern_formatter>("{LEVEL:>8} {message}")), krumb::level::info)

        // Built-in file sink with a custom formatter.
        .add_sink(std::make_shared<krumb::file_sink>("custom_sink.log", std::make_unique<logfmt_formatter>(), krumb::file_sink::mode::truncate))

        // Custom sinks.
        .add_sink(recent, krumb::level::warn)
        .add_sink(counter)

        .on_error([](std::string_view description) {
            std::cerr << "logging problem: " << description << "\n";
        })
        .build();

    log.log(krumb::level::trace, "starting up");
    log.log(krumb::level::debug, "loading configuration from {}", "app.conf");
    log.log(krumb::level::info, "listening on port {}", 8080);
    log.log(krumb::level::warn, "disk usage at {}%", 91);
    log.log(krumb::level::error, "request failed: \"{}\"", "timeout");
    log.log(krumb::level::critical, "shutting down");

    // Read back what the custom sinks collected.
    std::cout << "\nrecent lines (warn and above, last 3):\n";

    for (const std::string& line : recent->snapshot()) {
        std::cout << "  " << line << "\n";
    }

    std::cout << "\nevents per level:\n";

    for (const krumb::level lvl : {krumb::level::trace, krumb::level::debug, krumb::level::info, krumb::level::warn, krumb::level::error, krumb::level::critical}) {
        std::cout << "  " << krumb::to_string(lvl) << ": " << counter->count(lvl) << "\n";
    }

    return 0;
}

int main() {
    // Opening the log file can fail, and that is reported by an exception at construction time.
    try {
        return run();
    } catch (const std::exception& ex) {
        std::cerr << "cannot set up logging: " << ex.what() << "\n";

        return 1;
    }
}
