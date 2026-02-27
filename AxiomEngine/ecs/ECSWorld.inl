#pragma once

#include <stdexcept>
#include <tuple>

namespace axiom::ecs {

template <typename T> void ECSWorld::RegisterComponent() {
    const auto key = std::type_index(typeid(T));
    if (meta_.contains(key)) {
        return;
    }

    const std::size_t id = nextComponentId_++;
    meta_[key] = ComponentMeta{
        .id = id,
        .makeStorage = [] { return std::make_unique<Storage<T>>(); },
        .appendFromRaw = [](IStorage& storage, const void* raw) {
            static_cast<Storage<T>&>(storage).values.push_back(*static_cast<const T*>(raw));
        },
    };
}

template <typename T> std::size_t ECSWorld::ComponentType() {
    const auto it = meta_.find(std::type_index(typeid(T)));
    if (it == meta_.end()) {
        throw std::runtime_error("Component must be registered before usage");
    }
    return it->second.id;
}

template <typename T> void ECSWorld::AddComponent(Entity entity, const T& component) {
    Signature dst = locations_.at(entity).signature;
    dst.set(ComponentType<T>());
    MoveEntity(entity, dst, std::pair{ComponentType<T>(), static_cast<const void*>(&component)});
}

template <typename T> T* ECSWorld::GetComponent(Entity entity) {
    const auto loc = locations_.at(entity);
    const auto componentId = ComponentType<T>();
    auto& archetype = archetypes_[loc.archetype];
    if (!archetype.signature.test(componentId)) {
        return nullptr;
    }
    auto& storage = static_cast<Storage<T>&>(*archetype.columns.at(componentId));
    return &storage.values[loc.row];
}

template <typename... Ts, typename Fn> void ECSWorld::ForEach(Fn&& fn) {
    Signature required{};
    (required.set(ComponentType<Ts>()), ...);

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

} // namespace axiom::ecs
