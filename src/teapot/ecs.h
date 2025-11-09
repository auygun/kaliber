#ifndef TEAPOT_ECS_H
#define TEAPOT_ECS_H

#include <deque>
#include <limits>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/log.h"

namespace eng {

class Registry;

using Entity = uint32_t;
const Entity NULL_ENTITY = (uint32_t)-1;

// Base interface to allow storing all ComponentPool<T> in a single map within
// the Registry.
class ComponentPoolBase {
 public:
  virtual ~ComponentPoolBase() = default;
  // Called by the Registry when an entity is destroyed
  virtual void OnEntityDestroyed(Entity entity) = 0;
};

// A "Sparse Set" pool for a single component type T. All component data (T) is
// packed tightly into a vector. Iterating through this continuous block of
// memory is cache-friendly.
template <typename T>
class ComponentPool : public ComponentPoolBase {
 public:
  ComponentPool(std::vector<std::vector<ComponentPoolBase*>>* entity_pools)
      : entity_pools_{entity_pools} {}

  // Adds a pre-existing component to an entity via copy.
  T& Add(Entity entity, T&& component) {
    return Emplace(entity, std::move(component));
  }

  // Adds a pre-existing component to an entity via move.
  T& Add(Entity entity, const T& component) {
    return Emplace(entity, component);
  }

  // Constructs a component in-place for a given entity. This is the most
  // efficient method, as it forwards arguments directly to the component's
  // constructor within the dense array, avoiding intermediate copies or moves.
  template <typename... Args>
  T& Emplace(Entity entity, Args&&... args) {
    // Ensure our sparse array is large enough
    if (entity >= sparse_.size())
      sparse_.resize(entity + 1, NULL_ENTITY);

    // If entity already has this component, overwrite it.
    if (sparse_[entity] != NULL_ENTITY) {
      dense_[sparse_[entity]] = T(std::forward<Args>(args)...);
      return dense_[sparse_[entity]];
    }

    // Add the new component to the back of the dense array
    sparse_[entity] = dense_.size();
    dense_.emplace_back(std::forward<Args>(args)...);
    dense_to_entity_.push_back(entity);

    TrackPoolForEntity(entity);

    return dense_.back();
  }

  // Removes a component from an entity using O(1) swap-and-pop.
  void Remove(Entity entity) {
    if (!Has(entity))
      return;

    size_t dense_index_to_remove = sparse_[entity];
    size_t last_dense_index = dense_.size() - 1;
    Entity last_entity = dense_to_entity_[last_dense_index];

    // Swap the last element into the position of the one being removed
    dense_[dense_index_to_remove] = std::move(dense_[last_dense_index]);
    dense_to_entity_[dense_index_to_remove] = last_entity;

    // Update the sparse array for the moved entity
    sparse_[last_entity] = dense_index_to_remove;

    // Pop the last element
    dense_.pop_back();
    dense_to_entity_.pop_back();

    // Invalidate the removed entity
    sparse_[entity] = NULL_ENTITY;

    UntrackPoolForEntity(entity);
  }

  // Removes all components from this pool.
  void RemoveAll() {
    if (dense_.empty())
      return;

    // Untrack every entity that is currently in this pool.
    for (Entity entity : dense_to_entity_)
      UntrackPoolForEntity(entity);

    // Clear the internal vectors to remove all elements.
    dense_.clear();
    sparse_.clear();
    dense_to_entity_.clear();
  }

  // Gets the component for an entity.
  T& Get(Entity entity) {
    DCHECK(Has(entity)) << "Entity does not have this component.";
    return dense_[sparse_[entity]];
  }

  // Checks if an entity has this component.
  bool Has(Entity entity) const {
    return entity < sparse_.size() && sparse_[entity] != NULL_ENTITY;
  }

  // Checks if the pool is empty (i.e., no entities have this component).
  bool IsEmpty() const { return dense_.empty(); }

  // Interface implementation for when an entity is destroyed.
  void OnEntityDestroyed(Entity entity) override {
    if (Has(entity))
      Remove(entity);
  }

  // Data access for the View.
  std::vector<T>& GetDenseData() { return dense_; }
  std::vector<Entity>& GetDenseToEntityMap() { return dense_to_entity_; }

 private:
  // Stores the actual component data, tightly packed.
  std::vector<T> dense_;

  // Maps an Entity ID to an index in the `dense_` array.
  std::vector<size_t> sparse_;

  // Index = dense_ index, Value = Entity ID
  std::vector<Entity> dense_to_entity_;

