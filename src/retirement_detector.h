#pragma once

#include "app.h"
#include "model.h"

#include <chrono>
#include <string>
#include <unordered_map>
#include <cstdint>

namespace retirement {

struct RetirementStatistic {
    std::uint16_t time_in_game = 0;
    std::uint16_t no_action_time = 0;
};

class RetirementListener : public app::ApplicationListener {
public:

    RetirementListener(double retirement_time_in_sec, app::Application* app)
    : retirement_time_(static_cast<std::uint64_t>(retirement_time_in_sec * 1000)) // 1000 - ms multiplier
    , app_(app) {
    }

    void OnTick(std::chrono::milliseconds delta) override;
    void OnJoin(std::string token, model::Dog* dog) override;

private:
    std::uint64_t retirement_time_;
    app::Application* app_;

    std::unordered_map<model::Dog*, RetirementStatistic> dog_retirement_;
    std::unordered_map<model::Dog*, std::string> dog_to_token_;
};
}
