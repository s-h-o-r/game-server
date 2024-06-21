#include "sdk.h"

#include <boost/archive/binary_iarchive.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/system/errc.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>

#include "app.h"
#include "cl_parser.h"
#include "json_loader.h"
#include "./leaderboard/leaderboard.h"
#include "logger.h"
#include "model.h"
#include "model_serialization.h"
#include "retirement_detector.h"
#include "request_handler.h"
#include "ticker.h"

using namespace std::literals;
namespace net = boost::asio;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    #ifndef __clang__
    std::vector<std::jthread> workers;
    #else
    std::vector<std::thread> workers;
    #endif
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
    #ifdef __clang__
    for (auto& worker : workers) {
        worker.join();
    }
    #endif
}

constexpr const char DB_URL_ENV_NAME[]{"GAME_DB_URL"};

leaderboard::LeaderboardConfig GetConfigFromEnv() {
    leaderboard::LeaderboardConfig config;
    const unsigned num_threads = std::thread::hardware_concurrency();
    config.connection_pool_capacity = std::max(1u, num_threads);
    if (const auto* url = std::getenv(DB_URL_ENV_NAME)) {
        config.db_url = url;
    } else {
        throw std::runtime_error(DB_URL_ENV_NAME + " environment variable not found"s);
    }
    return config;
}

}  // namespace

int main(int argc, const char* argv[]) {
    cl_parser::Args cl_args;
    try {
        if (auto args = cl_parser::ParseComandLine(argc, argv)) {
            cl_args = *args;
        } else {
            return 0;
        }
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    try {
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(cl_args.config_file_path);
        app::Application app(&game, GetConfigFromEnv());

        if (cl_args.random_spawn_point) {
            game.TurnOnRandomSpawn();
        }

        std::shared_ptr<serialization::SerializationListener> listener{nullptr};
        if (!cl_args.state_file.empty()) {
            std::filesystem::path state_file_path{cl_args.state_file};
            if (std::filesystem::exists(state_file_path)) {
                std::ifstream strm{state_file_path, strm.binary};
                if (strm.is_open()) {
                    try {
                        boost::archive::binary_iarchive i_archive{strm};
                        serialization::ApplicationRepr repr;
                        i_archive >> repr;
                        repr.Restore(&app);
                    } catch (...) {
                        http_logger::LogServerError(boost::system::errc::errc_t::invalid_argument,
                                                    "cannot restore model from save"sv, "restore"sv);
                        throw;
                    }
                    strm.close();
                }
            } else {
                std::filesystem::create_directories(state_file_path.parent_path());
            }
            listener = std::make_shared<serialization::SerializationListener>
                (std::chrono::milliseconds{cl_args.save_state_period}, &app, cl_args.state_file);
        }
        app.SetListener(listener.get());

        // настройка и установка RetirePlayerListener

        std::shared_ptr<retirement::RetirementListener> retirement_listener
        = std::make_shared<retirement::RetirementListener>(game.GetRetirementTime() , &app);
        
        app.SetListener(retirement_listener.get());

        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
            }
        });

        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        auto game_state_strand = net::make_strand(ioc);

        auto handler = std::make_shared<http_handler::RequestHandler>(app, ioc, game_state_strand,
                                                                      std::move(cl_args.static_root), !(static_cast<bool>(cl_args.tick_period)));
        http_logger::InitBoostLogFilter(http_logger::LogFormatter);
        http_logger::LogginRequestHandler<http_handler::RequestHandler> logging_handler(*handler);

        if (cl_args.tick_period != 0) {
            auto tick_period = std::chrono::milliseconds{cl_args.tick_period};
            auto ticker = std::make_shared<tick::Ticker>(game_state_strand, tick_period, [&app](std::chrono::milliseconds delta) {
                app.ProcessTick(delta.count());
            });
            ticker->Start();
        }

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        http_server::ServeHttp(ioc, {address, port}, logging_handler);

        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        http_logger::LogServerStart(port, address.to_string());

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

        if (listener) {
            listener->Serialize();
        }

        http_logger::LogServerEnd(0, ""sv);
    } catch (const std::exception& ex) {
        http_logger::LogServerEnd(EXIT_FAILURE, ex.what());
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}
