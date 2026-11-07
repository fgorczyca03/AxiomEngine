#pragma once

#include <stdexcept>
#include <tuple>
#include <utility>

namespace axiom::ecs {

template <typename T> void ECSWorld::RegisterComponent() {
    const auto key = std::type_index(typeid(T));
    if (meta_.contains(key)) {
        return;
    }
    if (nextComponentId_ >= MaxComponents) {
        throw std::length_error("ECS component limit exceeded");
    }

    // This ID is the bit position used by every archetype signature in this world.
    const std::size_t id = nextComponentId_++;
    meta_[key] = ComponentMeta{
        .id = id,
        .makeStorage = [] { return std::make_unique<Storage<T>>(); },
        .appendFromRaw = [](IStorage& storage, const void* raw) {
            static_cast<Storage<T>&>(storage).values.push_back(*static_cast<const T*>(raw));
        },
    };
}

template <typename T> std::size_t ECSWorld::ComponentType() const {
    const auto it = meta_.find(std::type_index(typeid(T)));
    if (it == meta_.end()) {
        throw std::runtime_error("Component must be registered before usage");
    }
    return it->second.id;
}

template <typename T> void ECSWorld::AddComponent(Entity entity, const T& component) {
    const auto location = locations_.find(entity);
    if (location == locations_.end()) {
        throw std::out_of_range("Cannot add a component to an entity that is not alive");
    }
    const std::size_t componentId = ComponentType<T>();
    if (location->second.signature.test(componentId)) {
        // Replacing an existing component does not change the archetype or row.
        *GetComponent<T>(entity) = component;
        return;
    }
    Signature destination = location->second.signature;
    destination.set(componentId);
    MoveEntity(entity, destination, std::pair{componentId, static_cast<const void*>(&component)});
}

template <typename T> void ECSWorld::RemoveComponent(Entity entity) {
    const auto location = locations_.find(entity);
    if (location == locations_.end()) {
        return;
    }
    const std::size_t componentId = ComponentType<T>();
    if (!location->second.signature.test(componentId)) {
        return;
    }
    Signature destination = location->second.signature;
    destination.reset(componentId);
    MoveEntity(entity, destination, std::nullopt);
}

template <typename T> T* ECSWorld::GetComponent(Entity entity) {
    const auto location = locations_.find(entity);
    if (location == locations_.end()) {
        return nullptr;
    }
    const auto& loc = location->second;
    const auto componentId = ComponentType<T>();
    auto& archetype = archetypes_[loc.archetype];
    if (!archetype.signature.test(componentId)) {
        return nullptr;
    }
    auto& storage = static_cast<Storage<T>&>(*archetype.columns.at(componentId));
    return &storage.values[loc.row];
}

template <typename T> const T* ECSWorld::GetComponent(Entity entity) const {
    const auto location = locations_.find(entity);
    if (location == locations_.end()) {
        return nullptr;
    }
    const auto componentId = ComponentType<T>();
    const auto& archetype = archetypes_[location->second.archetype];
    if (!archetype.signature.test(componentId)) {
        return nullptr;
    }
    const auto& storage = static_cast<const Storage<T>&>(*archetype.columns.at(componentId));
    return &storage.values[location->second.row];
}

template <typename... Ts, typename Fn> void ECSWorld::ForEach(Fn&& fn) {
    Signature required{};
    (required.set(ComponentType<Ts>()), ...);

    // Matching signatures let each callback walk contiguous columns without per-entity lookups.
    for (auto& archetype : archetypes_) {
        if ((archetype.signature & required) != required) {
            continue;
        }
        auto cols = std::tuple<Storage<Ts>*...>{static_cast<Storage<Ts>*>(archetype.columns.at(ComponentType<Ts>()).get())...};
        for (std::size_t i = 0; i < archetype.entities.size(); ++i) {
            std::apply([&](auto*... col) { fn(archetype.entities[i], col->values[i]...); }, cols);
        }
    }
}

template <typename... Ts, typename Fn> void ECSWorld::ForEach(Fn&& fn) const {
    Signature required{};
    (required.set(ComponentType<Ts>()), ...);

    for (const auto& archetype : archetypes_) {
        if ((archetype.signature & required) != required) {
            continue;
        }
        auto columns = std::tuple<const Storage<Ts>*...>{
            static_cast<const Storage<Ts>*>(archetype.columns.at(ComponentType<Ts>()).get())...};
        for (std::size_t row = 0; row < archetype.entities.size(); ++row) {
            std::apply([&](const auto*... column) { fn(archetype.entities[row], std::as_const(column->values[row])...); },
                       columns);
        }
    }
}

} // namespace axiom::ecs
