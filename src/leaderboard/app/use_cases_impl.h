#pragma once
#include "../postgres/postgres.h"
#include "use_cases.h"

#include <string>
#include <vector>

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(postgres::Database& db)
        : db_(db) {
    }

    domain::PlayerId SaveRetiredPlayer(const std::string& name, std::uint16_t score, std::uint16_t play_time_in_ms) override;
    std::vector<domain::RetiredPlayer> GetLeaders(size_t start, size_t max_players) override;

    void StartTransaction() override;
    void Commit() override;

private:
    postgres::Database& db_;
    std::unique_ptr<UnitOfWork> transaction_{nullptr};

    void NullTransactionError() {
        throw std::logic_error("Transaction is closed. Before commit changes open a transaction");
    }

    void CheckTransaction() {
        if (transaction_ == nullptr) {
            NullTransactionError();
        }
    }
};

}  // namespace app
