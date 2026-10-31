#pragma once

// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <mutex>
#include <memory>
#include <string>
#include <string_view>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/sinks/sink.hpp>
#include <krumb/formatters/formatter.hpp>

// LINE SINK ---------------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    class line_sink : public sink {
        public:
        line_sink();

        explicit line_sink(std::unique_ptr<formatter> fmt);

        void write(const event& ev) final;
        void flush() final;

        protected:
        virtual void output(level lvl, std::string_view line) = 0;
        virtual void flush_output() = 0;

        private:
        std::mutex mutex_;
        std::unique_ptr<formatter> formatter_;
        std::string buffer_;
    };
}
