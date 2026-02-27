#include "AxiomEngine/ecs/ECSWorld.h"

#include <stdexcept>

namespace axiom::ecs {

Entity ECSWorld::CreateEntity() {
    if (archetypes_.empty()) {
        archetypes_.push_back({});
    }

    Entity entity = InvalidEntity;
    if (!freeList_.empty()) {
        entity = freeList_.back();
        freeList_.pop_back();
    } else {
        entity = nextEntity_++;
    }

    auto& root = archetypes_.front();
    root.entities.push_back(entity);
    locations_[entity] = EntityLocation{0, root.entities.size() - 1U, {}};
    return entity;
}

void ECSWorld::DestroyEntity(Entity entity) {
    const auto it = locations_.find(entity);
    if (it == locations_.end()) {
        return;
    }

    auto& archetype = MutableArchetype(it->second.archetypeIndex);
    EraseRow(archetype, it->second.row);
    locations_.erase(it);
    freeList_.push_back(entity);
}

Signature ECSWorld::GetSignature(Entity entity) const {
    const auto it = locations_.find(entity);
    if (it == locations_.end()) {
        return {};
    }
    return it->second.signature;
}

ECSWorld::Archetype& ECSWorld::MutableArchetype(std::size_t index) {
    return archetypes_.at(index);
}


std::size_t ECSWorld::FindOrCreateArchetype(const Signature& signature) {
    for (std::size_t i = 0; i < archetypes_.size(); ++i) {
        if (archetypes_[i].signature == signature) {
            return i;
        }
    }

    Archetype archetype{};
    archetype.signature = signature;
    for (std::size_t componentId = 0; componentId < MaxComponents; ++componentId) {
        if (!signature.test(componentId)) {
            continue;
        }

        const auto metaIt = componentById_.find(componentId);
        if (metaIt == componentById_.end()) {
            throw std::runtime_error("Archetype contains unregistered component");
        }

        archetype.columns.emplace(componentId, metaIt->second->createColumn());
    }

    archetypes_.push_back(std::move(archetype));
    return archetypes_.size() - 1U;
}

void ECSWorld::MoveEntityToSignature(Entity entity, const Signature& target, std::optional<std::pair<std::size_t, const void*>> inserted) {
    auto locIt = locations_.find(entity);
    if (locIt == locations_.end()) {
        throw std::runtime_error("Entity does not exist");
    }

    if (locIt->second.signature == target) {
        return;
    }

    const auto sourceIndex = locIt->second.archetypeIndex;
    auto sourceRow = locIt->second.row;
    auto& source = MutableArchetype(sourceIndex);

    const auto targetIndex = FindOrCreateArchetype(target);
    auto& destination = MutableArchetype(targetIndex);

    const std::size_t destinationRow = destination.entities.size();
    destination.entities.push_back(entity);

    for (std::size_t componentId = 0; componentId < MaxComponents; ++componentId) {
        if (!target.test(componentId)) {
            continue;
        }

        auto& dstColumn = *destination.columns.at(componentId);
        if (source.signature.test(componentId)) {
            auto& srcColumn = *source.columns.at(componentId);
            dstColumn.MoveAppendFrom(srcColumn, sourceRow);
            continue;
        }

        if (inserted.has_value() && inserted->first == componentId) {
            const auto metaIt = componentById_.find(componentId);
            metaIt->second->appendCopy(dstColumn, inserted->second);
        } else {
            const auto metaIt = componentById_.find(componentId);
            metaIt->second->appendDefault(dstColumn);
        }
    }

    EraseRow(source, sourceRow);
    locations_[entity] = EntityLocation{targetIndex, destinationRow, target};
}

void ECSWorld::EraseRow(Archetype& archetype, std::size_t row) {
    const Entity removed = archetype.entities[row];
    const Entity moved = archetype.entities.back();

    if (row + 1U < archetype.entities.size()) {
        archetype.entities[row] = moved;
    }
    archetype.entities.pop_back();

    for (auto& [_, column] : archetype.columns) {
        column->SwapErase(row);
    }

    if (removed != moved) {
        auto movedIt = locations_.find(moved);
        if (movedIt != locations_.end()) {
            movedIt->second.row = row;
        }
    }
}

} // namespace axiom::ecs
