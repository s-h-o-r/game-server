#pragma once
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>

#include "app.h"
#include "geom.h"
#include "model.h"
#include "player.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <vector>

namespace geom {

template <typename Archive>
void serialize(Archive& ar, Point2D& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

template <typename Archive>
void serialize(Archive& ar, Vec2D& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.x;
    ar& vec.y;
}

}  // namespace geom

namespace model {

template <typename Archive>
void serialize(Archive& ar, model::Loot& obj, [[maybe_unused]] const unsigned version) {
    ar& *obj.id;
    ar& obj.type;
    ar& obj.point;
}

}  // namespace model

namespace serialization {

class BagRepr {
public:
    BagRepr() = default;

    explicit BagRepr(const game_obj::Bag<model::Loot>& bag)
        : loot_(bag.GetAllLoot())
        , capacity_(bag.GetCapacity()) {
    }

    [[nodiscard]] game_obj::Bag<model::Loot> Restore() const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& loot_;
        ar& capacity_;
    }

private:
    std::vector<model::Loot> loot_;
    size_t capacity_ = 0;
};

// DogRepr (DogRepresentation) - сериализованное представление класса Dog
class DogRepr {
public:
    DogRepr() = default;

    explicit DogRepr(model::Dog& dog)
        : id_(dog.GetId())
        , name_(dog.GetName())
        , pos_(dog.GetPosition())
        , speed_(dog.GetSpeed())
        , dir_(dog.GetDirection())
        , bag_(*dog.GetBag())
        , score_(dog.GetScore()) {
    }

    [[nodiscard]] std::shared_ptr<model::Dog> Restore() const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& *id_;
        ar& name_;
        ar& pos_;
        ar& speed_;
        ar& dir_;
        ar& bag_;
        ar& score_;
    }

private:
    model::Dog::Id id_ = model::Dog::Id{0u};
    std::string name_;
    geom::Point2D pos_;
    geom::Vec2D speed_;
    model::Direction dir_ = model::Direction::NORTH;
    BagRepr bag_;
    std::uint16_t score_ = 0;
};

class GameSessionRepr {
public:
    GameSessionRepr() = default;

    explicit GameSessionRepr(const model::GameSession& session);

    [[nodiscard]] std::shared_ptr<model::GameSession> Restore(const model::Game* game) const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& *map_id_;
        ar& dogs_;
        ar& next_dog_id_;
        ar& loot_;
        ar& next_loot_id_;
    }

private:
    model::Map::Id map_id_ = model::Map::Id{""};
    std::vector<DogRepr> dogs_;
    std::uint32_t next_dog_id_ = 0;
    std::vector<std::shared_ptr<model::Loot>> loot_;
    std::uint32_t next_loot_id_ = 0;
};

class GameRepr {
public:
    GameRepr() = default;

    explicit GameRepr(const model::Game& game);

    void Restore(model::Game* game) const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& sessions_;
    }

private:
    std::vector<GameSessionRepr> sessions_;
};

struct PlayerRepr {
    model::Map::Id map_id_ = model::Map::Id{""};
    model::Dog::Id dog_id_ = model::Dog::Id{0u};

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& *map_id_;
        ar& *dog_id_;
    }
};

class PlayersRepr {
public:
    PlayersRepr() = default;
    
    explicit PlayersRepr(const user::Players& players);

    user::Players Restore(model::Game* game) const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& players_;
    }
private:
    std::vector<PlayerRepr> players_;
};

class PlayerTokenRepr {
public:
    PlayerTokenRepr() = default;

    explicit PlayerTokenRepr(const user::PlayerTokens& player_tokens);

    user::PlayerTokens Restore(user::Players* players) const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& token_to_player_;
    }
private:
    std::unordered_map<std::string, PlayerRepr> token_to_player_;
};

class ApplicationRepr {
public:
    ApplicationRepr() = default;

    explicit ApplicationRepr(const app::Application& app)
        : players_(app.players_)
        , player_tokens_(app.tokens_)
        , game_repr_(*app.game_) {

    }

    void Restore(app::Application* app) const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& players_;
        ar& player_tokens_;
        ar& game_repr_;
    }

private:
    PlayersRepr players_;
    PlayerTokenRepr player_tokens_;
    GameRepr game_repr_;
};

class SerializationListener : public app::ApplicationListener {
public:
    explicit SerializationListener(std::chrono::milliseconds save_period, const app::Application* app,
                                   std::filesystem::path state_file_path)
    : save_period_(save_period)
    , app_(app)
    , state_file_path_(std::move(state_file_path)) {
    }

    void Serialize() const;
    void OnTick(std::chrono::milliseconds delta) override;

private:
    std::chrono::milliseconds save_period_;
    std::chrono::milliseconds time_since_save_{0};

    const app::Application* app_;
    std::filesystem::path state_file_path_;
};

}  // namespace serialization
