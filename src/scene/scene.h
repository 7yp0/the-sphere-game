#pragma once

#include "types.h"
#include "renderer/renderer.h"
#include "collision/polygon_utils.h"
#include <cstdint>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>

// Undefine Windows macros that conflict with std::min/std::max
#undef min
#undef max

namespace Scene {

// =============================================================================
// 2.5D + 3D HYBRID RENDERING SYSTEM
// =============================================================================
// This engine uses TWO coordinate systems that work together:
//
// 1. 2.5D SYSTEM (Background, Props, Player)
//    - Objects have X,Y screen position only
//    - Z-DEPTH is AUTOMATICALLY derived from HEIGHT MAP (Y position → sample → Z)
//    - Props/Player move on 2D plane, height map determines their depth
//    - Depth scaling: objects further back (higher Z) render smaller
//
// 2. 3D LIGHTING SYSTEM (PointLights)
//    - All coordinates in OpenGL space (-1 to +1)
//    - X = horizontal (left/right)
//    - Y = height (bottom/top) → affects shadow length
//    - Z = depth (near/far into screen)
//
// INTERPLAY FOR LIGHTING:
//    - Shader samples height map → gets Z-depth for each background pixel
//    - Shader samples normal map → gets surface normal for each background pixel
//    - Light 3D position compared against pixel position + Z-depth + surface normal
//    - Result: accurate lighting and shadows based on height map depth and normals
//
// =============================================================================
// SHADOW SYSTEM (Hybrid: Screen-Space + Projected Texture)
// =============================================================================
//
// RESOLUTION: 320x180 = 57,600 Pixel (klein genug für teure Verfahren!)
//
// IMPORTANT: This is a 2.5D system, NOT true 3D!
//    - Background = FLAT 2D image (no geometry!)
//    - Depth Map = gives each pixel a "virtual" Z-depth
//    - Normal Map = gives each pixel a "virtual" surface orientation
//    - Only PointLights move in TRUE 3D space
//    - Everything else is 2D with "virtual" depth from textures!
//
// =============================================================================
// WHY HYBRID? (Das fundamentale Problem mit Raymarching in 2.5D)
// =============================================================================
//
//    Was wir haben:
//       Depth Map = 2D-Bild mit Z-Werten pro Pixel (aus Kamera-Sicht)
//       Licht = echte 3D-Position (X, Y, Z)
//
//    Was fehlt für echtes Raymarching:
//       - Die Depth Map zeigt nur "von der Kamera aus gesehen"
//       - Sie enthält NICHT, was zwischen Licht und Pixel liegt
//       - Player/Props sind NICHT in der Depth Map!
//
//    SEITENANSICHT (was Depth Map NICHT zeigt):
//
//           ☀️ Light (Y=0.5, Z=0.2)
//            \
//             \  ← Ray müsste ÜBER die Szene gehen
//              \   Aber Depth Map = nur Kamera-Sicht!
//               \
//        ┌───┐   \
//        │   │ Player    │ WALL
//        └───┘           │
//    ════════════════════╧═══ FLOOR
//
//    → Raymarching funktioniert NUR für Background-zu-Background Schatten!
//    → Für Player/Prop Schatten brauchen wir Silhouette-Texturen!
//
// =============================================================================
// SHADOW CASTING MATRIX:
// =============================================================================
//
//    SOURCE          TARGET          METHOD                  NOTES
//    ──────────────────────────────────────────────────────────────────────────
//    Background   →  Background      Screen-Space Sampling   Wand→Boden
//    Player/Prop  →  Floor           Projected Silhouette    Sprite→Boden
//    Player/Prop  →  Wall            Projected + UV Stretch  Sprite→Wand
//
// =============================================================================
// SYSTEM 1: BACKGROUND SELF-SHADOW (Screen-Space Sampling)
// =============================================================================
//
// Bei 320x180 können wir uns echtes Per-Pixel Sampling leisten!
//
// ALGORITHMUS:
//    Für jeden Pixel auf dem BODEN (normal.g < 0.7):
//    1. March in Screen-Space von Pixel → Light-Position
//    2. Bei jedem Step: Sample Depth Map
//    3. Wenn Z_sample < Z_interpoliert → Occluder gefunden → SCHATTEN
//
// WARUM DAS FUNKTIONIERT:
//    - Wir marchen entlang der Bildschirm-XY-Achse
//    - Die Depth Map gibt uns die Z-Tiefe für Background-Geometrie
//    - Wenn eine Wand "im Weg" steht (näher an Kamera), blockt sie das Licht
//
// BEISPIEL: Wand wirft Schatten auf Boden
//
//    Screen-Space View:
//    ┌─────────────────────────────┐
//    │   ☀️ Light                   │
//    │    \                        │
//    │     \  Ray                  │
//    │      \                      │
//    │  WALL ▓ ← Z=-0.5 (nah)      │
//    │       ▒                     │
//    │       ░ ← Shadow            │
//    │  FLOOR  ← Z=+0.5 (weit)     │
//    └─────────────────────────────┘
//
//    March-Logik:
//    - Start: Floor-Pixel, Z=+0.5
//    - Step 1: Sample bei (-10px, -5px), Depth Map gibt Z=-0.5
//    - Check: -0.5 < interpolierte_Z → JA, Occluder!
//    - Ergebnis: Dieses Floor-Pixel ist im Schatten
//
// PSEUDO-CODE:
//    ```glsl
//    float screenSpaceShadow(vec2 pixelUV, vec3 lightPosScreen) {
//        float pixelZ = texture(depthMap, pixelUV).r;
//        vec2 rayDir = normalize(lightPosScreen.xy - pixelUV);
//        float rayLen = length(lightPosScreen.xy - pixelUV);
//        
//        const int STEPS = 16;  // Bei 320x180 bezahlbar!
//        for (int i = 1; i < STEPS; i++) {
//            float t = float(i) / float(STEPS);
//            vec2 sampleUV = pixelUV + rayDir * rayLen * t;
//            
//            float sampleZ = texture(depthMap, sampleUV).r;
//            float expectedZ = mix(pixelZ, lightPosScreen.z, t);
//            
//            // Occluder gefunden? (Sample ist NÄHER als erwartet)
//            if (sampleZ < expectedZ - 0.05) {
//                return 0.5;  // Im Schatten (50% dunkel)
//            }
//        }
//        return 1.0;  // Kein Schatten
//    }
//    ```
//
// PERFORMANCE BEI 320x180:
//    16 Samples × 57,600 Pixel = 921,600 Samples/Frame
//    Bei 60 FPS = 55M Samples/Sekunde → trivial für GPU!
//
// =============================================================================
// SYSTEM 2: SPRITE SHADOWS (Projected Silhouette Texture)
// =============================================================================
//
// Für Player/Props brauchen wir Silhouette-Texturen, weil:
//    - Sie sind NICHT in der Depth Map
//    - Ihre Form ändert sich (Animation)
//    - Sie müssen pixel-perfekt sein
//
// PASS 1: RENDER SILHOUETTE
//    - Bind offscreen FBO (kann kleiner sein, z.B. 160x90)
//    - Clear zu transparent
//    - Render Player/Prop als solid alpha (keine Farbe, nur Form)
//    - Ergebnis: Silhouette-Textur mit Alpha-Maske
//
//    ```
//    Sprite:          Silhouette:
//    ┌─────────┐      ┌─────────┐
//    │  ╔═══╗  │      │  ▓▓▓▓▓  │
//    │  ║   ║  │  →   │  ▓▓▓▓▓  │
//    │  ║ 🧍║  │      │  ▓▓▓▓▓  │
//    │  ╚═══╝  │      │  ▓▓▓▓▓  │
//    └─────────┘      └─────────┘
//    ```
//
// PASS 2: PROJECT & SAMPLE
//    Für jeden Hintergrund-Pixel:
//    1. Berechne Shadow-UV (projiziert von Light durch Caster)
//    2. Falls auf Wand → UV strecken
//    3. Sample Silhouette-Textur
//    4. Wenn Alpha > 0 → im Schatten
//
// =============================================================================
// WALL SHADOW PROJECTION (UV Stretch)
// =============================================================================
//
// Wände sind durch die NORMAL MAP definiert:
//    - Normal.G > 0.7 = vertikale Fläche (Wand facing camera)
//    - Normal.G < 0.3 = horizontale Fläche (Boden)
//
// PROBLEM: Schatten muss auf der Wand "hochklettern"
//
//    Real-World:
//        💡────────────┐
//         \            │ WALL
//          \   ┌───┐   │
//           \  │   │   │
//    ════════\═╧═══╧═══╧═══  FLOOR
//             \___/ Shadow
//
//    Was wir sehen (Screen-Space):
//    ┌─────────────────────────────┐
//    │  WALL ▓▓▓▓▓▓▓               │
//    │       ▓▓▓▓▓▓▓ ← Shadow      │
//    │       ▓▓▓▓▓▓▓   auf Wand    │
//    │───────╫╫╫╫╫╫╫───────────────│
//    │       ╚══════╗              │
//    │    🧍────────╝ Shadow       │
//    │   FLOOR        auf Floor    │
//    └─────────────────────────────┘
//
// LÖSUNG: UV-Stretch basierend auf Wall-Normal
//
//    ```glsl
//    vec2 getShadowUV(vec2 pixelPos, vec3 casterPos, vec3 lightPos) {
//        // Basis-Projektion
//        vec2 lightDir = normalize(pixelPos - lightPos.xy);
//        vec2 shadowUV = /* ... projection math ... */;
//        
//        // Wall-Check via Normal Map
//        vec3 normal = texture(normalMap, pixelPos).rgb * 2.0 - 1.0;
//        bool isWall = (normal.g > 0.7);
//        
//        if (isWall) {
//            // Schatten "klettert hoch" auf der Wand
//            // Je höher der Pixel auf der Wand, desto mehr Stretch
//            float wallHeight = /* pixel Y relative to wall base */;
//            float lightHeight = lightPos.y;  // Licht-Höhe über Szene
//            
//            // UV in Y-Richtung strecken
//            float stretchFactor = 1.0 + wallHeight / max(lightHeight, 0.1);
//            shadowUV.y *= stretchFactor;
//        }
//        
//        return shadowUV;
//    }
//    ```
//
// =============================================================================
// VISUAL: COMPLETE SHADOW PIPELINE
// =============================================================================
//
//    ┌──────────────────────────────────────────────────────────────────┐
//    │  FRAME RENDERING                                                  │
//    ├──────────────────────────────────────────────────────────────────┤
//    │                                                                   │
//    │  PASS 1: SPRITE SILHOUETTES                                      │
//    │  ┌─────────┐     ┌─────────┐                                     │
//    │  │ Player  │ →   │ ▓▓▓▓▓▓▓ │  Offscreen FBO                      │
//    │  │ Sprite  │     │ ▓▓▓▓▓▓▓ │  (Alpha only)                       │
//    │  └─────────┘     └─────────┘                                     │
//    │                                                                   │
//    │  PASS 2: BACKGROUND + SHADOWS                                    │
//    │  ┌─────────────────────────────────────────────────────┐        │
//    │  │ For each pixel:                                      │        │
//    │  │   1. Sample diffuse, normal, depth                   │        │
//    │  │   2. Background self-shadow (screen-space march)     │        │
//    │  │   3. Sprite shadow (sample silhouette texture)       │        │
//    │  │   4. Apply lighting with shadow factor               │        │
//    │  └─────────────────────────────────────────────────────┘        │
//    │                                                                   │
//    │  PASS 3: SPRITES (with lighting, no self-shadow)                 │
//    │                                                                   │
//    │  PASS 4: UI (no shadows)                                         │
//    │                                                                   │
//    └──────────────────────────────────────────────────────────────────┘
//
// =============================================================================
// SOFT SHADOWS (Optional Enhancement)
// =============================================================================
//
// Bei 320x180 können wir uns auch Soft Shadows leisten:
//
// OPTION A: PCF (Percentage Closer Filtering)
//    - Sample Silhouette-Textur mehrfach mit Offset
//    - Average = weicher Rand
//    ```glsl
//    float shadow = 0.0;
//    for (int i = 0; i < 4; i++) {
//        vec2 offset = poissonDisk[i] * softness;
//        shadow += texture(silhouette, shadowUV + offset).a;
//    }
//    shadow /= 4.0;
//    ```
//
// OPTION B: Multi-Sample Screen-Space
//    - Für Background-Shadows: Sample entlang mehrerer leicht versetzter Rays
//    - Ergebnis: weiche Kanten bei Wand-Schatten
//
// =============================================================================
// PERFORMANCE BUDGET (320x180 @ 60 FPS)
// =============================================================================
//
//    Operation                          Samples/Frame     Cost
//    ──────────────────────────────────────────────────────────────────
//    Screen-Space Shadow (16 steps)     921,600           Low
//    Silhouette Render (1 draw/caster)  ~1,000 tris       Trivial
//    Shadow UV + Sample (per pixel)     57,600            Low
//    PCF Soft Shadows (4 samples)       230,400           Low
//    ──────────────────────────────────────────────────────────────────
//    TOTAL                              ~1.2M ops         Very feasible!
//
// =============================================================================

// Prop: 2.5D object - Z-depth derived from HEIGHT MAP at X,Y position
struct Prop {
    Vec3 position;      // X,Y = screen position; Z = depth (auto-calculated from height map)
    Vec2 size;
    Renderer::TextureID texture;
    Renderer::TextureID normal_map;  // Normal map for lighting (optional)
    std::string name;
    PivotPoint pivot = PivotPoint::TOP_LEFT;
};

struct Hotspot {
    std::string name;
    Collision::Polygon bounds;
    float interaction_distance = 0.0f;
    bool enabled = true;
    std::function<void()> callback;
};

// PointLight: 3D positioning in OpenGL coordinate space
struct PointLight {
    // COORDINATE SYSTEM (ALL IN OPENGL SPACE -1 to +1):
    //   X = horizontal (-1=left, +1=right) on screen
    //   Y = height above scene (-1=below, +1=above) - NOT screen Y!
    //   Z = depth into screen (-1=near camera, +1=far/background)
    //
    // IMPORTANT: Light Y is HEIGHT, not screen position!
    //   Y = 0.5  → light is above the scene (like ceiling light)
    //   Y = 0.0  → light is at scene level (like torch on floor)
    //   Y = -0.5 → light is below scene (underground glow)
    //
    // SHADOW PROJECTION:
    //   - Shadow direction: from light through caster onto scene
    //   - Shadow length: inversely proportional to light Y height
    //     → Y=1.0 (high): short shadows (sun at noon)
    //     → Y=0.1 (low): long shadows (sunset)
    //   - Wall shadows: UV stretched vertically based on wall normal
    //   - Projection formula: shadowUV = project(pixelPos, lightPos, casterBounds)
    Vec3 position;      // OpenGL coords (-1 to +1), Y = HEIGHT above scene
    Vec3 color;         // RGB color (0-1 range)
    float intensity;    // Brightness multiplier
    float radius;       // Radius in OpenGL units (0-4 range typical)
};

struct SceneGeometry {
    std::vector<Collision::Polygon> walkable_areas;
    std::vector<Hotspot> hotspots;
};

struct Scene {
    std::string name;
    uint32_t width;
    uint32_t height;
    Renderer::TextureID background;
    Renderer::TextureID background_normal_map;  // Normal map for background lighting
    Renderer::DepthMapData depth_map;  // Depth map data for Z-depth sampling
    std::vector<Prop> props;
    
