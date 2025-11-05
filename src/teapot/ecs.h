#ifndef TEAPOT_ECS_H
#define TEAPOT_ECS_H

#include <deque>
#include <limits>
#include <memory>
#include <typeindex>
#include <unordered_map>
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
  // Adds a pre-existing component to an entity via copy or move. This is a
  // convenience wrapper around Emplace.
  template <typename U>
  T& Add(Entity entity, U&& component) {
    return Emplace(entity, std::forward<U>(component));
  }

  // Constructs a component in-place for a given entity. This is the most
  // efficient method as it forwards arguments directly to the component's
  // constructor.
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

    return dense_.back();
  }

  // Removes a component from an entity. Uses the swap-and-pop O(1) operation.
  void Remove(Entity entity) {
    if (!Has(entity))
      return;

    // Get the index of the component to remove
    size_t dense_index_to_remove = sparse_[entity];

    // Get the last element's data
    T& last_component = dense_.back();
    Entity last_entity = dense_to_entity_.back();

    // Swap the last element into the position of the one being removed
    dense_[dense_index_to_remove] = last_component;
    dense_to_entity_[dense_index_to_remove] = last_entity;

    // Update the sparse array for the moved entity
    sparse_[last_entity] = dense_index_to_remove;

    // Invalidate the removed entity's sparse entry
    sparse_[entity] = NULL_ENTITY;

    // Pop the (now duplicate) last element
    dense_.pop_back();
    dense_to_entity_.pop_back();
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
};

// Forward declaration.
template <typename... Components>
class View;

// The main class that holds all entities and components.
class Registry {
 public:
  // Creates a new entity, recycling old IDs if available.
  Entity CreateEntity() {
    if (!free_list_.empty()) {
      Entity id = free_list_.front();
      free_list_.pop_front();
      return id;
    }
    return next_entity_id_++;
  }

  // Destroys an entity, removing all its components and adding its ID to the
  // free list for recycling.
  void DestroyEntity(Entity entity) {
    if (entity == NULL_ENTITY)
      return;

    // Notify all pools that this entity is gone
    for (auto* pool : pool_list_)
      pool->OnEntityDestroyed(entity);

    free_list_.push_back(entity);
  }

  // Adds a pre-existing component to an entity via copy or move. This is a
  // convenience wrapper around EmplaceComponent.
  template <typename T>
  std::decay_t<T>& AddComponent(Entity entity, T&& component) {
    using ComponentType = std::decay_t<T>;
    return GetPool<ComponentType>()->Emplace(entity,
                                             std::forward<T>(component));
  }

  // Constructs a component in-place for a given entity. This is the most
  // efficient way to add a component as it avoids any intermediate copies or
  // moves.
  template <typename T, typename... Args>
  T& EmplaceComponent(Entity entity, Args&&... args) {
    return GetPool<T>()->Emplace(entity, std::forward<Args>(args)...);
  }

  // Removes a component of type T from an entity.
  template <typename T>
  void RemoveComponent(Entity entity) {
    GetPool<T>()->Remove(entity);
  }

  // Gets the component of type T for an entity.
  template <typename T>
  T& GetComponent(Entity entity) {
    return GetPool<T>()->Get(entity);
  }

  // Checks if an entity has a component of type T.
  template <typename T>
  bool HasComponent(Entity entity) {
    auto pool = GetPool<T>();
    return pool && pool->Has(entity);
  }

  // Get (or create) the ComponentPool for type T.
  template <typename T>
  ComponentPool<std::decay_t<T>>* GetPool() {
    using RawType = std::decay_t<T>;
    std::type_index type_index = std::type_index(typeid(RawType));
    auto it = component_pools_.find(type_index);

    // Create a new pool for this component type if doesn't exist.
    if (it == component_pools_.end()) {
      auto pool = std::make_unique<ComponentPool<RawType>>();
      pool_list_.push_back(pool.get());
      auto* raw_pool = pool.get();
      component_pools_[type_index] = std::move(pool);
      return raw_pool;
    }
    return static_cast<ComponentPool<RawType>*>(it->second.get());
  }

  // Returns an iterable view for all entities with a specific set of
  // components.
  template <typename... Components>
  View<Components...> View(size_t begin_index = 0) {
    // Ensure the view is not empty
    static_assert(sizeof...(Components) > 0,
                  "View must be called with at least one component type.");

    // Pass all pools to the constructor using a pack expansion
    return eng::View<Components...>(begin_index, GetPool<Components>()...);
  }

 private:
  // Entity management
  size_t next_entity_id_ = 0;
  std::deque<Entity> free_list_;

  // Component management
  std::unordered_map<std::type_index, std::unique_ptr<ComponentPoolBase>>
      component_pools_;

  // Side-list for fast iteration during destruction
  std::vector<ComponentPoolBase*> pool_list_;
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
      if constexpr (sizeof...(Components) > 1)
        FindNextValid();
    }

    // Finds the next entity that has all components in 'Components...'.
    void FindNextValid() {
      while (index_ < dense_to_entity_map_->size()) {
        Entity entity = (*dense_to_entity_map_)[index_];

        bool has_all = true;
        std::apply(
            [&](auto*... pools) {
              auto check = [&](auto* p) {
                return (static_cast<ComponentPoolBase*>(p) == smallest_pool_) ||
                       p->Has(entity);
              };
              has_all = (check(pools) && ...);
            },
            pools_);

        if (has_all)
          return;  // Found a valid entity
        index_++;  // Keep searching
      }
    }

    // Returns a tuple of (Entity, Component&, Component&...).
    auto operator*() {
      Entity entity = (*dense_to_entity_map_)[index_];

      if constexpr (sizeof...(Components) == 1) {
        // Single component case. Return std::tuple<Entity, Component&>
        return std::tuple<Entity, Components&...>(
            entity, std::get<0>(pools_)->GetDenseData()[index_]);
      } else {
        // Multi component case. Return std::tuple<Entity, Components&...>
        return std::apply(
            [entity](auto*... pools) {
              // Use explicitly typed std::tuple to mix value (Entity) and
              // references (Components&)
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
  View(size_t begin_index, ComponentPool<Components>*... pools)
      : pools_(std::make_tuple(pools...)), begin_index_(begin_index) {
    // Find smallest pool to optimized main iteration loop.
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
    return Iterator(pools_, smallest_pool_, dense_to_entity_map_, begin_index_);
  }

  Iterator end() {
    return Iterator(pools_, smallest_pool_, dense_to_entity_map_,
                    dense_to_entity_map_->size());
  }

 private:
  std::tuple<ComponentPool<Components>*...> pools_;
  ComponentPoolBase* smallest_pool_ = nullptr;
  std::vector<Entity>* dense_to_entity_map_ = nullptr;
  size_t begin_index_;
};

}  // namespace eng

#endif  // TEAPOT_ECS_H
