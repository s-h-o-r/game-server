#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>

#include <pqxx/pqxx>

#include "../tagged.h"
#include "./app/use_cases_impl.h"
#include "./domain/retired_player.h"
#include "./postgres/postgres.h"
#include "../model.h"

namespace leaderboard {

struct LeaderboardConfig {
    size_t connection_pool_capacity;
    std::string db_url;
};

class Leaderboard {
public:
    explicit Leaderboard(const LeaderboardConfig& config)
    : db_(config.connection_pool_capacity, [&config] { return std::make_shared<pqxx::connection>(config.db_url); })
    , use_cases_(db_) {
    }

    void SaveRetiredPlayer(const std::string& name, std::uint16_t score, std::uint16_t time_in_game_ms);
    std::vector<domain::RetiredPlayer> GetLeaders(size_t start, size_t max_players);
    
private:
    postgres::Database db_;
    app::UseCasesImpl use_cases_;
};

}  // namespace leaderboard
