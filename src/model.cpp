#include "model.h"

#include <algorithm>
#include <iostream>
#include <cmath>
#include <random>
#include <stdexcept>

namespace model {
using namespace std::literals;

std::string DirectionToString(Direction dir) {
    switch (dir) {
        case Direction::NORTH:
            return "U"s;
        case Direction::SOUTH:
            return "D"s;
        case Direction::EAST:
            return "R"s;
        case Direction::WEST:
            return "L"s;
    }
    throw std::runtime_error("Unknown direction status in Dog class"s);
}

Road::Road(HorizontalTag, geom::Point start, geom::Coord end_x) noexcept
    : start_{start}
    , end_{end_x, start.y} {
}

Road::Road(VerticalTag, geom::Point start, geom::Coord end_y) noexcept
    : start_{start}
    , end_{start.x, end_y} {
}

bool Road::IsHorizontal() const noexcept {
    return start_.y == end_.y;
}

bool Road::IsVertical() const noexcept {
    return start_.x == end_.x;
}

geom::Point Road::GetStart() const noexcept {
    return start_;
}

geom::Point Road::GetEnd() const noexcept {
    return end_;
}

bool Road::IsDogOnRoad(geom::Point2D dog_point) const {
    double right_edge = GetRightEdge();
    double left_edge = GetLeftEdge();
    double upper_edge = GetUpperEdge();
    double bottom_edge = GetBottomEdge();

    return dog_point.x >= left_edge && dog_point.x <= right_edge
        && dog_point.y >= upper_edge && dog_point.y <= bottom_edge;
}

double Road::GetLeftEdge() const {
    double border_offset = 0.4;
    return std::min(start_.x, end_.x) - border_offset;
}

double Road::GetRightEdge() const {
    double border_offset = 0.4;
    return std::max(start_.x, end_.x) + border_offset;
}

double Road::GetUpperEdge() const {
    double border_offset = 0.4;
    return std::min(start_.y, end_.y) - border_offset;
}

double Road::GetBottomEdge() const {
    double border_offset = 0.4;
    return std::max(start_.y, end_.y) + border_offset;
}

const geom::Rectangle& Building::GetBounds() const noexcept {
    return bounds_;
}

const Office::Id& Office::GetId() const noexcept {
    return id_;
}

geom::Point Office::GetPosition() const noexcept {
    return position_;
}

geom::Offset Office::GetOffset() const noexcept {
    return offset_;
}

double Office::GetWidth() const noexcept {
    return width_;
}

const Map::Id& Map::GetId() const noexcept {
    return id_;
}

const std::string& Map::GetName() const noexcept {
    return name_;
}

double Map::GetSpeed() const noexcept {
    return dog_speed_;
}

const Map::Buildings& Map::GetBuildings() const noexcept {
    return buildings_;
}

const Map::Roads& Map::GetRoads() const noexcept {
    return roads_;
}

const Map::Offices& Map::GetOffices() const noexcept {
    return offices_;
}

const std::vector<extra_data::LootType>& Map::GetLootTypes() const noexcept {
    return loot_types_;
}

size_t Map::GetBagCapacity() const noexcept {
    return bag_capacity_;
}

unsigned Map::GetLootScore(std::uint8_t loot_type) const noexcept {
    return loot_type_to_score_.at(loot_type);
}

void Map::SetDogSpeed(double speed) {
    dog_speed_ = speed;
}

void Map::AddRoad(Road&& road) {
    roads_.push_back(std::move(road));
    Road& new_road = roads_.back();
    if (new_road.IsVertical()) {
        vertical_road_index_[new_road.GetStart().x].push_back(&new_road);
    } else {
        horizontal_road_index_[new_road.GetStart().y].push_back(&new_road);
    }
}

void Map::AddBuilding(Building&& building) {
    buildings_.emplace_back(std::move(building));
}

void Map::AddOffice(Office&& office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

void Map::AddLootType(extra_data::LootType&& loot_type, unsigned score) {
    loot_types_.emplace_back(std::move(loot_type));
    loot_type_to_score_.insert({static_cast<std::uint8_t>(loot_types_.size() - 1), score});
}

void Map::SetBagCapacity(size_t bag_capacity) {
    bag_capacity_ = bag_capacity;
}

geom::Point2D Map::GetRandomPoint() const {
    if (roads_.size() == 0) {
        throw std::logic_error("No roads on map to generate random road"s);
    }

    std::random_device random_device;
    std::mt19937 generator{random_device()};

    std::uniform_int_distribution int_dist(0, static_cast<int>(roads_.size() - 1));

    const Road& rand_road = roads_.at(int_dist(generator));

    if (rand_road.IsHorizontal()) {
        std::uniform_real_distribution double_dist(std::min(rand_road.GetStart().x, rand_road.GetEnd().x) + 0.0,
                                                   std::max(rand_road.GetStart().x, rand_road.GetEnd().x) + 0.0);
        return {static_cast<double>(double_dist(generator)), static_cast<double>(rand_road.GetStart().y)};
    }
    
    std::uniform_real_distribution double_dist(std::min(rand_road.GetStart().y, rand_road.GetEnd().y) + 0.0,
                                               std::max(rand_road.GetStart().y, rand_road.GetEnd().y) + 0.0);
    return {static_cast<double>(rand_road.GetStart().x), static_cast<double>(double_dist(generator))};
}

geom::Point2D Map::GetDefaultSpawnPoint() const {
    geom::Point first_road_start = roads_.front().GetStart();
    return {static_cast<double>(first_road_start.x),
            static_cast<double>(first_road_start.y)};
}

const Road* Map::GetVerticalRoad(geom::Point2D dog_point) const {
    geom::Point map_point = ConvertToMapPoint(dog_point);

    if (vertical_road_index_.contains(map_point.x)) {
        const auto& roads = vertical_road_index_.at(map_point.x);
        return *std::find_if(roads.begin(), roads.end(), [&dog_point](const Road* road) {
            return road->IsDogOnRoad(dog_point);
        });
    }
    return nullptr;
}

const Road* Map::GetHorizontalRoad(geom::Point2D dog_point) const {
    geom::Point map_point = ConvertToMapPoint(dog_point);
    if (horizontal_road_index_.contains(map_point.y)) {
        const auto& roads = horizontal_road_index_.at(map_point.y);
        return *std::find_if(roads.begin(), roads.end(), [&dog_point](const Road* road) {
            return road->IsDogOnRoad(dog_point);
        });
    }
    return nullptr;
}

std::vector<const Road*> Map::GetRelevantRoads(geom::Point2D dog_point) const {
    std::vector<const Road*> relevant_road;
    for (auto& road : roads_) {
        if (road.IsDogOnRoad(dog_point)) {
            relevant_road.push_back(&road);
        }
    }
    return relevant_road;
}

const std::string& Dog::GetName() const noexcept {
    return name_;
}

const Dog::Id& Dog::GetId() const noexcept {
    return id_;
}

void Dog::SetName(std::string_view name) {
    name_ = std::string(name);
}

void Dog::SetPosition(geom::Point2D new_pos) {
    prev_pos_ = pos_;
    pos_ = new_pos;
}

const geom::Point2D& Dog::GetPosition() const {
    return pos_;
}

const geom::Point2D& Dog::GetPreviousPosition() const {
    return prev_pos_;
}

void Dog::SetSpeed(geom::Vec2D new_speed) {
    speed_ = new_speed;
}

const geom::Vec2D& Dog::GetSpeed() const {
    return speed_;
}

void Dog::SetDirection(Direction new_dir) {
    dir_ = new_dir;
}

Direction Dog::GetDirection() const {
    return dir_;

}

double Dog::GetWidth() const noexcept {
    return width_;
}

void Dog::Stop() {
    speed_ = {0, 0};
}

bool Dog::IsStopped() const {
    return std::fabs(speed_.x) < std::numeric_limits<double>::epsilon()
        && std::fabs(speed_.y) < std::numeric_limits<double>::epsilon();
}

game_obj::Bag<Loot>* Dog::GetBag() {
    return &bag_;
}

void Dog::AddScore(std::uint16_t score_to_add) {
    score_ += score_to_add;
}

std::uint16_t Dog::GetScore() const {
    return score_;
}

LootOfficeDogProvider::LootOfficeDogProvider(const Map::Offices& offices) {
    for (const auto& office : offices) {
        items_.push_back(&office);
    }
}

size_t LootOfficeDogProvider::ItemsCount() const {
    return items_.size();
}

collision_detector::Item LootOfficeDogProvider::GetItem(size_t idx) const {
    if (std::holds_alternative<const Office*>(items_.at(idx))) {
        const Office* office_ptr = std::get<const Office*>(items_.at(idx));
        const geom::Point& position = office_ptr->GetPosition();
        return {{static_cast<double>(position.x), static_cast<double>(position.y)}, office_ptr->GetWidth()};
    } else if (std::holds_alternative<const Loot*>(items_.at(idx))) {
        const Loot* loot = std::get<const Loot*>(items_.at(idx));
        double item_width = 0.;
        return {loot->point, item_width};
    } else {
        throw std::logic_error("unknown variant type");
    }
}

size_t LootOfficeDogProvider::GatherersCount() const {
    return gatherers_.size();
}

collision_detector::Gatherer LootOfficeDogProvider::GetGatherer(size_t idx) const {
    auto dog = gatherers_.at(idx);
    return {dog->GetPreviousPosition(), dog->GetPosition(), dog->GetWidth()};
}

void LootOfficeDogProvider::PushBackLoot(const Loot* loot) {
    items_.push_back(loot);
}

void LootOfficeDogProvider::EraseLoot(size_t idx) {
    items_.erase(items_.begin() + idx);
}

const std::variant<const Office*, const Loot*>& LootOfficeDogProvider::GetRawLootVal(size_t idx) const {
    return items_.at(idx);

}

void LootOfficeDogProvider::AddGatherer(Dog* gatherer) {
    gatherers_.push_back(gatherer);
}

void LootOfficeDogProvider::EraseGatherer(Dog* gatherer) {
    gatherers_.erase(std::find(gatherers_.begin(), gatherers_.end(), gatherer));
}

const Dog* LootOfficeDogProvider::GetDog(size_t idx) const {
    return gatherers_.at(idx);
}

Dog* LootOfficeDogProvider::GetDog(size_t idx) {
    return gatherers_.at(idx);
}

const Map::Id& GameSession::GetMapId() const {
    return map_->GetId();
}

const model::Map* GameSession::GetMap() const {
    return map_;
}

Dog* GameSession::AddDog(std::string_view name) {
    geom::Vec2D default_speed = {0, 0};

    geom::Point2D start_point;
    if (random_dog_spawn_) {
        start_point = map_->GetRandomPoint();
    } else {
        start_point = map_->GetDefaultSpawnPoint();
    }

    geom::Point2D dog_pos = {static_cast<double>(start_point.x),
                                static_cast<double>(start_point.y)}; // map_->GetRandomDogPoint();

    auto dog = std::make_shared<Dog>(Dog::Id{next_dog_id_++}, std::string(name), dog_pos, default_speed, map_->GetBagCapacity());
    auto dog_id = dog->GetId();
    dogs_.emplace(dog_id, dog);
    items_gatherer_provider_.AddGatherer(dog.get());
    return dogs_.at(dog_id).get();
}

void GameSession::DeleteDog(const Dog::Id& id) {
    items_gatherer_provider_.EraseGatherer(dogs_.at(id).get());
    dogs_.erase(id);
}

const Dog* GameSession::GetDog(Dog::Id id) const {
    return dogs_.at(id).get();
}

Dog* GameSession::GetDog(Dog::Id id) {
    return dogs_.at(id).get();
}

const GameSession::IdToDogIndex& GameSession::GetDogs() const {
    return dogs_;
}

const GameSession::IdToLootIndex& GameSession::GetAllLoot() const {
    return loot_;
}

void GameSession::EraseLoot(Loot::Id loot_id) {
    loot_.erase(loot_id);
}

void GameSession::UpdateState(std::int64_t tick) {
    UpdateDogsState(tick);
    GenerateLoot(tick);
    HandleCollisions();
}

std::uint32_t GameSession::GetNextDogId() const {
    return next_dog_id_;
}

std::uint32_t GameSession::GetNextLootId() const {
    return next_loot_id_;
}

void GameSession::Restore(IdToDogIndex&& dogs, std::uint32_t next_dog_id, IdToLootIndex&& loot, std::uint32_t next_loot_id) {
    dogs_ = std::forward<IdToDogIndex>(dogs);
    next_dog_id_ = next_dog_id;
    for (auto& [_, dog] : dogs_) {
        items_gatherer_provider_.AddGatherer(dog.get());
    }

    loot_ = std::forward<IdToLootIndex>(loot);
    for (auto [_, loot] : loot_) {
        items_gatherer_provider_.PushBackLoot(loot.get());
    }
    next_loot_id_ = next_loot_id;
}


void GameSession::UpdateDogsState(std::int64_t tick) {
    double ms_convertion = 0.001; // 1ms = 0.001s
    double tick_multy = static_cast<double>(tick) * ms_convertion;

    for (auto [_, dog] : dogs_) {
        if (dog->IsStopped()) {
            continue;
        }

        auto cur_dog_pos = dog->GetPosition();
        auto dog_speed = dog->GetSpeed();
        geom::Point2D new_dog_pos = {cur_dog_pos.x + (dog_speed.x * tick_multy),
            cur_dog_pos.y + (dog_speed.y * tick_multy)};

        auto relevant_road = map_->GetRelevantRoads(cur_dog_pos);
        if (relevant_road.empty()) {
            throw std::logic_error("invalid dog position");
        }

        geom::Point2D relevant_point = cur_dog_pos;
        bool stopped = true;

        for (auto road : relevant_road) {
            if (road->IsDogOnRoad(new_dog_pos)) {
                relevant_point = new_dog_pos;
                stopped = false;
                break;
            } else {

                switch (dog->GetDirection()) {
                    case Direction::NORTH:
                        if (road->GetUpperEdge() < relevant_point.y) {
                            relevant_point = {relevant_point.x, road->GetUpperEdge()};
                        }
                        break;
                    case Direction::SOUTH:
                        if (road->GetBottomEdge() > relevant_point.y) {
                            relevant_point = {relevant_point.x, road->GetBottomEdge()};
                        }
                        break;
                    case Direction::EAST:
                        if (road->GetRightEdge() > relevant_point.x) {
                            relevant_point = {road->GetRightEdge(), relevant_point.y};
                        }
                        break;
                    case Direction::WEST:
                        if (road->GetLeftEdge() < relevant_point.x) {
                            relevant_point = {road->GetLeftEdge(), relevant_point.y};
                        }
                        break;
                    default:
                        throw std::logic_error("unknown dog direction");
                        break;
                }
            }
        }

        dog->SetPosition(relevant_point);
        if (stopped) {
            dog->Stop();
        }
    }
}

void GameSession::HandleCollisions() {
    auto gather_events = collision_detector::FindGatherEvents(items_gatherer_provider_);

    std::vector<size_t> items_to_erase;
    for (const auto& event : gather_events) {
        game_obj::Bag<Loot>* gatherer_bag = items_gatherer_provider_.GetDog(event.gatherer_id)->GetBag();
        if (std::holds_alternative<const Office*>(items_gatherer_provider_.GetRawLootVal(event.item_id))) {
            if (!gatherer_bag->Empty()) {
                for (size_t i = 0; i < gatherer_bag->GetSize(); ++i) {
                    auto loot = gatherer_bag->TakeTopLoot();
                    items_gatherer_provider_.GetDog(event.gatherer_id)->AddScore(map_->GetLootScore(loot.type));
                }
            }
        } else if (std::holds_alternative<const Loot*>(items_gatherer_provider_.GetRawLootVal(event.item_id))) {
            if (std::find(items_to_erase.begin(), items_to_erase.end(), event.item_id) == items_to_erase.end()) {
                const Loot* taking_loot = std::get<const Loot*>(items_gatherer_provider_.GetRawLootVal(event.item_id));
                if (gatherer_bag->PickUpLoot(*taking_loot)) {
                    items_to_erase.push_back(event.item_id);
                }
            }
        }
    }
    // убираем из provider и session весь лишний лут
    for (size_t id : items_to_erase) {
        loot_.erase(std::get<const Loot*>(items_gatherer_provider_.GetRawLootVal(id))->id);
        items_gatherer_provider_.EraseLoot(id);
    }
}

void GameSession::GenerateLoot(std::int64_t tick) {
    loot_gen::LootGenerator::TimeInterval time_interval(tick);
    unsigned loot_counter = loot_generator_.Generate(time_interval, static_cast<unsigned>(loot_.size()), static_cast<unsigned>(dogs_.size()));
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, static_cast<unsigned>(map_->GetLootTypes().size() - 1));
    for (; loot_counter != 0; --loot_counter) {
        auto loot_ptr = std::make_shared<Loot>(Loot::Id{next_loot_id_++}, static_cast<uint8_t>(dist(rng)), map_->GetRandomPoint());
        loot_.insert({loot_ptr->id, loot_ptr});
        items_gatherer_provider_.PushBackLoot(loot_ptr.get());
    }
}

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
        sessions_[maps_.back().GetId()]; // preparing a hash table for game sessions
    }
}

