#include "postgres.h"

#include <pqxx/pqxx>
#include <pqxx/zview.hxx>
#include <pqxx/params.hxx>

#include <string>
#include <tuple>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void ConnectionPool::ReturnConnection(ConnectionPtr&& conn) {
    // Возвращаем соединение обратно в пул
    {
        std::lock_guard lock{mutex_};
        if (used_connections_ == 0) {
            throw std::runtime_error("Imposible to return connection which wasn't used");
        }
        pool_[--used_connections_] = std::move(conn);
    }
    // Уведомляем один из ожидающих потоков об изменении состояния пула
    cond_var_.notify_one();
}


void RetiredPlayersRepositoryImpl::Save(const domain::RetiredPlayer& player) {
    work_.exec_params(R"(
INSERT INTO retired_players (id, name, score, play_time_ms) VALUES ($1, $2, $3, $4);
)"_zv, player.GetId().ToString(), player.GetName(), player.GetScore(), player.GetPlayTimeInMs());
}

std::vector<domain::RetiredPlayer> RetiredPlayersRepositoryImpl::GetLeaders(size_t start, size_t max_players) {
auto query_text = R"(
SELECT id, name, score, play_time_ms FROM retired_players ORDER BY score DESC, play_time_ms, name LIMIT $1 OFFSET $2
)"_zv;

    std::vector<domain::RetiredPlayer> leaders;
    for (const auto& [id, name, score, play_time_ms] :
         work_.query<std::string, std::string, int, int>(query_text, pqxx::params{max_players, start})) {
        leaders.push_back({domain::PlayerId::FromString(id), name,
            static_cast<std::uint16_t>(score), static_cast<std::uint16_t>(play_time_ms)});
    }
    return leaders;
}

void Database::Init() {
    auto connection_wrapper = connection_pool_.GetConnection();
    pqxx::work work{*connection_wrapper};
    work.exec(R"(
CREATE TABLE IF NOT EXISTS retired_players (
id UUID PRIMARY KEY,
name varchar(100) NOT NULL,
score INTEGER DEFAULT(0) NOT NULL,
play_time_ms INTEGER DEFAULT(0) NOT NULL
);
)"_zv);

    work.exec(R"(
CREATE INDEX IF NOT EXISTS score_time_name_idx ON retired_players 
(score DESC, play_time_ms, name);
)"_zv);

    work.commit();
}

}  // namespace postgres
