#pragma once

// Main ECS include file
// Include this to get the complete ECS system

#include "entity.h"
#include "component.h"
#include "components/all_components.h"

namespace ECS {

// World - combines EntityManager and ComponentRegistry for convenience
class World {
public:
    // Create a new entity
    EntityID create_entity() {
        return entities.create_entity();
    }
    
    // Destroy an entity and all its components
    void destroy_entity(EntityID id) {
        components.remove_all_components(entities, id);
        entities.destroy_entity(id);
    }
    
    // Check if entity is valid
    bool is_valid(EntityID id) const {
        return entities.is_valid(id);
    }
    
    // Add component to entity
    template<typename T>
    T& add_component(EntityID id) {
        return components.add_component<T>(entities, id);
    }
    
    // Add component with initial value
    template<typename T>
    T& add_component(EntityID id, const T& value) {
        return components.add_component<T>(entities, id, value);
    }
    
    // Get component from entity (returns nullptr if not present)
    template<typename T>
    T* get_component(EntityID id) {
        return components.get_component<T>(id);
    }
    
    template<typename T>
    const T* get_component(EntityID id) const {
        return components.get_component<T>(id);
    }
    
    // Check if entity has component
    template<typename T>
    bool has_component(EntityID id) const {
        return components.has_component<T>(id);
    }
    
    // Remove component from entity
    template<typename T>
    void remove_component(EntityID id) {
        components.remove_component<T>(entities, id);
    }
    
    // Get all entities
    std::vector<EntityID> get_all_entities() const {
        return entities.get_all_entities();
    }
    
    // Get entity count
    uint32_t get_entity_count() const {
        return entities.get_entity_count();
    }
    
    // Get all entities with a specific component
    template<typename T>
    std::vector<EntityID> get_entities_with() {
        return components.get_array<T>().get_all_entities();
    }

    EntityManager entities;
    ComponentRegistry components;
};

} // namespace ECS
