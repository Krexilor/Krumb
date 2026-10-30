#pragma once

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/core/event.hpp>

// SINK --------------------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    class sink {
        // A destination for log events. A sink formats the even itself, so each sink can choose its own format.
        public:
        sink() = default;
        virtual ~sink() = default;

        sink(const sink&) = delete;
        sink& operator = (const sink&) = delete;

        virtual void write(const event& ev) = 0;
        virtual void flush() = 0;
    };
}
