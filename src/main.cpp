#include "crypto/binance_feed.hpp"
#include "crypto/config.hpp"
#include "crypto/logger.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <thread>

namespace {
std::atomic_bool shutdown_requested{false};

void request_shutdown(int) {
    shutdown_requested.store(true);
}
}

int main(int argc, char* argv[]) {
    try {
        std::filesystem::path config_path = "config/default.toml";
        bool run_once = false;
        bool run_live = false;
        for (int index = 1; index < argc; ++index) {
            const std::string_view argument{argv[index]};
            if (argument == "--once") {
                run_once = true;
            } else if (argument == "--live") {
                run_live = true;
            } else if (argument == "--config" && index + 1 < argc) {
                config_path = argv[++index];
            } else if (argument == "--help") {
                std::cout << "Usage: crypto_signal_engine [--config path] [--once] [--live]\n";
                return EXIT_SUCCESS;
            } else {
                std::cerr << "unknown argument: " << argument << '\n';
                return EXIT_FAILURE;
            }
        }

        const auto config = crypto::Config::load(config_path);
        std::signal(SIGINT, request_shutdown);
        std::signal(SIGTERM, request_shutdown);
        if (run_live && run_once) {
            throw std::invalid_argument("--live and --once cannot be used together");
        }
        crypto::log(crypto::LogLevel::info, "startup", run_live ? "starting live Binance market-data feed" : "configuration loaded");
        crypto::SpscQueue<crypto::DepthUpdate, 4096> updates;
        std::thread feed_thread;
        if (run_live) {
            feed_thread = std::thread([&config, &updates]() {
                crypto::BinanceFeed{}.run(config, updates, shutdown_requested);
            });
        }
        crypto::log(crypto::LogLevel::info, "health", run_once ? "foundation smoke run healthy" : "foundation running");

        while (!run_once && !shutdown_requested.load()) {
            crypto::DepthUpdate update;
            while (updates.try_pop(update)) {
                crypto::log(crypto::LogLevel::info, "market-data", update.symbol + " depth update received");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        updates.close();
        if (feed_thread.joinable()) {
            feed_thread.join();
        }
        crypto::log(crypto::LogLevel::info, "shutdown", "service stopped");
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        crypto::log(crypto::LogLevel::error, "startup", error.what());
        return EXIT_FAILURE;
    }
}