// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <memory>
#include <stdexcept>
#include <utility>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/logger_builder.hpp>
#include <krumb/sinks/console_sink.hpp>

// LOGGER BUILDER ----------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    logger_builder& logger_builder::add_sink(std::shared_ptr<sink> target, level threshold) {
        if (!target) {
            throw std::invalid_argument("krumb: logger_builder::add_sink requires a non-null sink");
        }

        sinks_.push_back(detail::sink_entry{std::move(target), threshold});

        return *this;
    }

    logger_builder& logger_builder::on_error(error_handler handler) {
        handler_ = std::move(handler);

        return *this;
    }

    logger logger_builder::build() const {
        auto state = std::make_shared<detail::logger_state>();

        state->active_level.store(min_level_);
        state->flush_level.store(flush_level_);
        state->sinks = sinks_;
        state->handler = handler_;

        if (state->sinks.empty()) {
            state->sinks.push_back(detail::sink_entry{std::make_shared<console_sink>(), level::trace});
        }

        state->min_threshold = state->sinks.front().threshold;

        for (const detail::sink_entry& entry : state->sinks) {
            if (entry.threshold < state->min_threshold) {
                state->min_threshold = entry.threshold;
            }
        }

        return logger{std::move(state)};
    }
}
