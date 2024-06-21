#include "use_cases_impl.h"

#include "../domain/retired_player.h"

namespace app {
using namespace domain;


domain::PlayerId UseCasesImpl::SaveRetiredPlayer(const std::string& name, std::uint16_t score, std::uint16_t play_time_in_ms) {
    CheckTransaction();
    auto player_id = PlayerId::New();
    transaction_->RetiredPlayers().Save({player_id, name, score, play_time_in_ms});
    return player_id;
}

std::vector<domain::RetiredPlayer> UseCasesImpl::GetLeaders(size_t start, size_t max_players) {
    CheckTransaction();
    return transaction_->RetiredPlayers().GetLeaders(start, max_players);
}

void UseCasesImpl::StartTransaction() {
    transaction_.reset();
    transaction_ = db_.MakeTransaction();
}

void UseCasesImpl::Commit() {
    CheckTransaction();
    try {
        transaction_->Commit();
    } catch (const std::exception& e) {
        transaction_->Abort();
        throw e;
    }
}

}  // namespace app
