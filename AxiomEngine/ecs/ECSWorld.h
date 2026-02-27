#pragma once

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace axiom::ecs {

using Entity = std::uint32_t;
constexpr Entity InvalidEntity = 0;
constexpr std::size_t MaxComponents = 128;
using Signature = std::bitset<MaxComponents>;

class ECSWorld {
  public:
    ECSWorld() = default;

    Entity CreateEntity();
    void DestroyEntity(Entity entity);

    template <typename T> void RegisterComponent();
    template <typename T> std::size_t ComponentType();

    template <typename T, typename... Args> T& EmplaceComponent(Entity entity, Args&&... args);
    template <typename T> void RemoveComponent(Entity entity);
    template <typename T> T* GetComponent(Entity entity);
    template <typename T> const T* GetComponent(Entity entity) const;

    [[nodiscard]] Signature GetSignature(Entity entity) const;

    template <typename... Ts, typename Fn> void ForEach(Fn&& fn);

  private:
    struct IColumn {
        virtual ~IColumn() = default;
        virtual std::unique_ptr<IColumn> CreateEmpty() const = 0;
        virtual void MoveAppendFrom(IColumn& source, std::size_t row) = 0;
        virtual void AppendDefault() = 0;
        virtual void AppendCopy(const void* rawValue) = 0;
        virtual void SwapErase(std::size_t row) = 0;
    };

    template <typename T> struct Column final : IColumn {
        std::vector<T> data{};

        std::unique_ptr<IColumn> CreateEmpty() const override { return std::make_unique<Column<T>>(); }
        void MoveAppendFrom(IColumn& source, std::size_t row) override;
        void AppendDefault() override;
        void AppendCopy(const void* rawValue) override;
        void SwapErase(std::size_t row) override;
    };

    struct ComponentMeta {
        std::size_t id{};
        std::function<std::unique_ptr<IColumn>()> createColumn{};
        std::function<void(IColumn&)> appendDefault{};
        std::function<void(IColumn&, const void*)> appendCopy{};
    };

    struct Archetype {
        Signature signature{};
        std::vector<Entity> entities{};
        std::unordered_map<std::size_t, std::unique_ptr<IColumn>> columns{};
    };

    struct EntityLocation {
        std::size_t archetypeIndex{};
        std::size_t row{};
        Signature signature{};
    };

    std::unordered_map<std::type_index, ComponentMeta> componentByType_{};
    std::unordered_map<std::size_t, ComponentMeta*> componentById_{};

    std::vector<Archetype> archetypes_{};
    std::unordered_map<Entity, EntityLocation> locations_{};

    std::vector<Entity> freeList_{};
    Entity nextEntity_{1};
    std::size_t nextComponentId_{0};

    Archetype& MutableArchetype(std::size_t index);
    const Archetype& ReadArchetype(std::size_t index) const;
    std::size_t FindOrCreateArchetype(const Signature& signature);

    void MoveEntityToSignature(Entity entity, const Signature& target, std::optional<std::pair<std::size_t, const void*>> inserted);
    void EraseRow(Archetype& archetype, std::size_t row);
};

} // namespace axiom::ecs

#include "AxiomEngine/ecs/ECSWorld.inl"
