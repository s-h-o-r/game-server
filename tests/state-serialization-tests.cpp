#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_contains.hpp>
#include <catch2/matchers/catch_matchers_predicate.hpp>
#include <sstream>

#include "../src/json_loader.h"
#include "../src/model.h"
#include "../src/model_serialization.h"
#include "../src/player.h"


using namespace model;
using namespace std::literals;
namespace {

using InputArchive = boost::archive::text_iarchive;
using OutputArchive = boost::archive::text_oarchive;

struct Fixture {
    std::stringstream strm;
    OutputArchive output_archive{strm};
};

}  // namespace

SCENARIO_METHOD(Fixture, "Point serialization") {
    GIVEN("A point") {
        const geom::Point2D p{10, 20};
        WHEN("point is serialized") {
            output_archive << p;

            THEN("it is equal to point after serialization") {
                InputArchive input_archive{strm};
                geom::Point2D restored_point;
                input_archive >> restored_point;
                CHECK(p == restored_point);
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "Loot Serialization") {
    GIVEN("a loot") {
        const Loot loot{Loot::Id{10}, 3, {10, 20}};
        WHEN("loot is serialized") {
            output_archive << loot;

            THEN("it can be deserialized") {
                InputArchive input_archive{strm};
                Loot loot_deserialised;
                input_archive >> loot_deserialised;
                CHECK(loot == loot_deserialised);
            }
        }

    }
}

SCENARIO_METHOD(Fixture, "Bag Serialization") {
    GIVEN("a bag") {
        const game_obj::Bag<Loot> bag = [] {
            game_obj::Bag<Loot> bag{5};
            bag.PickUpLoot(Loot{Loot::Id{10}, 3, {10, 20}});
            bag.PickUpLoot(Loot{Loot::Id{15}, 2, {1, 5}});
            return bag;
        }();

        WHEN("bag is serialized") {
            {
                serialization::BagRepr bag_repr{bag};
                output_archive << bag_repr;
            }

            THEN("it can be deserialized") {
                InputArchive input_archive{strm};
                serialization::BagRepr bag_deserialised;
                input_archive >> bag_deserialised;
                const auto bag_restored = bag_deserialised.Restore();

                CHECK(bag.GetCapacity() == bag_restored.GetCapacity());
                CHECK(bag.GetAllLoot() == bag_restored.GetAllLoot());
            }
        }
    }
}


SCENARIO_METHOD(Fixture, "Dog Serialization") {
    GIVEN("a dog") {
        auto dog = [] {
            Dog dog{Dog::Id{42}, "Pluto"s, {42.2, 12.5}, {2.3, -1.2}, 3};
            dog.AddScore(42);
            dog.GetBag()->PickUpLoot(Loot{Loot::Id{10}, 2u});
            dog.SetDirection(Direction::EAST);
            return dog;
        }();

        WHEN("dog is serialized") {
            {
                serialization::DogRepr repr{dog};
                output_archive << repr;
            }

            THEN("it can be deserialized") {
                InputArchive input_archive{strm};
                serialization::DogRepr repr;
                input_archive >> repr;
                const auto restored = repr.Restore();

                CHECK(dog.GetId() == restored->GetId());
                CHECK(dog.GetName() == restored->GetName());
                CHECK(dog.GetPosition() == restored->GetPosition());
                CHECK(dog.GetSpeed() == restored->GetSpeed());
                CHECK(dog.GetBag()->GetCapacity() == restored->GetBag()->GetCapacity());
                CHECK(*dog.GetBag() == *restored->GetBag());
            }
        }
    }
}

template <typename Range, typename Predicate = void*> // такой класс нужен для того, чтобы можно было добавлять свои предикаты для сравнения
struct IsPermutationMatcher : Catch::Matchers::MatcherGenericBase { // в моей программе используются разные структуры, а такой класс поможет их сравнить на равенство
    IsPermutationMatcher(Range range, Predicate predicate = nullptr)
        : range_{std::move(range)}
        , predicate_(predicate) {
    }

    IsPermutationMatcher(IsPermutationMatcher&&) = default;

    template <typename OtherRange>
    bool match(OtherRange other) const {
        using std::begin;
        using std::end;

        if constexpr (std::is_same_v<Predicate, void*>) {
            return std::equal(range_.begin(), range_.end(), other.begin(), other.end());
        } else {
            return std::equal(range_.begin(), range_.end(), other.begin(), other.end(), predicate_);
        }
    }

    std::string describe() const override {
        return "Is permutation of: "s + Catch::rangeToString(range_);
    }

private:
    Range range_;
    Predicate predicate_;
};

template<typename Range, typename Predicate>
IsPermutationMatcher<Range, Predicate> IsPermutation(Range&& range, Predicate predicate) {
    return IsPermutationMatcher<Range, Predicate>{std::forward<Range>(range), predicate};
}

template<typename Range>
IsPermutationMatcher<Range> IsPermutation(Range&& range) {
    return IsPermutationMatcher<Range>{std::forward<Range>(range)};
}

namespace user {
bool operator==(const Player& lhs, const Player& rhs) {
    return lhs.GetGameSession()->GetMapId() == rhs.GetGameSession()->GetMapId() &&
    lhs.GetDog()->GetId() == rhs.GetDog()->GetId();
}
} //namespace user

SCENARIO_METHOD(Fixture, "Game and App Serialization") {
    GIVEN("a game from config.json with sessions on all maps and a dog and a loot") {
        Game game = json_loader::LoadGame("../../tests/test_config.json"s);
        app::Application app(&game);
        game.SetLootConfig(1., 1.);

        game.StartGameSession(game.FindMap(Map::Id{"town"s}));
        game.StartGameSession(game.FindMap(Map::Id{"map3"s}));
        auto& game_session = game.StartGameSession(game.FindMap(Map::Id{"map1"s}));
        auto join_res_dog1 = app.JoinGame("dog1"s, "map1"s);
        Dog* dog1 = game_session.GetDog(join_res_dog1.player_id);

        game_session.UpdateState(1000);
        REQUIRE(game_session.GetAllLoot().size() == 1);

        WHEN("game session is serialized") {
            {
                serialization::GameSessionRepr gs_repr(game_session);
                output_archive << gs_repr;
            }

            THEN("it can be deserialized") {
                InputArchive input_archive{strm};
                serialization::GameSessionRepr repr;
                input_archive >> repr;
                const auto restored = repr.Restore(&game);

                CHECK(game_session.GetMap() == restored->GetMap());

                auto map_sh_ptr_predicate = [] (const auto& lhs, const auto& rhs) {
                    return *lhs.second == *rhs.second;
                };

                CHECK_THAT(game_session.GetDogs(), IsPermutation(restored->GetDogs(), map_sh_ptr_predicate));
                CHECK_THAT(game_session.GetAllLoot(), IsPermutation(restored->GetAllLoot(), map_sh_ptr_predicate));
                CHECK(game_session.GetNextDogId() == restored->GetNextDogId());
                CHECK(game_session.GetNextLootId() == restored->GetNextLootId());
            }
        }

        WHEN("game is serialized") {
            {
                serialization::GameRepr game_repr(game);
                output_archive << game_repr;
            }

            InputArchive input_archive{strm};
            serialization::GameRepr repr;
            input_archive >> repr;
            model::Game restored = json_loader::LoadGame("../../tests/test_config.json"s);
            repr.Restore(&restored);

            THEN("It can be deserialized") {
                auto sessions_predicate = [] (const auto& lhs, const auto& rhs) {
                    return std::equal(lhs.second.begin(), lhs.second.end(),
                                      rhs.second.begin(), rhs.second.end(), [](const auto& rhs, const auto& lhs) {
                        return rhs->GetMap()->GetId() == lhs->GetMap()->GetId();
                    });
                };

                CHECK_THAT(game.GetAllSessions(), IsPermutation(restored.GetAllSessions(), sessions_predicate));

                model::Game another_game;
                CHECK_THROWS_AS(repr.Restore(&another_game), std::logic_error);
            }
        }

        GIVEN("game with two players") {
            auto join_res_dog2 = app.JoinGame("dog2"s, "map3"s);

            GameSession* gs1 = game.GetGameSession(Map::Id{"map1"s});
            user::Players players;
            auto& player1 = players.Add(dog1, gs1);

            GameSession* gs2 = game.GetGameSession(Map::Id{"map3"s});
            auto dog2 = gs2->GetDog(join_res_dog1.player_id);
            auto& player2 = players.Add(dog2, gs2);

            auto players_list = players.GetAllPlayers();

            WHEN("players is serialized") {
                {
                    serialization::PlayersRepr player_repr(players);
                    output_archive << player_repr;
                }

                InputArchive input_archive{strm};
                serialization::PlayersRepr repr;
                input_archive >> repr;
                user::Players restored = repr.Restore(&game);

                THEN("all players can be deserialized") {
                    const auto& restored_players_list = restored.GetAllPlayers();

                    CHECK_THAT(players_list, IsPermutation(restored_players_list, [](const auto& lhs, const auto& rhs) {
                        return *lhs == *rhs;

                    }));
                    CHECK(*restored.FindByDogIdAndMapId(dog1->GetId(), gs1->GetMapId()) ==
                          *players.FindByDogIdAndMapId(dog1->GetId(), gs1->GetMapId()));

                    CHECK(*restored.FindByDogIdAndMapId(dog2->GetId(), Map::Id{"map3"s}) ==
                          *players.FindByDogIdAndMapId(dog2->GetId(), Map::Id{"map3"s}));
                }
            }

            GIVEN("PlayerTokens with two players' tokens") {
                user::PlayerTokens player_tokens;
                user::Token token1 = player_tokens.AddPlayer(&player1);
                user::Token token2 = player_tokens.AddPlayer(&player2);

                WHEN("PlayerTokens is serialized") {
                    {
                        serialization::PlayerTokenRepr player_tokens_repr(player_tokens);
                        output_archive << player_tokens_repr;
                    }

                    THEN("it can be deserialized") {
                        InputArchive input_archive{strm};
                        serialization::PlayerTokenRepr repr;
                        input_archive >> repr;
                        user::PlayerTokens restored = repr.Restore(&players);

                        CHECK(*player_tokens.FindPlayerByToken(token1) == *restored.FindPlayerByToken(token1));
                        CHECK(*player_tokens.FindPlayerByToken(token2) == *restored.FindPlayerByToken(token2));
                        CHECK(player_tokens.FindPlayerByToken(user::Token{"rand"}) == restored.FindPlayerByToken(user::Token{"rand"}));
                    }
                }
            }

            GIVEN("an app with prepared game, 3 maps, 2 players on different maps") {
                WHEN("it is serialized") {
                    {
                        serialization::ApplicationRepr app_repr(app);
                        output_archive << app_repr;
                    }

                    THEN("it can be deserialized") {
                        InputArchive input_archive{strm};
                        serialization::ApplicationRepr repr;
                        input_archive >> repr;

                        model::Game new_game = json_loader::LoadGame("../../tests/test_config.json"s);
                        app::Application restored{&new_game};
                        repr.Restore(&restored);

                        CHECK(app.GetPlayerGameSession(*join_res_dog1.token)->GetMap()->GetId() ==
                              restored.GetPlayerGameSession(*join_res_dog1.token)->GetMap()->GetId());

                        CHECK(app.GetPlayerGameSession(*join_res_dog2.token)->GetMap()->GetId() ==
                              restored.GetPlayerGameSession(*join_res_dog2.token)->GetMap()->GetId());

                        CHECK_THROWS_AS(app.GetPlayerGameSession("strange_token"s), app::ListPlayersError);
                    }
                }
            }
        }
    }
}

