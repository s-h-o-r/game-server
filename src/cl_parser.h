#pragma once

#include <boost/program_options.hpp>
#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace cl_parser {

struct Args {
    std::int64_t tick_period = 0;
    std::int64_t save_state_period = 0;
    std::string config_file_path;
    std::string static_root;
    std::string state_file;
    bool random_spawn_point = false;
};

[[nodiscard]] std::optional<Args> ParseComandLine(int argc, const char* const argv[]);

} // namespace cl_parser