    std::vector<PointLight> lights;     // Dynamic point lights for scene illumination
    SceneGeometry geometry;
};

struct SceneManager {
    Scene current_scene;
};

// Calculate Z-DEPTH based on depth map sampling
// Coordinate convention:
//   Z = -1.0: NEAR camera (objects appear larger, in front)
//   Z = +1.0: FAR from camera (objects appear smaller, behind)
// Depth map encoding:
//   White pixels (255) = near (Z=-1), Black pixels (0) = far (Z=+1)
inline float get_z_from_depth_map(const Scene& scene, float world_x, float world_y) {
    if (scene.depth_map.is_valid()) {
        // Normalize world position to texture coordinates [0, 1]
        float tex_u = world_x / scene.width;
        float tex_v = world_y / scene.height;
        
        // Clamp to [0, 1]
        tex_u = std::max(0.0f, std::min(1.0f, tex_u));
        tex_v = std::max(0.0f, std::min(1.0f, tex_v));
        
        // Convert to pixel coordinates
        int32_t pixel_x = (int32_t)(tex_u * (scene.depth_map.width - 1));
        int32_t pixel_y = (int32_t)(tex_v * (scene.depth_map.height - 1));
        
        // Sample depth value (0-255)
        uint32_t pixel_index = pixel_y * scene.depth_map.width + pixel_x;
        uint8_t depth_value = scene.depth_map.pixels[pixel_index];
        
        // Normalize to [0, 1]
        float normalized_depth = depth_value / 255.0f;
        
        // Map to Z range: white (255) = near (-1.0), black (0) = far (+1.0)
        // Full range from -1 to +1
        float z = 1.0f - (normalized_depth * 2.0f);
        
        return z;
    } else {
        // No depth map - neutral depth
        return 0.0f;
    }
}

// Calculate depth scaling based on depth map sampling
// Matches the Z-coordinate: negative Z (near) = larger scale, positive Z (far) = smaller scale
inline float get_depth_scaling(const Scene& scene, float world_x, float world_y) {
    if (scene.depth_map.is_valid()) {
        // Get Z value from depth map
        float z_value = get_z_from_depth_map(scene, world_x, world_y);
        
        // Map Z to scale: z=-0.8 (near, large) -> scale=1.4, z=0.8 (far, small) -> scale=0.6
        // Formula: scale = 1.0 - (z_value / 0.8) * 0.4
        float scale = 1.0f - (z_value / 0.8f) * 0.4f;
        return scale;
    } else {
        // Fallback: use Y-based scaling if no depth map
        float t = world_y / scene.height;
        return 0.6f + (t * 0.8f);
    }
}

void init_scene_test();

}
