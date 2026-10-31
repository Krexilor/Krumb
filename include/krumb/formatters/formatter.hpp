#pragma once

// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <string>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/core/event.hpp>

// FORMATTER ---------------------------------------------------------------------------------------------------------------------------------------|
namespace krumb {
    // Turns an event into text. A sink owns its formatter and calls it from one thread at a time,
    // so an implementation needs no locking and may keep scratch state.
    class formatter {
        public:
        formatter() = default;
        virtual ~formatter() = default;

        formatter(const formatter&) = delete;
        formatter& operator = (const formatter&) = delete;

        virtual void format(const event& ev, std::string& out) = 0;
    };
}
