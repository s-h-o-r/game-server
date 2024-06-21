#pragma once

#include <boost/json.hpp>

#include <string>

namespace extra_data {

namespace json = boost::json;

struct LootType {
    json::object loot_info;
};

} // namespace extra_data
