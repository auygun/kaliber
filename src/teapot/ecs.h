#ifndef TEAPOT_ECS_H
#define TEAPOT_ECS_H

#include <memory>
#include <queue>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "base/log.h"

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
  // Adds a component for an entity.
  T& Add(Entity entity, T component) {
    // Ensure our sparse array is large enough
    if (entity >= sparse_.size()) {
      sparse_.resize(entity + 1, NULL_ENTITY);
    }

    // If entity already has this component, just update it
    if (sparse_[entity] != NULL_ENTITY) {
      dense_[sparse_[entity]] = component;
      return dense_[sparse_[entity]];
    }

    // Add the new component to the back of the dense array
    size_t dense_index = dense_.size();
    dense_.push_back(component);
    entity_to_dense_.push_back(entity);

    // Update the sparse array to point to the new dense index
    sparse_[entity] = dense_index;

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
    Entity last_entity = entity_to_dense_.back();

    // Swap the last element into the position of the one being removed
    dense_[dense_index_to_remove] = last_component;
    entity_to_dense_[dense_index_to_remove] = last_entity;

    // Update the sparse array for the moved entity
    sparse_[last_entity] = dense_index_to_remove;

    // Invalidate the removed entity's sparse entry
    sparse_[entity] = NULL_ENTITY;

    // Pop the (now duplicate) last element
    dense_.pop_back();
    entity_to_dense_.pop_back();
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

  // Data access for the ECSView.
  std::vector<T>& GetDenseData() { return dense_; }
  std::vector<Entity>& GetDenseToEntityMap() { return entity_to_dense_; }

 private:
  // Stores the actual component data, tightly packed.
  std::vector<T> dense_;

  // Maps an Entity ID to an index in the `dense_` array.
  std::vector<size_t> sparse_;

  // Index = dense_ index, Value = Entity ID
  std::vector<Entity> entity_to_dense_;
};

template <typename T, typename... Rest>
class ECSView;

// The main class that holds all entities and components.
class Registry {
 public:
  // Creates a new entity, recycling old IDs if available.
  Entity CreateEntity() {
    if (!free_list_.empty()) {
      Entity id = free_list_.front();
      free_list_.pop();
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
    for (auto const& [type, pool] : component_pools_)
      pool->OnEntityDestroyed(entity);

    free_list_.push(entity);
  }

  // Adds a component of type T to an entity.
  template <typename T>
  T& AddComponent(Entity entity, T component) {
    DCHECK(GetPool<T>());
    return GetPool<T>()->Add(entity, component);
  }

  // Removes a component of type T from an entity.
  template <typename T>
  void RemoveComponent(Entity entity) {
    DCHECK(GetPool<T>());
    GetPool<T>()->Remove(entity);
  }

  // Gets the component of type T for an entity.
  template <typename T>
  T& GetComponent(Entity entity) {
    DCHECK(GetPool<T>());
    return GetPool<T>()->Get(entity);
  }

  // Checks if an entity has a component of type T.
  template <typename T>
  bool HasComponent(Entity entity) {
    auto pool = GetPool<T>();
    return pool && pool->Has(entity);
  }

  // Create the ComponentPool for type T.
  template <typename T>
  ComponentPool<T>* CreatePool() {
    std::type_index type_index = std::type_index(typeid(T));

    // Create a new pool for this component type if doesn't exist.
    if (component_pools_.find(type_index) == component_pools_.end())
      component_pools_[type_index] = std::make_unique<ComponentPool<T>>();

    // Downcast and return the pointer
    return static_cast<ComponentPool<T>*>(component_pools_[type_index].get());
  }

  // Access to the ComponentPool for type T.
  template <typename T>
  ComponentPool<T>* GetPool() {
    std::type_index type_index = std::type_index(typeid(T));

    // Create a new pool for this component type if doesn't exist.
    if (component_pools_.find(type_index) == component_pools_.end())
      return nullptr;

    // Downcast and return the pointer
    return static_cast<ComponentPool<T>*>(component_pools_[type_index].get());
  }

  // Returns an iterable view for all entities with component T, and optionally
  // all components in Rest... Iteration is performed on the pool of type T.
  // For best performance, T should be the rarest component in the query.
  template <typename T, typename... Rest>
  ECSView<T, Rest...> View() {
    // We iterate on the first component type, T
    return ECSView<T, Rest...>(this, GetPool<T>());
  }

 private:
  // Entity management
  size_t next_entity_id_ = 0;
  std::queue<Entity> free_list_;

  // Component management
  std::unordered_map<std::type_index, std::unique_ptr<ComponentPoolBase>>
      component_pools_;
};

// An iterable object that a system uses to loop over all entities with a
// specific set of components.
// T is the primary component type to iterate over.
// Rest is the other component types that an entity must also have.
template <typename T, typename... Rest>
class ECSView {
 public:
  class Iterator {
   public:
    Iterator(Registry* registry, ComponentPool<T>* pool, size_t index)
        : registry_(registry), pool_(pool), index_(index) {
      // Find the first valid entity that has all components
      FindNextValid();
    }

    // Finds the next entity in the main pool that also has all components in
    // 'Rest...'.
    void FindNextValid() {
      auto& entities = pool_->GetDenseToEntityMap();

      while (index_ < entities.size()) {
        Entity entity = entities[index_];

        // If we are checking for more than one component...
        if constexpr (sizeof...(Rest) > 0) {
          // Use a C++17 fold expression to check all components
          if (!(registry_->HasComponent<Rest>(entity) && ...)) {
            // This entity is missing a required component, skip it
            index_++;
            continue;
          }
        }

        // This entity has all required components, stop here
        break;
      }
    }

    // Returns the (Entity, Component&) pair or (Entity, Component&,
    // Component&...) tuple.
    auto operator*() {
      Entity entity = pool_->GetDenseToEntityMap()[index_];

      if constexpr (sizeof...(Rest) == 0) {
        // Single component case. Return std::pair<Entity, T&>
        return std::pair<Entity, T&>{entity, pool_->GetDenseData()[index_]};

      } else {
        // Multi component case. Return std::tuple<Entity, T&, Rest&...>
        return std::forward_as_tuple(entity, pool_->GetDenseData()[index_],
                                     registry_->GetComponent<Rest>(entity)...);
      }
    }

    bool operator!=(const Iterator& other) const {
      return index_ != other.index_;
    }

    Iterator& operator++() {
      index_++;
      FindNextValid();
      return *this;
    }

   private:
    Registry* registry_;
    ComponentPool<T>* pool_;  // The pool we are iterating over
    size_t index_;
  };

  ECSView(Registry* registry, ComponentPool<T>* pool)
      : registry_(registry), pool_(pool) {}

  Iterator begin() { return Iterator(registry_, pool_, 0); }
  Iterator end() {
    return Iterator(registry_, pool_, pool_->GetDenseData().size());
  }

 private:
  Registry* registry_;
  ComponentPool<T>* pool_;
};

#endif  // TEAPOT_ECS_H
