#pragma once

#include "../domain/retired_player_fwd.h"

#include <memory>

namespace app {

class UnitOfWork {
public:
    virtual ~UnitOfWork() = default;
    virtual void Commit() = 0;
    virtual void Abort() = 0;
    virtual domain::RetiredPlayersRepository& RetiredPlayers() = 0;
};

class UnitOfWorkFactory {
public:

    virtual std::unique_ptr<UnitOfWork> CreateUnitOfWork() = 0;

protected:
    ~UnitOfWorkFactory() = default;
};

} // namespace app
