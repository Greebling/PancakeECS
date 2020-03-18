#pragma once

#include <vector>
#include "ComponentData.h"


constexpr IndexType MAX_ENTITIES = std::numeric_limits<IndexType>::max();
constexpr IndexType BASE_ENTITY_VECTOR_SIZE = 16;

template <typename... Ts>
class ComponentView;

class ComponentVectorBase {
public:
    ComponentVectorBase() {
        entityIndex = new std::unordered_map<EntityID, IndexType>();
    }

    virtual ~ComponentVectorBase() {
        delete entityIndex;
    }

    virtual void RemoveComponentFrom(EntityID id) = 0;

    [[nodiscard]] const std::unordered_map<EntityID, IndexType> &getEntities() const {
        return *entityIndex;
    }

    size_t Size() {
        return entityIndex->size();
    }

protected:
    /// Maps an EntityID to the index of the component it owns
    std::unordered_map<EntityID, IndexType> *entityIndex;
};

template<typename ComponentType>
class ComponentVector : public ComponentVectorBase {
public:
    ComponentVector()
            : ComponentVectorBase() {
        static_assert(std::is_base_of<ComponentData, ComponentType>::value, "Must derive from ComponentData!");

        _components = new std::vector<ComponentType>();
        _components->reserve(BASE_ENTITY_VECTOR_SIZE);
    }

    ~ComponentVector() override {
        delete _components;
    }

private:
	template<typename ...>
	friend
	class ComponentView;
    
    static unsigned int index;
	static inline std::vector<unsigned int> savedIndices = std::vector<unsigned int>();
	
	
    std::vector<ComponentType> *_components;

public:
    ComponentType &AddComponent(EntityID id) {
        // should not add component if it is already there
        if (entityIndex->count(id))
            return (*_components)[(*entityIndex)[id]];

        entityIndex->insert(std::pair(id, _components->size()));

        _components->push_back(ComponentType());
        _components->back().id = id;

        return _components->back();
    }

    void RemoveComponentFrom(EntityID id) override {
	    RemoveComponent(id);
    }

    void RemoveComponent(EntityID id) {
        if (!entityIndex->count(id))
            return;

        (*_components)[(*entityIndex)[id]] = _components->back();
        (*entityIndex)[_components->back().id] = (*entityIndex)[id];
        entityIndex->erase(id);
        _components->pop_back();
    }

    ComponentType *GetComponent(EntityID id) {
        if (!Contains(id))
            return nullptr;

        return &(*_components)[(*entityIndex)[id]];
    }

    ComponentType &operator[](size_t index) {
        return (*_components)[index];
    }

    [[nodiscard]] bool Contains(EntityID id) const {
        return entityIndex->count(id) != 0;
    }

    [[nodiscard]] IndexType IndexOf(EntityID id) const {
        assert(Contains(id));
	
        return (*entityIndex)[id];
    }
};

template<typename ComponentType>
unsigned int ComponentVector<ComponentType>::index = 0;
