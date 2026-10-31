// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <mutex>
#include <memory>
#include <utility>
#include <stdexcept>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/sinks/line_sink.hpp>
#include <krumb/formatters/pattern_formatter.hpp>

// LINE SINK ---------------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    line_sink::line_sink() : formatter_(std::make_unique<pattern_formatter>()) {}

    line_sink::line_sink(std::unique_ptr<formatter> fmt) : formatter_(std::move(fmt)) {
        if (!formatter_) {
            throw std::invalid_argument("krumb: line_sink requires a non-null formatter");
        }
    }

    void line_sink::write(const event& ev) {
        std::lock_guard lock(mutex_);

        buffer_.clear();
        formatter_->format(ev, buffer_);
        buffer_.push_back('\n');

        output(ev.lvl, buffer_);
    }

    void line_sink::flush() {
        std::lock_guard lock(mutex_);
        flush_output();
    }
}
