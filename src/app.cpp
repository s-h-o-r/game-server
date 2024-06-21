#include "app.h"

namespace app {

using namespace std::literals;

std::string GetMapError::what() const {
    switch (reason) {
        case GetMapErrorReason::mapNotFound:
            return "Map not found"s;
    }
    throw std::runtime_error("Unknown GetMapError");
}

const model::Map* GetMapUseCase::GetMap(model::Map::Id map_id) const {
    const model::Map* map = game_->FindMap(map_id);
    if (map == nullptr) {
        throw GetMapError{GetMapErrorReason::mapNotFound};
    }
    return map;
}

const model::Game::Maps& ListMapsUseCase::GetMapsList() const {
    return game_->GetMaps();
}

std::string ListPlayersError::what() const {
    switch (reason) {
        case ListPlayersErrorReason::unknownToken:
            return "Player token has not been found"s;
    }
    throw std::runtime_error("Unknown ListPlayersError");
}

const model::GameSession::IdToDogIndex& GetPlayersInfoUseCase::GetPlayersList(std::string_view token) const {
    const user::Player* player = tokens_->FindPlayerByToken(user::Token{std::string(token)});
    if (player == nullptr) {
        throw ListPlayersError{ListPlayersErrorReason::unknownToken};
    }
    return player->GetGameSession()->GetDogs();
}

const model::GameSession* GetPlayersInfoUseCase::GetPlayerGameSession(std::string_view token) const {
    const user::Player* player = tokens_->FindPlayerByToken(user::Token{std::string(token)});
    if (player == nullptr) {
        throw ListPlayersError{ListPlayersErrorReason::unknownToken};
    }
    return tokens_->FindPlayerByToken(user::Token{std::string(token)})->GetGameSession();
}

std::string JoinGameError::what() const {
    switch (reason) {
        case JoinGameErrorReason::invalidMap:
            return "Map not found"s;
        case JoinGameErrorReason::invalidName:
            return "Invalid name"s;
    }
    throw std::runtime_error("Unknown JoinGameError");
}

JoinGameResult JoinGameUseCase::JoinGame(const std::string& user_name, const std::string& map_id) {
    if (user_name.empty()) {
        throw JoinGameError{JoinGameErrorReason::invalidName};
    }

    const model::Map* map = game_->FindMap(model::Map::Id{map_id});

    if (!map) {
        throw JoinGameError{JoinGameErrorReason::invalidMap};
    }

    model::GameSession* session = game_->GetGameSession(model::Map::Id{map_id});
    if (session == nullptr) {
        session = &game_->StartGameSession(map);
    }

    model::Dog* dog = session->AddDog(user_name);
    user::Token player_token = tokens_->AddPlayer(&players_->Add(dog, session));

    return {player_token, dog->GetId()};
}

bool ManageDogActionsUseCase::MoveDog(std::string_view token, std::string_view move) {
    user::Player* player = tokens_->FindPlayerByToken(user::Token{std::string(token)});
    if (player == nullptr) {
        throw ListPlayersError{ListPlayersErrorReason::unknownToken};
    }

    double map_speed = player->GetGameSession()->GetMap()->GetSpeed();
    model::Dog* dog = player->GetDog();

    if (move.empty()) {
        dog->Stop();
    } else if (move == "L"sv) {
        dog->SetSpeed({-map_speed, 0});
        dog->SetDirection(model::Direction::WEST);
    } else if (move == "R"sv) {
        dog->SetSpeed({map_speed, 0});
        dog->SetDirection(model::Direction::EAST);
    } else if (move == "U"sv) {
        dog->SetSpeed({0, -map_speed});
        dog->SetDirection(model::Direction::NORTH);
    } else if (move == "D"sv) {
        dog->SetSpeed({0, map_speed});
        dog->SetDirection(model::Direction::SOUTH);
    } else {
        return false;
    }

    return true;
}

void ProcessTickUseCase::ProcessTick(std::int64_t tick) {
    game_->UpdateState(tick);
}

void DeletePlayerUseCase::DeletePlayer(const std::string& token) {
    user::Player* player = player_tokens_->FindPlayerByToken(user::Token{token});
    game_->GetGameSession(player->GetGameSession()->GetMap()->GetId())->DeleteDog(player->GetDog()->GetId());
    players_->Delete(player);
    player_tokens_->DeletePlayer(user::Token{token});
}

void LeaderboardUseCase::SaveToLeaderboard(const std::string& name, std::uint16_t score, std::uint16_t time_in_game_ms) {
    leaderboard_->SaveRetiredPlayer(name, score, time_in_game_ms);
}

std::vector<domain::RetiredPlayer> LeaderboardUseCase::GetLeaders(size_t start, size_t max_players) {
    return leaderboard_->GetLeaders(start, max_players);
}

const model::Game::Maps& Application::ListMaps() const {
    return list_maps_use_case_.GetMapsList();
}

const model::Map* Application::FindMap(model::Map::Id map_id) const {
    return get_map_use_case_.GetMap(map_id);
}

const model::GameSession* Application::GetPlayerGameSession(std::string_view token) const {
    return get_players_info_use_case_.GetPlayerGameSession(token);
}

const model::GameSession::IdToDogIndex& Application::ListPlayers(std::string_view token) const {
    return get_players_info_use_case_.GetPlayersList(token);
}

JoinGameResult Application::JoinGame(const std::string& user_name, const std::string& map_id) {
    auto join_result = join_game_use_case_.JoinGame(user_name, map_id);
    NotifyListenersJoin(*join_result.token, tokens_.FindPlayerByToken(join_result.token)->GetDog());
    return join_result;
}

bool Application::MoveDog(std::string_view token, std::string_view move) {
    return manage_dog_actions_use_case_.MoveDog(token, move);
}

void Application::ProcessTick(std::int64_t tick) {
    NotifyListenersTick(tick);
    process_tick_use_case_.ProcessTick(tick);
}

void Application::DeletePlayer(const std::string& player_token) {
    delete_player_use_case_.DeletePlayer(player_token);
}

void Application::SaveToLeaderboard(const std::string& name, std::uint16_t score, std::uint16_t time_in_game_ms) {
    leaderboard_use_case_.SaveToLeaderboard(name, score, time_in_game_ms);
}

std::vector<domain::RetiredPlayer> Application::GetLeaders(size_t start, size_t max_players) {
    return leaderboard_use_case_.GetLeaders(start, max_players);
}

bool Application::IsTokenValid(std::string_view token) const {
    return tokens_.FindPlayerByToken(user::Token{std::string(token)});
}

void Application::SetListener(ApplicationListener* listener) {
    if (listener != nullptr) {
        listeners_.push_back(listener);
    }
}

void Application::NotifyListenersTick(std::int64_t tick) const {
    for (auto* listener : listeners_) {
        if (listener != nullptr) {
            listener->OnTick(std::chrono::milliseconds{tick});
        }
    }
}

void Application::NotifyListenersJoin(std::string token, model::Dog* dog) const {
    for (auto* listener : listeners_) {
        if (listener != nullptr) {
            listener->OnJoin(token, dog);
        }
    }
}

} // namespace app
