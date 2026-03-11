#pragma once

#include "entity.h"
#include "components/all_components.h"
#include <array>
#include <cassert>
#include <optional>

namespace ECS {

// Component type identifiers
enum class ComponentType : uint32_t {
    Transform2_5D = 0,
    Transform3D = 1,
    Sprite = 2,
    Light = 3,
    ShadowCaster = 4,
    Emissive = 5,
    
    // Add new component types here before COUNT
    COUNT
};

static_assert(static_cast<uint32_t>(ComponentType::COUNT) <= MAX_COMPONENT_TYPES,
              "Too many component types defined");

// Generic component array - sparse storage indexed by EntityID
template<typename T>
class ComponentArray {
public:
    ComponentArray() {
        // Initialize all slots as empty
        valid_.fill(false);
    }
    
    // Add component to entity
    T& add(EntityID id) {
        assert(id != INVALID_ENTITY && id < MAX_ENTITIES && "Invalid entity ID");
        valid_[id] = true;
        data_[id] = T{};
        return data_[id];
    }
    
    // Add component with initial value
    T& add(EntityID id, const T& value) {
        assert(id != INVALID_ENTITY && id < MAX_ENTITIES && "Invalid entity ID");
        valid_[id] = true;
        data_[id] = value;
        return data_[id];
    }
    
    // Get component (returns nullptr if not present)
    T* get(EntityID id) {
        if (id == INVALID_ENTITY || id >= MAX_ENTITIES || !valid_[id]) {
            return nullptr;
        }
        return &data_[id];
    }
    
    const T* get(EntityID id) const {
        if (id == INVALID_ENTITY || id >= MAX_ENTITIES || !valid_[id]) {
            return nullptr;
        }
        return &data_[id];
    }
    
    // Check if entity has this component
    bool has(EntityID id) const {
        return id != INVALID_ENTITY && id < MAX_ENTITIES && valid_[id];
    }
    
    // Remove component from entity
    void remove(EntityID id) {
        if (id != INVALID_ENTITY && id < MAX_ENTITIES) {
            valid_[id] = false;
        }
    }
    
    // Get all entities with this component
    std::vector<EntityID> get_all_entities() const {
        std::vector<EntityID> result;
        for (EntityID i = 1; i < MAX_ENTITIES; ++i) {
            if (valid_[i]) {
                result.push_back(i);
            }
        }
        return result;
    }

private:
    std::array<T, MAX_ENTITIES> data_;
    std::array<bool, MAX_ENTITIES> valid_;
};

// Component Registry - manages all component arrays
class ComponentRegistry {
public:
    // Get the component array for a specific type
    template<typename T>
    ComponentArray<T>& get_array();
    
    // Add component to entity
    template<typename T>
    T& add_component(EntityManager& em, EntityID id) {
        auto& arr = get_array<T>();
        em.set_component_bit(id, static_cast<uint32_t>(get_component_type<T>()));
        return arr.add(id);
    }
    
    // Add component with initial value
    template<typename T>
    T& add_component(EntityManager& em, EntityID id, const T& value) {
        auto& arr = get_array<T>();
        em.set_component_bit(id, static_cast<uint32_t>(get_component_type<T>()));
        return arr.add(id, value);
    }
    
    // Get component from entity
    template<typename T>
    T* get_component(EntityID id) {
        return get_array<T>().get(id);
    }
    
    template<typename T>
    const T* get_component(EntityID id) const {
        // Const cast needed because get_array is non-const (for lazy init)
        return const_cast<ComponentRegistry*>(this)->get_array<T>().get(id);
    }
    
    // Check if entity has component
    template<typename T>
    bool has_component(EntityID id) const {
        return const_cast<ComponentRegistry*>(this)->get_array<T>().has(id);
    }
    
    // Remove component from entity
    template<typename T>
    void remove_component(EntityManager& em, EntityID id) {
        get_array<T>().remove(id);
        em.clear_component_bit(id, static_cast<uint32_t>(get_component_type<T>()));
    }
    
    // Remove all components when entity is destroyed
    void remove_all_components(EntityManager& em, EntityID id);
    
private:
    // Get ComponentType enum for a given component struct
    template<typename T>
    static constexpr ComponentType get_component_type();
    
    // Component arrays for each type
    ComponentArray<Transform2_5DComponent> transform2_5d_components_;
    ComponentArray<Transform3DComponent> transform3d_components_;
    ComponentArray<SpriteComponent> sprite_components_;
    ComponentArray<LightComponent> light_components_;
    ComponentArray<ShadowCasterComponent> shadow_caster_components_;
    ComponentArray<EmissiveComponent> emissive_components_;
};

// Template specializations for get_component_type
template<> constexpr ComponentType ComponentRegistry::get_component_type<Transform2_5DComponent>() { return ComponentType::Transform2_5D; }
template<> constexpr ComponentType ComponentRegistry::get_component_type<Transform3DComponent>() { return ComponentType::Transform3D; }
template<> constexpr ComponentType ComponentRegistry::get_component_type<SpriteComponent>() { return ComponentType::Sprite; }
template<> constexpr ComponentType ComponentRegistry::get_component_type<LightComponent>() { return ComponentType::Light; }
template<> constexpr ComponentType ComponentRegistry::get_component_type<ShadowCasterComponent>() { return ComponentType::ShadowCaster; }
template<> constexpr ComponentType ComponentRegistry::get_component_type<EmissiveComponent>() { return ComponentType::Emissive; }

// Template specializations for get_array
template<> inline ComponentArray<Transform2_5DComponent>& ComponentRegistry::get_array<Transform2_5DComponent>() { return transform2_5d_components_; }
template<> inline ComponentArray<Transform3DComponent>& ComponentRegistry::get_array<Transform3DComponent>() { return transform3d_components_; }
template<> inline ComponentArray<SpriteComponent>& ComponentRegistry::get_array<SpriteComponent>() { return sprite_components_; }
template<> inline ComponentArray<LightComponent>& ComponentRegistry::get_array<LightComponent>() { return light_components_; }
template<> inline ComponentArray<ShadowCasterComponent>& ComponentRegistry::get_array<ShadowCasterComponent>() { return shadow_caster_components_; }
template<> inline ComponentArray<EmissiveComponent>& ComponentRegistry::get_array<EmissiveComponent>() { return emissive_components_; }

// Implementation of remove_all_components
inline void ComponentRegistry::remove_all_components(EntityManager& em, EntityID id) {
    transform2_5d_components_.remove(id);
    transform3d_components_.remove(id);
    sprite_components_.remove(id);
    light_components_.remove(id);
    shadow_caster_components_.remove(id);
    emissive_components_.remove(id);
    // Clear all component bits
    em.get_entity(id).component_mask = 0;
}

} // namespace ECS
