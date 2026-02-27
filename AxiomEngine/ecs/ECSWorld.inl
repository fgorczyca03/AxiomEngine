#pragma once

#include <stdexcept>
#include <tuple>
#include <utility>

namespace axiom::ecs {

template <typename T> void ECSWorld::Column<T>::MoveAppendFrom(IColumn& source, std::size_t row) {
    auto& src = static_cast<Column<T>&>(source);
    data.push_back(std::move(src.data[row]));
}

template <typename T> void ECSWorld::Column<T>::AppendDefault() { data.emplace_back(); }

template <typename T> void ECSWorld::Column<T>::AppendCopy(const void* rawValue) {
    data.push_back(*static_cast<const T*>(rawValue));
}

template <typename T> void ECSWorld::Column<T>::SwapErase(std::size_t row) {
    if (row + 1U < data.size()) {
        data[row] = std::move(data.back());
    }
    data.pop_back();
}

template <typename T> void ECSWorld::RegisterComponent() {
    const auto key = std::type_index(typeid(T));
    if (componentByType_.contains(key)) {
        return;
    }

    if (nextComponentId_ >= MaxComponents) {
        throw std::runtime_error("Exceeded MaxComponents limit");
    }

    const std::size_t id = nextComponentId_++;
    auto [it, _] = componentByType_.emplace(
        key,
        ComponentMeta{
            .id = id,
            .createColumn = []() { return std::make_unique<Column<T>>(); },
            .appendDefault = [](IColumn& column) { static_cast<Column<T>&>(column).AppendDefault(); },
            .appendCopy = [](IColumn& column, const void* value) { static_cast<Column<T>&>(column).AppendCopy(value); },
        });

    componentById_.emplace(id, &it->second);
}

template <typename T> std::size_t ECSWorld::ComponentType() {
    const auto it = componentByType_.find(std::type_index(typeid(T)));
    if (it == componentByType_.end()) {
        throw std::runtime_error("Component must be registered before use");
    }
    return it->second.id;
}

template <typename T, typename... Args> T& ECSWorld::EmplaceComponent(Entity entity, Args&&... args) {
    const auto componentId = ComponentType<T>();
    auto* existing = GetComponent<T>(entity);
    if (existing != nullptr) {
        *existing = T{std::forward<Args>(args)...};
        return *existing;
    }

    const T value{std::forward<Args>(args)...};
    Signature target = GetSignature(entity);
    target.set(componentId);
    MoveEntityToSignature(entity, target, std::pair{componentId, static_cast<const void*>(&value)});
    return *GetComponent<T>(entity);
}

template <typename T> void ECSWorld::RemoveComponent(Entity entity) {
    Signature target = GetSignature(entity);
    target.reset(ComponentType<T>());
    MoveEntityToSignature(entity, target, std::nullopt);
}

template <typename T> T* ECSWorld::GetComponent(Entity entity) {
    const auto locIt = locations_.find(entity);
    if (locIt == locations_.end()) {
        return nullptr;
    }

    const auto componentId = ComponentType<T>();
    auto& archetype = MutableArchetype(locIt->second.archetypeIndex);
    if (!archetype.signature.test(componentId)) {
        return nullptr;
    }

    auto& column = static_cast<Column<T>&>(*archetype.columns.at(componentId));
    return &column.data[locIt->second.row];
}

template <typename T> const T* ECSWorld::GetComponent(Entity entity) const {
    return const_cast<ECSWorld*>(this)->GetComponent<T>(entity);
}

template <typename... Ts, typename Fn> void ECSWorld::ForEach(Fn&& fn) {
    Signature required{};
    const auto componentIds = std::array<std::size_t, sizeof...(Ts)>{ComponentType<Ts>()...};
    for (const auto id : componentIds) {
        required.set(id);
    }

    for (auto& archetype : archetypes_) {
        if ((archetype.signature & required) != required) {
            continue;
        }

        auto columns = std::tuple<Column<Ts>*...>{
            static_cast<Column<Ts>*>(archetype.columns.at(ComponentType<Ts>()).get())...,
        };

        for (std::size_t row = 0; row < archetype.entities.size(); ++row) {
            std::apply(
                [&](auto*... cols) {
                    fn(archetype.entities[row], cols->data[row]...);
                },
                columns);
        }
    }
}

} // namespace axiom::ecs
