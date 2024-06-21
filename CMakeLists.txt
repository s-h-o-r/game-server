cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(GameModelLib STATIC
    src/sdk.h
    src/tagged.h
    src/model.h
    src/model.cpp
    src/boost_json.cpp
    src/json_loader.h
    src/json_loader.cpp
    src/player.h
    src/player.cpp
    src/loot_generator.h
    src/loot_generator.cpp
    src/extra_data.h
    src/app.h
    src/app.cpp
    src/collision_detector.h
    src/collision_detector.cpp
    src/geom.h
    src/game_objects.h
    src/model_serialization.h
    src/model_serialization.cpp
    src/retirement_detector.h
    src/retirement_detector.cpp
    src/leaderboard/leaderboard.h
    src/leaderboard/leaderboard.cpp
    src/leaderboard/app/use_cases.h
    src/leaderboard/app/use_cases_impl.cpp
    src/leaderboard/app/use_cases_impl.h
    src/leaderboard/app/unit_of_work_fwd.h
    src/leaderboard/app/unit_of_work.h
    src/leaderboard/domain/retired_player.h
    src/leaderboard/domain/retired_player_fwd.h
    src/leaderboard/util/tagged_uuid.cpp
    src/leaderboard/util/tagged_uuid.h
    src/leaderboard/postgres/postgres.cpp
    src/leaderboard/postgres/postgres.h
)

target_link_libraries(GameModelLib PUBLIC CONAN_PKG::boost Threads::Threads CONAN_PKG::libpq CONAN_PKG::libpqxx)
target_include_directories(GameModelLib PUBLIC CONAN_PKG::boost)

add_executable(game_server
    src/main.cpp
    src/http_server.cpp
    src/http_server.h
    src/request_handler.cpp
    src/request_handler.h
    src/logger.cpp
    src/logger.h
    src/cl_parser.h
    src/cl_parser.cpp
    src/ticker.h
    src/ticker.cpp
    src/
)

target_link_libraries(game_server GameModelLib)


if(CMAKE_BUILD_TYPE EQUAL "Debug")

    add_executable(game_server_tests
        tests/loot_generator_tests.cpp
        tests/collision-detector-tests.cpp
        tests/state-serialization-tests.cpp
    )
    target_link_libraries(game_server_tests CONAN_PKG::catch2 GameModelLib)

    include(CTest)
    include(${CONAN_BUILD_DIRS_CATCH2_DEBUG}/Catch.cmake)
    catch_discover_tests(game_server_tests)
endif()
