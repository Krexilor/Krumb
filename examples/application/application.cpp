// STANDARD HEADERS --------------------------------------------------------------------------------------------------------------------------------|
#include <memory>
#include <utility>
#include <iostream>
#include <exception>
#include <string_view>

// PROJECT HEADERS ---------------------------------------------------------------------------------------------------------------------------------|
#include <krumb/krumb.hpp>

// ORDER SERVICE -----------------------------------------------------------------------------------------------------------------------------------|
namespace {
    // A component that receives its logger from the outside. a logger is a cheap handle, so it is stored by value.
    class order_service {
    public:
        explicit order_service(krumb::logger log)
            : log_(std::move(log)) {
        }

        void place(int order_id, double amount) {
            log_.log(krumb::level::debug, "placing order {} for {:.2f}", order_id, amount);

            if (amount < 0.0) {
                log_.log(krumb::level::error, "order {} rejected: negative amount", order_id);
                return;
            }

            if (amount > 1000.0) {
                log_.log(krumb::level::warn, "order {} is unusually large: {:.2f}", order_id, amount);
            }

            log_.log(krumb::level::info, "order {} placed", order_id);
        }

    private:
        krumb::logger log_;
    };
}

// APPLICATION -------------------------------------------------------------------------------------------------------------------------------------|
// A typical application setup: one logger with a console and a file, built once at startup.
int run() {
    // 1. Build a logger with two destinations, each with its own threshold.
    // the logger level (debug) applies first, then each sink drops what is below its own threshold.
    // the console shows info and above. the file keeps everything that passes the logger level.
    // messages at warn and above are flushed to the destinations immediately.
    krumb::logger app_log = krumb::logger_builder()
        .min_level(krumb::level::debug)
        .flush_level(krumb::level::warn)
        .add_sink(std::make_shared<krumb::console_sink>(), krumb::level::info)
        .add_sink(std::make_shared<krumb::file_sink>("application.log", krumb::file_sink::mode::truncate))
        .on_error([](std::string_view description) {
            std::cerr << "logging problem: " << description << "\n";
        })
        .build();

    // 2. Pass the logger to the components that need it.
    order_service orders(app_log);

    orders.place(1, 49.99);
    orders.place(2, 2500.00);
    orders.place(3, -5.00);

    // 3. Make it the default logger, so the free functions use the same destinations.
    krumb::set_default_logger(app_log);
    krumb::info("free functions now use the application logger");

    // 4. Quiet mode: raising the level at runtime affects every handle of this logger, including the one inside the service.
    app_log.set_level(krumb::level::warn);

    orders.place(4, 10.00);
    orders.place(5, 5000.00);

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
