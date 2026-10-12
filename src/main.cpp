#include "crypto/binance_feed.hpp"
#include "crypto/config.hpp"
#include "crypto/binance_rest.hpp"
#include "crypto/logger.hpp"
#include "crypto/order_book.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <csignal>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
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
        std::map<std::string, crypto::OrderBook> books;
        std::thread feed_thread;
        if (run_live) {
            feed_thread = std::thread([&config, &updates]() {
                crypto::BinanceFeed{}.run(config, updates, shutdown_requested);
            });
            for (const auto& symbol : config.symbols) {
                auto [book, inserted] = books.emplace(symbol, crypto::OrderBook{symbol});
                (void)inserted;
                crypto::BookSnapshot snapshot;
                if (!crypto::fetch_snapshot(symbol, snapshot) || !book->second.apply_snapshot(snapshot)) {
                    shutdown_requested.store(true);
                    updates.close();
                    if (feed_thread.joinable()) {
                        feed_thread.join();
                    }
                    throw std::runtime_error("unable to initialize order book for " + symbol);
                }
                crypto::log(crypto::LogLevel::info, "order-book", symbol + " snapshot initialized");
            }
        }
        crypto::log(crypto::LogLevel::info, "health", run_once ? "foundation smoke run healthy" : "foundation running");

        while (!run_once && !shutdown_requested.load()) {
            crypto::DepthUpdate update;
            while (updates.try_pop(update)) {
                const auto book = books.find(update.symbol);
                if (book == books.end()) {
                    crypto::log(crypto::LogLevel::warning, "order-book", "received update for unknown symbol " + update.symbol);
                    continue;
                }
                const auto result = book->second.apply_update(update);
                if (result == crypto::BookUpdateResult::applied) {
                    const auto mid_price = book->second.mid_price();
                    crypto::log(crypto::LogLevel::info, "order-book", update.symbol + " updated mid=" + std::to_string(mid_price.value_or(0.0)));
                } else if (result == crypto::BookUpdateResult::sequence_gap) {
                    crypto::log(crypto::LogLevel::warning, "order-book", update.symbol + " sequence gap; book_id=" +
                        std::to_string(book->second.last_update_id()) + " update_range=" +
                        std::to_string(update.first_update_id) + "-" + std::to_string(update.final_update_id));
                    crypto::BookSnapshot recovery_snapshot;
                    if (crypto::fetch_snapshot(update.symbol, recovery_snapshot) && book->second.apply_snapshot(recovery_snapshot)) {
                        crypto::log(crypto::LogLevel::info, "order-book", update.symbol + " recovered from REST snapshot");
                    } else {
                        crypto::log(crypto::LogLevel::warning, "order-book", update.symbol + " REST recovery failed");
                    }
                } else if (result == crypto::BookUpdateResult::invalid_update) {
                    crypto::log(crypto::LogLevel::warning, "order-book", update.symbol + " invalid update");
                }
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