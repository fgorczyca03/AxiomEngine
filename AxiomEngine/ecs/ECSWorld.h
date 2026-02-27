#pragma once

#include <bitset>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace axiom::ecs {

using Entity = std::uint32_t;
constexpr std::size_t MaxComponents = 64;
using Signature = std::bitset<MaxComponents>;

class ECSWorld {
  public:
    ECSWorld() = default;

    Entity CreateEntity();
    void DestroyEntity(Entity entity);

    template <typename T> void RegisterComponent();
    template <typename T> std::size_t ComponentType();
    template <typename T> void AddComponent(Entity entity, const T& component);
    template <typename T> T* GetComponent(Entity entity);

    template <typename... Ts, typename Fn> void ForEach(Fn&& fn);

  private:
    struct IStorage {
        virtual ~IStorage() = default;
        virtual std::unique_ptr<IStorage> CloneEmpty() const = 0;
        virtual void MoveRowTo(std::size_t src, IStorage& dst) = 0;
        virtual void SwapAndPop(std::size_t row) = 0;
    };

    template <typename T> struct Storage final : IStorage {
        std::vector<T> values{};
        std::unique_ptr<IStorage> CloneEmpty() const override { return std::make_unique<Storage<T>>(); }
        void MoveRowTo(std::size_t src, IStorage& dst) override {
            static_cast<Storage<T>&>(dst).values.push_back(std::move(values[src]));
        }
        void SwapAndPop(std::size_t row) override {
            if (row + 1U < values.size()) {
                values[row] = std::move(values.back());
            }
            values.pop_back();
        }
    };

    struct ComponentMeta {
        std::size_t id{};
        std::function<std::unique_ptr<IStorage>()> makeStorage{};
        std::function<void(IStorage&, const void*)> appendFromRaw{};
    };

    struct Archetype {
        Signature signature{};
        std::vector<Entity> entities{};
        std::unordered_map<std::size_t, std::unique_ptr<IStorage>> columns{};
    };

    struct EntityLocation {
        std::size_t archetype{};
        std::size_t row{};
        Signature signature{};
    };

    std::unordered_map<std::type_index, ComponentMeta> meta_{};
    std::vector<Archetype> archetypes_{};
    std::unordered_map<Entity, EntityLocation> locations_{};
    std::vector<Entity> free_{};
    Entity nextEntity_{1};
    std::size_t nextComponentId_{0};

    std::size_t FindOrCreateArchetype(const Signature& signature);
    void MoveEntity(Entity entity, const Signature& dstSignature, std::optional<std::pair<std::size_t, const void*>> inserted);
};

} // namespace axiom::ecs

#include "AxiomEngine/ecs/ECSWorld.inl"