  std::vector<std::vector<ComponentPoolBase*>>* entity_pools_;

  void TrackPoolForEntity(Entity entity) {
    if (entity >= entity_pools_->size())
      entity_pools_->resize(entity + 1);
    (*entity_pools_)[entity].push_back(this);
  }

  void UntrackPoolForEntity(Entity entity) {
    if (entity >= entity_pools_->size())
      return;
    auto& pools = (*entity_pools_)[entity];
    for (size_t i = 0; i < pools.size(); ++i) {
      if (pools[i] == this) {
        pools[i] = pools.back();
        pools.pop_back();
        return;
      }
    }
  }
};

// Forward declaration.
template <typename... Components>
class View;

// The main class that holds all entities and components.
class Registry {
 public:
  // Creates a new entity, recycling old IDs if available.
  Entity CreateEntity() {
    Entity id;
    if (!free_list_.empty()) {
      id = free_list_.front();
      free_list_.pop_front();
    } else {
      id = next_entity_id_++;
      // Ensure pool tracking vector is large enough
      if (id >= entity_pools_.size())
        entity_pools_.resize(id + 1);
    }
    return id;
  }

  // Destroys an entity, removing all its components and recycling its ID.
  void DestroyEntity(Entity entity) {
    if (entity == NULL_ENTITY)
      return;

    // Only notify pools this entity actually has.
    if (entity < entity_pools_.size()) {
      // Safely extract pools to a local vector before iteration
      std::vector<ComponentPoolBase*> pools;
      std::swap(pools, entity_pools_[entity]);
      for (auto* pool : pools)
        pool->OnEntityDestroyed(entity);
    }

    free_list_.push_back(entity);
  }

  // Single generic AddComponent that handles both copy and move.
  template <typename T>
  std::decay_t<T>& AddComponent(Entity entity, T&& component) {
    return GetOrCreatePool<T>()->Emplace(entity, std::forward<T>(component));
  }

  // Constructs a component in-place for a given entity. This is the most
  // efficient way to add a component as it avoids any intermediate copies or
  // moves by forwarding arguments directly to the component's constructor.
  template <typename T, typename... Args>
  T& EmplaceComponent(Entity entity, Args&&... args) {
    return GetOrCreatePool<T>()->Emplace(entity, std::forward<Args>(args)...);
  }

  // Removes a component of type T from an entity.
  template <typename T>
  void RemoveComponent(Entity entity) {
    auto pool = GetPool<T>();
    if (pool)
      pool->Remove(entity);
  }

  // Removes all components of type T from all entities.
  template <typename T>
  void RemoveAll() {
    auto pool = GetPool<T>();
    if (pool)
      pool->RemoveAll();
  }

  // Gets the component of type T for an entity.
  template <typename T>
  T& GetComponent(Entity entity) {
    return GetOrCreatePool<T>()->Get(entity);
  }

  // Checks if an entity has a component of type T.
  template <typename T>
  bool HasComponent(Entity entity) {
    auto pool = GetPool<T>();
    return pool && pool->Has(entity);
  }

  // Checks if the component pool for type T is empty.
  // Returns true if the pool doesn't exist or if it has no components.
  template <typename T>
  bool IsEmpty() {
    auto pool = GetPool<T>();
    return pool ? pool->IsEmpty() : true;
  }

  // Get or create the ComponentPool for type T.
  // Uses std::decay_t<T> to ensure we get the raw component type for the pool.
  template <typename T>
  ComponentPool<std::decay_t<T>>* GetOrCreatePool() {
    using RawType = std::decay_t<T>;
    std::type_index type_index = std::type_index(typeid(RawType));
    auto it = component_pools_.find(type_index);

    // Create a new pool for this component type if doesn't exist.
    if (it == component_pools_.end()) {
      auto pool = std::make_unique<ComponentPool<RawType>>(&entity_pools_);
      auto* raw_pool = pool.get();
      component_pools_[type_index] = std::move(pool);
      return raw_pool;
    }
    return static_cast<ComponentPool<RawType>*>(it->second.get());
  }

  // Gets the ComponentPool for type T, but does not create it.
  // Returns nullptr if the pool does not exist.
  template <typename T>
  ComponentPool<std::decay_t<T>>* GetPool() {
    using RawType = std::decay_t<T>;
    std::type_index type_index = std::type_index(typeid(RawType));
    auto it = component_pools_.find(type_index);

    if (it == component_pools_.end())
      return nullptr;

    return static_cast<ComponentPool<RawType>*>(it->second.get());
  }