const Game::Maps& Game::GetMaps() const noexcept {
    return maps_;
}

const Map* Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

GameSession& Game::StartGameSession(const Map* map) {
    using namespace std::chrono_literals;
    if (sessions_[map->GetId()].empty()) {
        sessions_[map->GetId()].push_back(std::make_shared<GameSession>(map, random_dog_spawn_, loot_config_));
    }
    return *sessions_[map->GetId()].back();
}

const GameSession* Game::GetGameSession(Map::Id map_id) const {
    if (!sessions_.contains(map_id) || sessions_.at(map_id).empty()) {
        return nullptr;
    }
    return sessions_.at(map_id).back().get();
}

GameSession* Game::GetGameSession(Map::Id map_id) {
    return const_cast<GameSession*>(static_cast<const Game&>(*this).GetGameSession(map_id)); // по Майерсу
}

const Game::SessionsByMaps& Game::GetAllSessions() const {
    return sessions_;
}


void Game::RestoreSessions(SessionsByMaps&& restoring_sessions) {
    sessions_ = std::move(restoring_sessions);
}

void Game::TurnOnRandomSpawn() {
    random_dog_spawn_ = true;
}

void Game::TurnOffRandomSpawn() {
    random_dog_spawn_ = false;
}

void Game::SetRetirementTime(double retirement_time) {
    dog_retirement_time_ = retirement_time;
}

double Game::GetRetirementTime() const noexcept {
    return dog_retirement_time_;
}

void Game::SetDogSpeed(double default_speed) {
    default_dog_speed_ = default_speed;
}

double Game::GetDefaultGogSpeed() const noexcept {
    return default_dog_speed_;
}

void Game::SetLootConfig(double period, double probability) {
    loot_config_.period = period;
    loot_config_.probability = probability;
}

const LootConfig& Game::GetLootConfig() const {
    return loot_config_;
}

bool Game::IsDogSpawnRandom() const {
    return random_dog_spawn_;
}


void Game::UpdateState(std::int64_t tick) {
    for (auto& [_, map_sessions] : sessions_) {
        for (auto session : map_sessions) {
            session->UpdateState(tick);
        }
    }
}
}  // namespace model
