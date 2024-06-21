#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>

#include <chrono>
#include <memory>

namespace tick {

namespace net = boost::asio;
namespace sys = boost::system;

class Ticker : public std::enable_shared_from_this<Ticker> {
public:
    using Strand = net::strand<net::io_context::executor_type>;
    using Handler = std::function<void(std::chrono::milliseconds delta)>;

    Ticker(Strand& strand, std::chrono::milliseconds period, Handler handler)
        : strand_(strand)
        , period_(period)
        , handler_(std::move(handler)) {
    }

    void Start();

private:
    using Clock = std::chrono::steady_clock;

    Strand& strand_;
    std::chrono::milliseconds period_;
    net::steady_timer timer_{strand_};
    Handler handler_;
    Clock::time_point last_tick_;

    void ScheduleTick();
    void OnTick(sys::error_code ec);
};

} // namespace tick
