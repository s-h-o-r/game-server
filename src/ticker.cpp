#include "ticker.h"

#include <cassert>

namespace tick {

using namespace std::chrono;

void Ticker::Start() {
    net::dispatch(strand_, [self = shared_from_this()] {
        self->last_tick_ = steady_clock::now();
        self->ScheduleTick();
    });
}

void Ticker::ScheduleTick() {
    timer_.expires_after(period_);
    timer_.async_wait([self = shared_from_this()](sys::error_code ec) {
        self->OnTick(ec);
    });
}

void Ticker::OnTick(sys::error_code ec) {
    if (!ec) {
        auto this_tick = steady_clock::now();
        auto delta = duration_cast<milliseconds>(this_tick - last_tick_);
        last_tick_ = this_tick;
        try {
            handler_(delta);
        } catch (...) {
        }
        ScheduleTick();
    }
}

} // namespace tick
