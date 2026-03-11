#pragma once

#include <cstdint>
#include <vector>
#include <queue>
#include <cassert>

namespace ECS {

// Entity identifier type
using EntityID = uint32_t;

// Reserved invalid entity ID
constexpr EntityID INVALID_ENTITY = 0;

// Maximum number of entities (can be increased if needed)
constexpr uint32_t MAX_ENTITIES = 1024;

// Maximum number of component types
constexpr uint32_t MAX_COMPONENT_TYPES = 32;

// Component mask - bitfield indicating which components an entity has
using ComponentMask = uint32_t;

// Entity structure - minimal footprint, just ID and component mask
struct Entity {
    EntityID id = INVALID_ENTITY;
    ComponentMask component_mask = 0;
    
    bool is_valid() const { return id != INVALID_ENTITY; }
};

// Entity Manager - handles entity lifecycle
class EntityManager {
public:
    EntityManager() {
        // Initialize with invalid entity at index 0
        entities_.resize(1);
        entities_[0].id = INVALID_ENTITY;
        next_id_ = 1;
    }
    
    // Create a new entity
    EntityID create_entity() {
        EntityID id;
        
        if (!free_ids_.empty()) {
            // Reuse a recycled ID
            id = free_ids_.front();
            free_ids_.pop();
        } else {
            // Allocate new ID
            id = next_id_++;
            if (id >= entities_.size()) {
                entities_.resize(id + 1);
            }
        }
        
        assert(id < MAX_ENTITIES && "Maximum entity count exceeded");
        
        entities_[id].id = id;
        entities_[id].component_mask = 0;
        entity_count_++;
        
        return id;
    }
    
    // Destroy an entity and recycle its ID
    void destroy_entity(EntityID id) {
        if (!is_valid(id)) return;
        
        entities_[id].id = INVALID_ENTITY;
        entities_[id].component_mask = 0;
        free_ids_.push(id);
        entity_count_--;
    }
    
    // Check if an entity ID is valid
    bool is_valid(EntityID id) const {
        return id != INVALID_ENTITY && 
               id < entities_.size() && 
               entities_[id].id == id;
    }
    
    // Get entity reference (for component mask manipulation)
    Entity& get_entity(EntityID id) {
        assert(is_valid(id) && "Invalid entity ID");
        return entities_[id];
    }
    
    const Entity& get_entity(EntityID id) const {
        assert(is_valid(id) && "Invalid entity ID");
        return entities_[id];
    }
    
    // Get current entity count
    uint32_t get_entity_count() const { return entity_count_; }
    
    // Get all valid entity IDs
    std::vector<EntityID> get_all_entities() const {
        std::vector<EntityID> result;
        result.reserve(entity_count_);
        for (size_t i = 1; i < entities_.size(); ++i) {
            if (entities_[i].is_valid()) {
                result.push_back(static_cast<EntityID>(i));
            }
        }
        return result;
    }
    
    // Set component bit in entity's mask
    void set_component_bit(EntityID id, uint32_t component_type) {
        if (!is_valid(id)) return;
        entities_[id].component_mask |= (1u << component_type);
    }
    
    // Clear component bit in entity's mask
    void clear_component_bit(EntityID id, uint32_t component_type) {
        if (!is_valid(id)) return;
        entities_[id].component_mask &= ~(1u << component_type);
    }
    
    // Check if entity has component
    bool has_component_bit(EntityID id, uint32_t component_type) const {
        if (!is_valid(id)) return false;
        return (entities_[id].component_mask & (1u << component_type)) != 0;
    }
    
    // Get component mask for entity
    ComponentMask get_component_mask(EntityID id) const {
        if (!is_valid(id)) return 0;
        return entities_[id].component_mask;
    }

private:
    std::vector<Entity> entities_;
    std::queue<EntityID> free_ids_;
    EntityID next_id_ = 1;
    uint32_t entity_count_ = 0;
};

} // namespace ECS
