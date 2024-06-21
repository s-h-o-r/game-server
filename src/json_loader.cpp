#include "json_loader.h"
#include <fstream>
#include <iostream>
#include <string>

#define BOOST_BEAST_USE_STD_STRING_VIEW

namespace json_loader {

using namespace std::literals;
namespace sys = boost::system;

model::Road PrepareRoad(json::object& road_info) {
    geom::Point start{json::value_to<geom::Coord>(road_info.at("x0"sv)),
        json::value_to<geom::Coord>(road_info.at("y0"sv))};
    if (road_info.if_contains("x1"sv)) {
        return model::Road(model::Road::HORIZONTAL, start,
                           json::value_to<geom::Coord>(road_info.at("x1"sv)));
    } else {
        return model::Road(model::Road::VERTICAL, start,
                           json::value_to<geom::Coord>(road_info.at("y1"sv)));
    }
}

model::Building PrepareBuilding(json::object& building_info) {
    geom::Point point{json::value_to<geom::Coord>(building_info.at("x"sv)),
        json::value_to<geom::Coord>(building_info.at("y"sv))};
    geom::Size size{json::value_to<geom::Dimension>(building_info.at("w"sv)),
        json::value_to<geom::Dimension>(building_info.at("h"sv))};

    return model::Building{{point, size}};
}

model::Office PrepareOffice(json::object& office_info) {
    geom::Point point{json::value_to<geom::Coord>(office_info.at("x"sv)),
        json::value_to<geom::Coord>(office_info.at("y"sv))};
    geom::Offset offset{json::value_to<geom::Coord>(office_info.at("offsetX"sv)),
        json::value_to<geom::Coord>(office_info.at("offsetY"sv))};

    std::string id_str(json::value_to<std::string>(office_info.at("id"sv)));

    return {model::Office::Id{id_str}, point, offset};
}

model::Map PrepareMap(json::object& map_info, double default_dog_speed, size_t default_bag_capacity) {
    model::Map map(model::Map::Id(json::value_to<std::string>(map_info.at("id"sv))),
                   json::value_to<std::string>(map_info.at("name"sv)));

    if (map_info.count("dogSpeed"sv)) {
        map.SetDogSpeed(json::value_to<double>(map_info.at("dogSpeed"sv)));
    } else {
        map.SetDogSpeed(default_dog_speed);
    }

    if (map_info.count("bagCapacity"sv)) {
        map.SetBagCapacity(json::value_to<size_t>(map_info.at("bagCapacity"sv)));
    } else {
        map.SetBagCapacity(default_bag_capacity);
    }

    json::array roads = map_info.at("roads"sv).as_array();
    for (auto it = roads.begin(); it != roads.end(); ++it) {
        map.AddRoad(PrepareRoad(it->as_object()));
    }

    json::array buildings = map_info.at("buildings"sv).as_array();
    for (auto it = buildings.begin(); it != buildings.end(); ++it) {
        map.AddBuilding(PrepareBuilding(it->as_object()));
    }

    json::array offices = map_info.at("offices"sv).as_array();
    for (auto it = offices.begin(); it != offices.end(); ++it) {
        map.AddOffice(PrepareOffice(it->as_object()));
    }

    json::array loot_types = map_info.at("lootTypes"sv).as_array();
    for (auto it = loot_types.begin(); it != loot_types.end(); ++it) {
        unsigned type_score = static_cast<unsigned>(it->at("value"sv).as_int64());
        map.AddLootType({std::move(it->as_object())}, type_score);
    }

    return map;
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    model::Game game;

    std::ifstream json(json_path);
    if (!json.is_open()) {
        throw std::runtime_error("Cannot open the file.");
    }

    std::string json_data((std::istreambuf_iterator<char>(json)),
                          std::istreambuf_iterator<char>());

    sys::error_code ec;

    json::value game_info{json::parse(json_data, ec)};

    double default_dog_speed = 1.0;
    size_t default_bag_capacity = 3;

    if (game_info.as_object().count("defaultDogSpeed"sv)) {
        default_dog_speed = json::value_to<double>(game_info.at("defaultDogSpeed"sv));
    }

    if (game_info.as_object().count("defaultBagCapacity"sv)) {
        default_bag_capacity = json::value_to<size_t>(game_info.at("defaultBagCapacity"sv));
    }

    json::array& maps = game_info.as_object().at("maps"sv).as_array();
    for (auto it = maps.begin(); it != maps.end(); ++it) {
        size_t bag_capacity = default_bag_capacity;
        if (it->as_object().count("bagCapacity"sv)) {
            bag_capacity = json::value_to<size_t>(it->as_object().at("bagCapacity"sv));
        }

        game.AddMap(PrepareMap(it->as_object(), default_dog_speed, bag_capacity));
    }

    auto loot_config = game_info.at("lootGeneratorConfig"sv).as_object();
    game.SetLootConfig(loot_config.at("period"sv).as_double(), loot_config.at("probability"sv).as_double());

    if (game_info.as_object().count("dogRetirementTime"sv)) {
        game.SetRetirementTime(game_info.at("dogRetirementTime"sv).as_double());
    }
    return game;
}

}  // namespace json_loader
