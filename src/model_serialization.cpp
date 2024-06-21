#include "model_serialization.h"

namespace serialization {

[[nodiscard]] game_obj::Bag<model::Loot> BagRepr::Restore() const {
    game_obj::Bag<model::Loot> bag{capacity_};
    for (const model::Loot& item : loot_) {
        bag.PickUpLoot(item);
    }
    return bag;
}

[[nodiscard]] std::shared_ptr<model::Dog> DogRepr::Restore() const {
    game_obj::Bag<model::Loot> bag = bag_.Restore();
    auto dog_ptr = std::make_shared<model::Dog>(id_, name_, pos_, speed_, bag.GetCapacity());
    dog_ptr->SetDirection(dir_);
    dog_ptr->AddScore(score_);
    auto* dog_bag = dog_ptr->GetBag();
    *dog_bag = std::move(bag);
    return dog_ptr;
}

GameSessionRepr::GameSessionRepr(const model::GameSession& session)
    : map_id_(session.GetMap()->GetId())
    , next_dog_id_(session.GetNextDogId())
    , next_loot_id_(session.GetNextLootId()) {
    for (const auto& [_, dog] : session.GetDogs()) {
        dogs_.push_back(DogRepr(*dog));
    }

    for (const auto& [_, loot_sptr] : session.GetAllLoot()) {
        loot_.push_back(loot_sptr);
    }
}

[[nodiscard]] std::shared_ptr<model::GameSession> GameSessionRepr::Restore(const model::Game* game) const {
    if (game->FindMap(map_id_) == nullptr) {
        throw std::logic_error("there is no map with such id");
    }
    auto session = std::make_shared<model::GameSession>(game->FindMap(map_id_), game->IsDogSpawnRandom(), game->GetLootConfig());

    model::GameSession::IdToDogIndex dog_index;
    for (const DogRepr& dog_repr : dogs_) {
        auto dog = dog_repr.Restore();
        dog_index[dog->GetId()] = dog;
    }

    model::GameSession::IdToLootIndex loot_index;
    for (auto& loot : loot_) {
        loot_index[loot->id] = loot;
    }
    session->Restore(std::move(dog_index), next_dog_id_, std::move(loot_index), next_loot_id_);
    return session;
}

GameRepr::GameRepr(const model::Game& game) {
    for (const auto& [map_id, sessions] : game.GetAllSessions()) {
        for (const auto& session : sessions) {
            sessions_.push_back(GameSessionRepr(*session));
        }
    }
}

void GameRepr::Restore(model::Game* game) const {
    try {
        model::Game::SessionsByMaps sessions;
        for (auto& session_repr : sessions_) {
            auto session_ptr = session_repr.Restore(game);
            sessions[session_ptr->GetMapId()].push_back(session_ptr);
        }
        game->RestoreSessions(std::move(sessions));
    } catch (...) {
        throw std::logic_error("imposible to restore game for this configuration");
    }
}

PlayersRepr::PlayersRepr(const user::Players& players) {
    for (const auto& player : players.GetAllPlayers()) {
        players_.push_back({player->GetGameSession()->GetMapId(), player->GetDog()->GetId()});
    }
}

user::Players PlayersRepr::Restore(model::Game* game) const {
    user::Players restored_players;
    for (const PlayerRepr& player : players_) {
        auto* session = game->GetGameSession(player.map_id_);
        if (session == nullptr) {
            session = &game->StartGameSession(game->FindMap(player.map_id_));
        }
        restored_players.Add(session->GetDog(player.dog_id_),
                             session);
    }
    return restored_players;
}

PlayerTokenRepr::PlayerTokenRepr(const user::PlayerTokens& player_tokens) {
    for (const auto& [token, player_ptr] : player_tokens.token_to_player_) {
        auto res = token_to_player_.emplace(*token, PlayerRepr{player_ptr->GetGameSession()->GetMapId(), player_ptr->GetDog()->GetId()});
        if (!res.second) {
            throw std::logic_error("trying to emplace duplicated token");
        }
    }
}

user::PlayerTokens PlayerTokenRepr::Restore(user::Players* players) const {
    user::PlayerTokens restored_player_tokens;
    for (const auto& [token, player_repr] : token_to_player_) {
        restored_player_tokens.token_to_player_.emplace(token, players->FindByDogIdAndMapId(player_repr.dog_id_,
                                                                                            player_repr.map_id_));
    }
    return restored_player_tokens;
}

void ApplicationRepr::Restore(app::Application* app) const {
    game_repr_.Restore(app->game_);
    app->players_ = players_.Restore(app->game_);
    app->tokens_ = player_tokens_.Restore(&app->players_);
}

void SerializationListener::Serialize() const {
    std::filesystem::create_directories(state_file_path_.parent_path());
    std::filesystem::path tmp_path = state_file_path_.parent_path() / "tmp";
    std::ofstream strm{tmp_path, strm.binary | strm.trunc};

    if (!strm.is_open()) {
        throw std::logic_error("cannot open tmp save file and save game state");
    }

    serialization::ApplicationRepr app_repr(*app_);
    boost::archive::binary_oarchive o_archive{strm};
    o_archive << app_repr;
    strm.close();
    std::filesystem::rename(tmp_path, state_file_path_);
}

void SerializationListener::OnTick(std::chrono::milliseconds delta) {
    time_since_save_ += delta;
    if (time_since_save_ >= save_period_) {
        Serialize();
        save_period_ = std::chrono::milliseconds::zero();
    }
}

}  // namespace serialization
