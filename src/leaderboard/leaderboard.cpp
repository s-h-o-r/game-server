#include "leaderboard.h"

#include <iostream>


namespace leaderboard {

using namespace std::literals;

void Leaderboard::SaveRetiredPlayer(const std::string& name, std::uint16_t score, std::uint16_t time_in_game_ms) {
    try {
        use_cases_.StartTransaction();
        use_cases_.SaveRetiredPlayer(name, score, time_in_game_ms);
        use_cases_.Commit();
    } catch (const std::exception& e) {
        std::cerr << "Failed to save retired player"sv << std::endl;
        throw e;
    }
}

std::vector<domain::RetiredPlayer> Leaderboard::GetLeaders(size_t start, size_t max_players) {
    try {
        use_cases_.StartTransaction();
        return use_cases_.GetLeaders(start, max_players);
    } catch (const std::exception& e) {
        std::cerr << "Failed to get leaderboard"sv << std::endl;
        throw e;
    }
}

}  // namespace leaderboard
