#include <cmath>
#include <iostream>
#include <filesystem>

#include <catch2/catch_test_macros.hpp>

#include "../src/json_loader.h"
#include "../src/loot_generator.h"
#include "../src/model.h"


using namespace std::literals;

SCENARIO("Loot generation") {
    using loot_gen::LootGenerator;
    using TimeInterval = LootGenerator::TimeInterval;

    GIVEN("a loot generator") {
        LootGenerator gen{1s, 1.0};

        constexpr TimeInterval TIME_INTERVAL = 1s;

        WHEN("loot count is enough for every looter") {
            THEN("no loot is generated") {
                for (unsigned looters = 0; looters < 10; ++looters) {
                    for (unsigned loot = looters; loot < looters + 10; ++loot) {
                        INFO("loot count: " << loot << ", looters: " << looters);
                        REQUIRE(gen.Generate(TIME_INTERVAL, loot, looters) == 0);
                    }
                }
            }
        }

        WHEN("number of looters exceeds loot count") {
            THEN("number of loot is proportional to loot difference") {
                for (unsigned loot = 0; loot < 10; ++loot) {
                    for (unsigned looters = loot; looters < loot + 10; ++looters) {
                        INFO("loot count: " << loot << ", looters: " << looters);
                        REQUIRE(gen.Generate(TIME_INTERVAL, loot, looters) == looters - loot);
                    }
                }
            }
        }
    }

    GIVEN("a loot generator with some probability") {
        constexpr TimeInterval BASE_INTERVAL = 1s;
        LootGenerator gen{BASE_INTERVAL, 0.5};

        WHEN("time is greater than base interval") {
            THEN("number of generated loot is increased") {
                CHECK(gen.Generate(BASE_INTERVAL * 2, 0, 4) == 3);
            }
        }

        WHEN("time is less than base interval") {
            THEN("number of generated loot is decreased") {
                const auto time_interval
                    = std::chrono::duration_cast<TimeInterval>(std::chrono::duration<double>{
                        1.0 / (std::log(1 - 0.5) / std::log(1.0 - 0.25))});
                CHECK(gen.Generate(time_interval, 0, 4) == 1);
            }
        }
    }

    GIVEN("a loot generator with custom random generator") {
        LootGenerator gen{1s, 0.5, [] {
                              return 0.5;
                          }};
        WHEN("loot is generated") {
            THEN("number of loot is proportional to random generated values") {
                const auto time_interval
                    = std::chrono::duration_cast<TimeInterval>(std::chrono::duration<double>{
                        1.0 / (std::log(1 - 0.5) / std::log(1.0 - 0.25))});
                CHECK(gen.Generate(time_interval, 0, 4) == 0);
                CHECK(gen.Generate(time_interval, 0, 4) == 1);
            }
        }
    }
}

SCENARIO("Loot generation on map") {
    using loot_gen::LootGenerator;
    using TimeInterval = LootGenerator::TimeInterval;
    using namespace model;

    GIVEN("A game") {
        Game game = json_loader::LoadGame("../../tests/test_config.json");
        REQUIRE(!game.GetMaps().empty());
        REQUIRE(game.GetLootConfig().period == 5.0);
        REQUIRE(game.GetLootConfig().probability == 0.5);

        auto maps = game.GetMaps();
        for (auto map : maps) {
            GIVEN("a map " << *map.GetId()) {
                WHEN("map has been successfully loaded") {
                    THEN("map should has info about loot") {
                        auto loot_types = map.GetLootTypes();
                        CHECK(!loot_types.empty());
                        for (auto loot_type : loot_types) {
                            INFO("loot_info shouldn't be empty");
                            CHECK(!loot_type.loot_info.empty());
                        }
                    }
                }
            }
        }

        WHEN("session has started with loot generator (probability 100%) and period 1s") {
            LootGenerator gen{1s, 1.};
            GameSession session(game.FindMap(Map::Id{"map1"s}), false, LootConfig{1., 0.5});
            REQUIRE(session.GetAllLoot().empty());

            WHEN("try to generate loot without looters") {
                session.UpdateState(1000);
                THEN("loot should be empty") {
                    CHECK(session.GetAllLoot().empty());
                }
            }

            WHEN("session has one looter") {
                session.AddDog("TestPlayer1"sv);

                session.UpdateState(1000);
                THEN("after the first tick session should have one piece of loot") {
                    CHECK(session.GetAllLoot().size() == 1);
                }

                session.UpdateState(1000);
                THEN("after the second tick session should also have one piece of loot") {
                    INFO("amount of looters: " << session.GetDogs().size() << "\namount of loot: " << session.GetAllLoot().size());
                    CHECK(session.GetAllLoot().size() == 1);
                }
            }

            WHEN("amount of looters increasing amount of loot also increasing") {
                for (unsigned looter = 1; looter < 11; ++looter) {
                    session.AddDog("TestPlayer"sv);
                    INFO("amount of looters: " << session.GetDogs().size() << "\namount of loot: " << session.GetAllLoot().size());
                    session.UpdateState(1000);
                    INFO("amount of looters: " << session.GetDogs().size() << "\namount of loot: " << session.GetAllLoot().size());
                    CHECK(session.GetAllLoot().size() == looter);
                }
            }
        }
    }
}
