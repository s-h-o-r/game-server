#pragma once

#include <numeric>
#include <random>
#include <unordered_map>
#include <vector>

#include "model.h"
#include "tagged.h"
namespace serialization {
class PlayerTokenRepr;
} // namespace serialization

namespace user {
namespace detail {
struct TokenTag {};
} // namespace model::detail

using Token = util::Tagged<std::string, detail::TokenTag>;

class Player {
public:
    Player() = delete;
    explicit Player(const model::GameSession* session, model::Dog* dog)
        : session_(session)
        , dog_(dog) {
    }

    model::Dog* GetDog();
    const model::Dog* GetDog() const;
    const model::GameSession* GetGameSession() const;

private:
    const model::GameSession* session_;
    model::Dog* dog_;
};


class PlayerTokens {
public:
    friend class serialization::PlayerTokenRepr; // breaching encapsulation but it's the best idea I had

    PlayerTokens() = default;

    PlayerTokens(PlayerTokens&& other)
    : token_to_player_(std::move(other.token_to_player_)) {
    }

    PlayerTokens& operator=(PlayerTokens&& other) {
        token_to_player_ = std::move(other.token_to_player_);
        return *this;
    }

    PlayerTokens(const PlayerTokens&) = delete;
    PlayerTokens& operator=(const PlayerTokens&) = delete;

    Token AddPlayer(Player* player);
    void DeletePlayer(const Token& token);
    Player* FindPlayerByToken(const Token& token);
    const Player* FindPlayerByToken(const Token& token) const;

private:
    std::random_device random_device_;
    std::mt19937_64 generator1_{[this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }()};
    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};

    using TokenHasher = util::TaggedHasher<Token>;
    using TokenToPlayer = std::unordered_map<Token, Player*, TokenHasher>;

    TokenToPlayer token_to_player_;

    Token GenerateUniqueToken();
};

class Players {
public:
    using PlayersList = std::vector<std::shared_ptr<Player>>;

    Player& Add(model::Dog* dog, const model::GameSession* session);
    void Delete(Player* player);
    Player* FindByDogIdAndMapId(model::Dog::Id dog_id, model::Map::Id map_id);
    const PlayersList& GetAllPlayers() const;
private:
    using MapIdHasher = util::TaggedHasher<model::Map::Id>;
    using DogIdHasher = util::TaggedHasher<model::Dog::Id>;
    using MapToDogToPlayerIndex = std::unordered_map<model::Map::Id, std::unordered_map<model::Dog::Id, Player*,
                                    DogIdHasher>,
                                    MapIdHasher>;

    PlayersList players_;
    MapToDogToPlayerIndex map_to_dog_to_player_;
};
}
