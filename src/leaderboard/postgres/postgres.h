#pragma once

#include <condition_variable>
#include <mutex>

#include <pqxx/connection>
#include <pqxx/transaction>

#include "../app/unit_of_work.h"
#include "../domain/retired_player.h"

namespace postgres {

class ConnectionPool {
    using PoolType = ConnectionPool;
    using ConnectionPtr = std::shared_ptr<pqxx::connection>;

public:
    class ConnectionWrapper {
    public:
        ConnectionWrapper(std::shared_ptr<pqxx::connection>&& conn, PoolType& pool) noexcept
            : conn_{std::move(conn)}
            , pool_{&pool} {
        }

        ConnectionWrapper(const ConnectionWrapper&) = delete;
        ConnectionWrapper& operator=(const ConnectionWrapper&) = delete;

        ConnectionWrapper(ConnectionWrapper&&) = default;
        ConnectionWrapper& operator=(ConnectionWrapper&&) = default;

        pqxx::connection& operator*() const& noexcept {
            return *conn_;
        }
        pqxx::connection& operator*() const&& = delete;

        pqxx::connection* operator->() const& noexcept {
            return conn_.get();
        }

        ~ConnectionWrapper() {
            if (conn_) {
                pool_->ReturnConnection(std::move(conn_));
            }
        }

    private:
        std::shared_ptr<pqxx::connection> conn_;
        PoolType* pool_;
    };

    // ConnectionFactory is a functional object returning std::shared_ptr<pqxx::connection>
    template <typename ConnectionFactory>
    ConnectionPool(size_t capacity, ConnectionFactory&& connection_factory) {
        pool_.reserve(capacity);
        for (size_t i = 0; i < capacity; ++i) {
            pool_.emplace_back(connection_factory());
        }
    }

    ConnectionWrapper GetConnection() {
        std::unique_lock lock{mutex_};
        cond_var_.wait(lock, [this] {
            return used_connections_ < pool_.size();
        });

        return {std::move(pool_[used_connections_++]), *this};
    }

private:
    void ReturnConnection(ConnectionPtr&& conn);

    std::mutex mutex_;
    std::condition_variable cond_var_;
    std::vector<ConnectionPtr> pool_;
    size_t used_connections_ = 0;
};


class RetiredPlayersRepositoryImpl : public domain::RetiredPlayersRepository {
public:
    explicit RetiredPlayersRepositoryImpl(pqxx::work& work)
        : work_(work) {
    }

    void Save(const domain::RetiredPlayer& player) override;
    std::vector<domain::RetiredPlayer> GetLeaders(size_t start, size_t max_players) override;

private:
    pqxx::work& work_;
};

class UnitOfWorkImpl : public app::UnitOfWork {
public:
    UnitOfWorkImpl(ConnectionPool::ConnectionWrapper connection)
    : work_(*connection)
    , retired_players_(work_) {
    }

    void Commit() override {
        work_.commit();
    }

    void Abort() override {
        work_.abort();
    }

    domain::RetiredPlayersRepository& RetiredPlayers() override {
        return retired_players_;
    }

private:
    pqxx::work work_;
    RetiredPlayersRepositoryImpl retired_players_;
};

class UnitOfWorkFactoryImpl : public app::UnitOfWorkFactory {
public:
    explicit UnitOfWorkFactoryImpl(ConnectionPool& connection_pool)
    : connection_pool_(connection_pool) {
    }

    std::unique_ptr<app::UnitOfWork> CreateUnitOfWork() override {
        return std::make_unique<UnitOfWorkImpl>(connection_pool_.GetConnection());
    }

private:
    ConnectionPool& connection_pool_;
};

class Database {
public:
    template <typename ConnectionFactory>
    explicit Database(size_t connection_pool_capacity, ConnectionFactory&& connection_factory)
    : connection_pool_(connection_pool_capacity, std::move(connection_factory))
    , unit_of_work_factory_(connection_pool_) {
        Init();
    }

    std::unique_ptr<app::UnitOfWork> MakeTransaction() {
        return unit_of_work_factory_.CreateUnitOfWork();
    }

private:
    ConnectionPool connection_pool_;
    UnitOfWorkFactoryImpl unit_of_work_factory_;

    void Init();
};

}  // namespace postgres
