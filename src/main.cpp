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
        for (int index = 1; index < argc; ++index) {
            const std::string_view argument{argv[index]};
            if (argument == "--once") {
                run_once = true;
            } else if (argument == "--config" && index + 1 < argc) {
                config_path = argv[++index];
            } else if (argument == "--help") {
                std::cout << "Usage: crypto_signal_engine [--config path] [--once]\n";
                return EXIT_SUCCESS;
            } else {
                std::cerr << "unknown argument: " << argument << '\n';
                return EXIT_FAILURE;
            }
        }

        const auto config = crypto::Config::load(config_path);
        std::signal(SIGINT, request_shutdown);
        std::signal(SIGTERM, request_shutdown);
        crypto::log(crypto::LogLevel::info, "startup", "configuration loaded; market-data feeds are not enabled yet");
        crypto::log(crypto::LogLevel::info, "health", run_once ? "foundation smoke run healthy" : "foundation running");

        while (!run_once && !shutdown_requested.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        crypto::log(crypto::LogLevel::info, "shutdown", "service stopped");
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        crypto::log(crypto::LogLevel::error, "startup", error.what());
        return EXIT_FAILURE;
    }
}