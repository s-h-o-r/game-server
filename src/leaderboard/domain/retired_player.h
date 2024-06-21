#pragma once
#include <string>
#include <vector>
#include <optional>
#include <numeric>
#include <cstdint>

#include "../util/tagged_uuid.h"

namespace domain {

namespace detail {
struct PlayerTag {};
}  // namespace detail

using PlayerId = util::TaggedUUID<detail::PlayerTag>;

class RetiredPlayer {
public:
    RetiredPlayer(PlayerId player_id, const std::string& name, std::uint16_t score, std::uint16_t play_time_ms)
        : player_id_(player_id)
        , name_(name)
        , score_(score)
        , play_time_ms_(play_time_ms) {
    }

    const PlayerId& GetId() const noexcept {
        return player_id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    std::uint16_t GetScore() const noexcept {
        return score_;
    }

    std::uint16_t GetPlayTimeInMs() const noexcept {
        return play_time_ms_;
    }

private:
    PlayerId player_id_;
    std::string name_;
    std::uint16_t score_;
    std::uint16_t play_time_ms_;
};

class RetiredPlayersRepository {
public:
    virtual void Save(const RetiredPlayer& player) = 0;
    virtual std::vector<RetiredPlayer> GetLeaders(size_t start, size_t max_players) = 0;

protected:
    ~RetiredPlayersRepository() = default;
};

}  // namespace domain