  // Returns an iterable view for all entities with a specific set of
  // components.
  template <typename... Components>
  View<Components...> View() {
    static_assert(sizeof...(Components) > 0, "View must have > 0 components.");
    return eng::View<Components...>(GetOrCreatePool<Components>()...);
  }

 private:
  // Entity management
  size_t next_entity_id_ = 0;
  std::deque<Entity> free_list_;

  // Ownership of pools by type.
  std::unordered_map<std::type_index, std::unique_ptr<ComponentPoolBase>>
      component_pools_;

  // Tracks which pools an entity has for fast destruction.
  std::vector<std::vector<ComponentPoolBase*>> entity_pools_;
};

// An iterable object that a system uses to loop over all entities with a
// specific set of components.
// Components is the component types that an entity must have.
template <typename... Components>
class View {
 private:
  class Iterator {
   public:
    Iterator(const std::tuple<ComponentPool<Components>*...>& pools,
             ComponentPoolBase* smallest_pool,
             std::vector<Entity>* dense_to_entity_map,
             size_t index)
        : pools_(pools),
          smallest_pool_(smallest_pool),
          dense_to_entity_map_(dense_to_entity_map),
          index_(index) {
      // If we have multiple components, we need to find the first valid entity
      // that has ALL of them.
      if constexpr (sizeof...(Components) > 1)
        FindNextValid();
    }

    // Finds the next entity that has all components in 'Components...'.
    void FindNextValid() {
      while (index_ < dense_to_entity_map_->size()) {
        Entity entity = (*dense_to_entity_map_)[index_];

        // Check if this entity exists in all other pools
        bool has_all = true;
        std::apply(
            [&](auto*... pools) {
              auto check = [&](auto* p) {
                return (static_cast<ComponentPoolBase*>(p) == smallest_pool_) ||
                       p->Has(entity);
              };
              // Fold expression to check all pools
              has_all = (check(pools) && ...);
            },
            pools_);

        if (has_all)
          return;  // Found a valid entity
        index_++;  // Keep searching
      }
    }

    // Returns a tuple of (Entity, Component&, Component&...).
    // We return explicitly by tuple to ensure Entity is copied by value,
    // while components are returned by reference.
    auto operator*() {
      Entity entity = (*dense_to_entity_map_)[index_];

      if constexpr (sizeof...(Components) == 1) {
        // Optimized single component case.
        return std::tuple<Entity, Components&...>(
            entity, std::get<0>(pools_)->GetDenseData()[index_]);
      } else {
        // Multi component case. Return std::tuple<Entity, Components&...>
        return std::apply(
            [entity](auto*... pools) {
              return std::tuple<Entity, Components&...>(entity,
                                                        pools->Get(entity)...);
            },
            pools_);
      }
    }

    bool operator!=(const Iterator& other) const {
      return index_ != other.index_;
    }

    Iterator& operator++() {
      index_++;
      if constexpr (sizeof...(Components) > 1)
        FindNextValid();
      return *this;
    }

   private:
    const std::tuple<ComponentPool<Components>*...>& pools_;
    ComponentPoolBase* smallest_pool_;
    std::vector<Entity>* dense_to_entity_map_;
    size_t index_;
  };

 public:
  // Constructor that accepts a variadic pack of component pools.
  View(ComponentPool<Components>*... pools)
      : pools_(std::make_tuple(pools...)) {
    // Find the smallest pool to drive the main iteration loop efficiently.
    size_t minSize = std::numeric_limits<size_t>::max();
    auto findSmallest = [&](auto* pool) {
      if (pool->GetDenseData().size() < minSize) {
        minSize = pool->GetDenseData().size();
        smallest_pool_ = pool;
        dense_to_entity_map_ = &pool->GetDenseToEntityMap();
      }
    };
    (findSmallest(pools), ...);
  }

  Iterator begin() {
    return Iterator(pools_, smallest_pool_, dense_to_entity_map_, 0);
  }

  Iterator end() {
    return Iterator(pools_, smallest_pool_, dense_to_entity_map_,
                    dense_to_entity_map_->size());
  }

 private:
  std::tuple<ComponentPool<Components>*...> pools_;
  ComponentPoolBase* smallest_pool_ = nullptr;
  std::vector<Entity>* dense_to_entity_map_ = nullptr;
};

}  // namespace eng

#endif  // TEAPOT_ECS_H
