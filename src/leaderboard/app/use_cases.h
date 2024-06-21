#pragma once

#include "../domain/retired_player.h"

#include <string>
#include <vector>

namespace app {

class UseCases {
public:

    virtual domain::PlayerId SaveRetiredPlayer(const std::string& name, std::uint16_t score, std::uint16_t play_time_in_ms) = 0;
    virtual std::vector<domain::RetiredPlayer> GetLeaders(size_t start, size_t max_players) = 0;

    virtual void StartTransaction() = 0;
    virtual void Commit() = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app
