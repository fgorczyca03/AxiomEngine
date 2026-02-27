#include "AxiomEngine/ecs/ECSWorld.h"

namespace axiom::ecs {

Entity ECSWorld::CreateEntity() {
    if (archetypes_.empty()) {
        archetypes_.push_back({});
    }

    const Entity entity = free_.empty() ? nextEntity_++ : free_.back();
    if (!free_.empty()) {
        free_.pop_back();
    }

    auto& root = archetypes_.front();
    root.entities.push_back(entity);
    locations_[entity] = EntityLocation{0, root.entities.size() - 1, {}};
    return entity;
}

void ECSWorld::DestroyEntity(Entity entity) {
    const auto loc = locations_.at(entity);
    auto& archetype = archetypes_[loc.archetype];

    const Entity moved = archetype.entities.back();
    if (loc.row + 1U < archetype.entities.size()) {
        archetype.entities[loc.row] = moved;
    }
    archetype.entities.pop_back();

    for (auto& [_, storage] : archetype.columns) {
        storage->SwapAndPop(loc.row);
    }

    if (moved != entity) {
        locations_[moved].row = loc.row;
    }

    locations_.erase(entity);
    free_.push_back(entity);
}

std::size_t ECSWorld::FindOrCreateArchetype(const Signature& signature) {
    for (std::size_t i = 0; i < archetypes_.size(); ++i) {
        if (archetypes_[i].signature == signature) {
            return i;
        }
    }

    Archetype archetype{};
    archetype.signature = signature;
    for (const auto& [_, info] : meta_) {
        if (signature.test(info.id)) {
            archetype.columns[info.id] = info.makeStorage();
        }
    }
    archetypes_.push_back(std::move(archetype));
    return archetypes_.size() - 1;
}

void ECSWorld::MoveEntity(Entity entity, const Signature& dstSignature, std::optional<std::pair<std::size_t, const void*>> inserted) {
    const auto srcLoc = locations_.at(entity);
    auto& src = archetypes_[srcLoc.archetype];

    const auto dstIndex = FindOrCreateArchetype(dstSignature);
    auto& dst = archetypes_[dstIndex];

    const std::size_t dstRow = dst.entities.size();
    dst.entities.push_back(entity);

    for (auto& [componentId, srcStorage] : src.columns) {
        if (!dstSignature.test(componentId)) {
            continue;
        }
        srcStorage->MoveRowTo(srcLoc.row, *dst.columns.at(componentId));
    }

    if (inserted.has_value()) {
        const auto [componentId, raw] = *inserted;
        for (const auto& [_, m] : meta_) {
            if (m.id == componentId) {
                m.appendFromRaw(*dst.columns.at(componentId), raw);
                break;
            }
        }
    }

    const Entity moved = src.entities.back();
    if (srcLoc.row + 1U < src.entities.size()) {
        src.entities[srcLoc.row] = moved;
    }
    src.entities.pop_back();

    for (auto& [_, storage] : src.columns) {
        storage->SwapAndPop(srcLoc.row);
    }

    if (moved != entity) {
        locations_[moved].row = srcLoc.row;
    }

    locations_[entity] = EntityLocation{dstIndex, dstRow, dstSignature};
}

} // namespace axiom::ecs
