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

// Forward declaration.
template <typename... Components>
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

  // Returns an iterable view for all entities with a specific set of
  // components.
  template <typename... Components>
  ECSView<Components...> View(size_t begin_index = 0) {
    // Ensure the view is not empty
    static_assert(sizeof...(Components) > 0,
                  "View must be called with at least one component type.");

    // Pass all pools to the constructor using a pack expansion
    return ECSView<Components...>(begin_index, GetPool<Components>()...);
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
// Components is the component types that an entity must have.
template <typename... Components>
class ECSView {
 private:
  class Iterator {
   public:
    Iterator(std::tuple<ComponentPool<Components>*...>* pools,
             ComponentPoolBase* smallest_pool,
             std::vector<Entity>* dense_to_entity_map,
             size_t index)
        : pools_(pools),
          smallest_pool_(smallest_pool),
          dense_to_entity_map_(dense_to_entity_map),
          index_(index) {
      FindNextValid();
    }

    // Finds the next entity that has all components in 'Components...'.
    void FindNextValid() {
      while (index_ < dense_to_entity_map_->size()) {
        Entity entity = (*dense_to_entity_map_)[index_];

        bool hasAll = true;
        std::apply(
            [&](auto*... pools) {
              auto check = [&](auto* p) {
                // Don't check the pool we're already iterating.
                if (static_cast<ComponentPoolBase*>(p) == smallest_pool_)
                  return true;
                return p->Has(entity);
              };
              hasAll = (check(pools) && ...);
            },
            *pools_);  // Unpack the tuple of all pools

        if (hasAll)
          break;   // Found a valid entity
        index_++;  // Keep searching
      }
    }

    // Returns a tuple of (Entity, Component&, Component&...).
    auto operator*() {
      Entity entity = (*dense_to_entity_map_)[index_];

      // C++17 'apply' unfolds the tuple of pools
      return std::apply(
          [&](auto*... pools) {
            return std::forward_as_tuple(entity, pools->Get(entity)...);
          },
          *pools_);
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
    std::tuple<ComponentPool<Components>*...>* pools_;
    ComponentPoolBase* smallest_pool_;
    std::vector<Entity>* dense_to_entity_map_;
    size_t index_;
  };

 public:
  // Constructor that accepts a variadic pack of component pools.
  ECSView(size_t begin_index, ComponentPool<Components>*... pools)
      : pools_(std::make_tuple(pools...)), begin_index_(begin_index) {
    // Find the smallest pool to iterate.
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
    return Iterator(&pools_, smallest_pool_, dense_to_entity_map_,
                    begin_index_);
  }

  Iterator end() {
    return Iterator(&pools_, smallest_pool_, dense_to_entity_map_,
                    dense_to_entity_map_->size());
  }

 private:
  std::tuple<ComponentPool<Components>*...> pools_;
  ComponentPoolBase* smallest_pool_;
  std::vector<Entity>* dense_to_entity_map_;
  size_t begin_index_;
};

#endif  // TEAPOT_ECS_H
