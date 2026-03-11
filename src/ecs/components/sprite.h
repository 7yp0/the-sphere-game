#pragma once

// Sprite and Emissive Components for ECS
// Handles visual representation of entities

#include "../../types.h"
#include "../../renderer/renderer.h"
#include "../../renderer/animation.h"

namespace ECS {

// ============================================================================
// SpriteComponent - Visual representation of an entity
// ============================================================================
// Supports both static sprites and animated sprites (mutually exclusive).
// When animation is set, it takes precedence over static texture.
//
// Usage:
//   Static sprite:  Set texture, base_size, uv_range
//   Animated:       Set animation pointer via set_animation()
// ============================================================================

struct SpriteComponent {
    // Static sprite properties
    Renderer::TextureID texture;          // Main texture (diffuse)
    Renderer::TextureID normal_map;       // Normal map for lighting (0 = none)
    Vec2 base_size;                       // Base sprite size in pixels
    Vec4 uv_range;                        // UV coords (minU, minV, maxU, maxV) for spritesheets
    
    // Animation (optional)
    Renderer::SpriteAnimation* animation; // Animation data (nullptr = static sprite)
    
    // Rendering options
    PivotPoint pivot;
    bool visible;
    bool flip_x;
    bool flip_y;
    
    SpriteComponent() 
        : texture(0)
        , normal_map(0)
        , base_size(32, 32)
        , uv_range(0, 0, 1, 1)
        , animation(nullptr)
        , pivot(PivotPoint::CENTER)
        , visible(true)
        , flip_x(false)
        , flip_y(false) {}
    
    // Check if this sprite uses animation
    bool is_animated() const { return animation != nullptr; }
    
    // Set animation (disables static texture rendering)
    void set_animation(Renderer::SpriteAnimation* anim) {
        animation = anim;
        if (anim) {
            texture = anim->texture;  // Sync texture
        }
    }
    
    // Clear animation (returns to static texture)
    void clear_animation() {
        animation = nullptr;
    }
    
    // Get current UV range (from animation frame or static uv_range)
    Vec4 get_current_uv() const {
        if (animation && !animation->frames.empty()) {
            const auto& frame = animation->frames[animation->current_frame];
            return Vec4(frame.u0, frame.v0, frame.u1, frame.v1);
        }
        return uv_range;
    }
    
    // Get current texture (from animation or static)
    Renderer::TextureID get_current_texture() const {
        if (animation) {
            return animation->texture;
        }
        return texture;
    }
};

// ============================================================================
// EmissiveComponent - Self-illumination for glowing objects
// ============================================================================
// Allows sprites to emit light independently of scene lighting.
// Values > 1.0 create HDR glow effects.
//
// Examples:
//   Candle flame: Vec3(2.0, 1.5, 0.5) - bright orange glow
//   Neon sign:    Vec3(0.0, 3.0, 2.0) - bright cyan/green
//   Dim indicator: Vec3(0.2, 0.2, 0.0) - subtle yellow
//
// Note: EmissiveComponent makes the sprite itself glow.
//       To illuminate OTHER objects, add a LightComponent.
// ============================================================================

struct EmissiveComponent {
    Vec3 emissive_color;    // RGB emission color (values > 1.0 for HDR glow)
    float intensity;        // Emission intensity multiplier
    
    EmissiveComponent() 
        : emissive_color(0, 0, 0)
        , intensity(1.0f) {}
    
    EmissiveComponent(Vec3 color, float inten = 1.0f)
        : emissive_color(color)
        , intensity(inten) {}
    
    // Get final emission color (color * intensity)
    Vec3 get_emission() const {
        return Vec3(
            emissive_color.x * intensity,
            emissive_color.y * intensity,
            emissive_color.z * intensity
        );
    }
    
    // Check if this component actually emits light
    bool is_emitting() const {
        return intensity > 0.0f && 
               (emissive_color.x > 0.0f || emissive_color.y > 0.0f || emissive_color.z > 0.0f);
    }
};

// ============================================================================
// Helper Functions
// ============================================================================

namespace SpriteHelpers {

// Update animation (call each frame with delta time)
// Returns true if animation looped
inline bool update_animation(SpriteComponent& sprite, float delta_time) {
    if (sprite.animation) {
        return Renderer::animate(sprite.animation, delta_time);
    }
    return false;
}

// Reset animation to first frame
inline void reset_animation(SpriteComponent& sprite) {
    if (sprite.animation) {
        Renderer::animation_reset(sprite.animation);
    }
}

// Create UV range for a specific cell in a grid-based spritesheet
// cell_x, cell_y = cell position (0-indexed)
// cols, rows = grid dimensions
inline Vec4 create_spritesheet_uv(uint32_t cell_x, uint32_t cell_y, 
                                   uint32_t cols, uint32_t rows) {
    float cell_width = 1.0f / cols;
    float cell_height = 1.0f / rows;
    
    float u0 = cell_x * cell_width;
    float v0 = cell_y * cell_height;
    float u1 = u0 + cell_width;
    float v1 = v0 + cell_height;
    
    return Vec4(u0, v0, u1, v1);
}

// Apply flip transformations to UV coordinates
inline Vec4 apply_flip(Vec4 uv, bool flip_x, bool flip_y) {
    Vec4 result = uv;
    if (flip_x) {
        float temp = result.x;
        result.x = result.z;  // Swap u0 and u1
        result.z = temp;
    }
    if (flip_y) {
        float temp = result.y;
        result.y = result.w;  // Swap v0 and v1
        result.w = temp;
    }
    return result;
}

} // namespace SpriteHelpers

} // namespace ECS
