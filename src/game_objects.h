#pragma once

#include <vector>
#include <utility>

namespace game_obj {

template <typename Loot>
class Bag {
public:
    Bag() = default;

    explicit Bag(size_t capacity)
    : capacity_(capacity) {
        loot_.reserve(capacity_);
    }

    bool PickUpLoot(Loot loot) {
        if (Full()) {
            return false;
        }
        loot_.push_back(std::move(loot));
        return true;
    }

    Loot TakeTopLoot() {
        auto loot = std::move(loot_.back());
        loot_.pop_back();
        return loot;
    }

    const std::vector<Loot>& GetAllLoot() const {
        return loot_;
    }

    size_t GetSize() const {
        return loot_.size();
    }

    size_t GetCapacity() const {
        return capacity_;
    }

    bool Empty() const {
        return loot_.empty();
    }

    bool Full() const {
        return loot_.size() == capacity_;
    }

    auto operator<=>(const Bag&) const = default;

private:
    std::vector<Loot> loot_;
    size_t capacity_ = 0;
};

}
