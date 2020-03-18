// Created on 22.12.2019.

#pragma once

#include <unordered_map> // TODO: make hash work without including unordered_map

typedef unsigned short SaltType;
typedef unsigned int IndexType;

struct EntityID {
    EntityID()
            : _index(0), _salt(0) {
    }

    EntityID(IndexType index, SaltType salt)
            : _index(index), _salt(salt) {
    }

    [[nodiscard]] SaltType Salt() const {
        return _salt;
    }

    [[nodiscard]] IndexType Index() const {
        return _index;
    }

    [[nodiscard]] bool IsAlive() const {
        return _index != 0;
    }

    void MarkDead() {
        _index = 0;
    }

    bool operator==(const EntityID &rhs) const {
        return _salt == rhs._salt && _index == rhs._index;
    }

    bool operator!=(const EntityID &rhs) const {
        return _salt != rhs._salt || _index != rhs._index;
    }


private:
    friend std::hash<EntityID>;

    IndexType _index;
    SaltType _salt;
};


// Hashing function for EntityID
namespace std {
    template<>
    struct hash<EntityID> {
        std::size_t operator()(const EntityID &k) const {
            // we need to ne mindful of shift overflows using the below formula
            constexpr IndexType shiftAmount = static_cast<IndexType>(std::min(
                    std::numeric_limits<IndexType>::digits,
                    std::numeric_limits<std::size_t>::digits -
                    (std::numeric_limits<IndexType>::digits + 1)));
            return k._salt << shiftAmount xor k._index;
        }
    };
}