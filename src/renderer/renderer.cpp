#include "renderer.h"
#include "renderer_utils.h"
#include "shader_loader.h"
#include "texture_loader.h"
#include "debug/debug_log.h"
#include "opengl_compat.h"
#include "config.h"
#include <cmath>

namespace Renderer {

static GLuint quadVAO = 0;
static GLuint quadVBO = 0;
static GLuint shaderProgram = 0;
static GLuint colorShaderProgram = 0;
static GLuint litShaderProgram = 0;    // Shader for lit sprites with ECS lights
static GLuint upscaleShaderProgram = 0;  // Shader for upscaling FBO to viewport
static uint32_t g_viewport_width = 0;
static uint32_t g_viewport_height = 0;
static TextureID g_depth_map_texture = 0;
static uint32_t g_scene_width = 0;
static uint32_t g_scene_height = 0;

// Framebuffer Object (FBO) for offscreen rendering
static GLuint g_fbo = 0;
static GLuint g_fbo_texture = 0;
static GLuint g_fbo_depth_rbo = 0;
static uint32_t g_fbo_width = 0;
static uint32_t g_fbo_height = 0;
static bool g_rendering_to_fbo = false;  // Track if currently rendering to FBO

// Global projector lights (set once per frame, used by all lit renders)
static ProjectorLightData g_projector_lights[MAX_PROJECTOR_LIGHTS];
static uint32_t g_num_projector_lights = 0;

void init_renderer(uint32_t width, uint32_t height)
{
    g_viewport_width = width;
    g_viewport_height = height;
    glViewport(0, 0, width, height);
    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);  // Standard: render if z is LESS than (closer to camera than) existing z
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float vertices[] = {
        -0.5f, -0.5f,     0.0f, 1.0f,
         0.5f, -0.5f,     1.0f, 1.0f,
        -0.5f,  0.5f,     0.0f, 0.0f,
         0.5f,  0.5f,     1.0f, 0.0f
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    std::string vertexSrc = load_shader_source("basic.vert");
    std::string fragSrc = load_shader_source("basic.frag");

    shaderProgram = compile_and_link_shader(vertexSrc.c_str(), fragSrc.c_str());
    
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture0"), 0);
    glUseProgram(0);
    
    std::string colorVertSrc = load_shader_source("color.vert");
    std::string colorFragSrc = load_shader_source("color.frag");
    colorShaderProgram = compile_and_link_shader(colorVertSrc.c_str(), colorFragSrc.c_str());
    
    // Load lit shader for ECS-based lighting
    std::string litVertSrc = load_shader_source("basic_lit.vert");
    std::string litFragSrc = load_shader_source("basic_lit.frag");
    litShaderProgram = compile_and_link_shader(litVertSrc.c_str(), litFragSrc.c_str());
    
    if (litShaderProgram == 0) {
        DEBUG_ERROR("Failed to compile basic_lit shader program!");
    } else {
        DEBUG_INFO("Successfully loaded basic_lit shader program (ID: %u)", litShaderProgram);
        glUseProgram(litShaderProgram);
        glUniform1i(glGetUniformLocation(litShaderProgram, "texture0"), 0);    // Diffuse
        glUniform1i(glGetUniformLocation(litShaderProgram, "normalMap"), 1);   // Normal map
        glUniform1i(glGetUniformLocation(litShaderProgram, "depthMap"), 2);    // Depth map
        glUseProgram(0);
    }
}

void set_depth_map_data(TextureID depthMapTexture, uint32_t sceneWidth, uint32_t sceneHeight)
{
    g_depth_map_texture = depthMapTexture;
    g_scene_width = sceneWidth;
    g_scene_height = sceneHeight;
}

void clear_screen()
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void render_sprite(TextureID tex, Vec3 pos, Vec2 size, PivotPoint pivot)
{
    // Default texture coordinate range (full texture)
    render_sprite(tex, pos, size, Vec4(0.0f, 0.0f, 1.0f, 1.0f), pivot);
}

void render_sprite(TextureID tex, Vec3 pos, Vec2 size, Vec4 tex_coord_range, PivotPoint pivot)
{
    // Get current render target dimensions (FBO or viewport)
    uint32_t render_width = get_render_width();
    uint32_t render_height = get_render_height();
    
    // Convert pixel coordinates to OpenGL coordinates
    Vec2 opengl_pos = Coords::pixel_to_opengl(Vec2(pos.x, pos.y), render_width, render_height);
    Vec2 opengl_size = Vec2(
        (size.x / (float)render_width) * 2.0f,
        (size.y / (float)render_height) * 2.0f
    );
    
    // Calculate offset from pivot point to center
    Vec2 pivot_offset = Coords::get_pivot_offset(pivot, size.x, size.y);
    Vec2 pivot_offset_opengl = Vec2(
        (pivot_offset.x / (float)render_width) * 2.0f,
        -(pivot_offset.y / (float)render_height) * 2.0f  // Negate because Y is inverted
    );
    
    // Shader expects center point
    Vec2 opengl_center = Vec2(
        opengl_pos.x + pivot_offset_opengl.x,
        opengl_pos.y + pivot_offset_opengl.y
    );
    
    glUseProgram(shaderProgram);
    
    GLint posLoc = glGetUniformLocation(shaderProgram, "spritePos");
    GLint sizeLoc = glGetUniformLocation(shaderProgram, "spriteSize");
    GLint zLoc = glGetUniformLocation(shaderProgram, "spriteZ");
    GLint texCoordLoc = glGetUniformLocation(shaderProgram, "texCoordRange");
    
    glUniform2f(posLoc, opengl_center.x, opengl_center.y);
    glUniform2f(sizeLoc, opengl_size.x, opengl_size.y);
    glUniform1f(zLoc, pos.z);  // Use z component from Vec3
    glUniform4f(texCoordLoc, tex_coord_range.x, tex_coord_range.y, tex_coord_range.z, tex_coord_range.w);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void render_sprite_animated(const SpriteAnimation* anim, Vec3 pos, Vec2 size, PivotPoint pivot)
{
    if (!anim || anim->frames.empty()) {
        DEBUG_ERROR("render_sprite_animated() - invalid animation (null or empty frames)");
        return;
    }
    if (anim->current_frame >= anim->frames.size()) {
        DEBUG_ERROR("render_sprite_animated() - current_frame (%u) out of bounds (size: %zu)",
                   anim->current_frame, anim->frames.size());
        return;
    }
    
    // Get UV coordinates for current frame
    const SpriteFrame& frame = anim->frames[anim->current_frame];
    Vec4 tex_coord_range(frame.u0, frame.v0, frame.u1, frame.v1);
    
    // Render using sprite map texture with UV mapping
    render_sprite(anim->texture, pos, size, tex_coord_range, pivot);
}

void render_rect(Vec3 pos, Vec2 size, Vec4 color, PivotPoint pivot)
{
    // Get current render target dimensions (FBO or viewport)
    uint32_t render_width = get_render_width();
    uint32_t render_height = get_render_height();
    
    // Convert pixel coordinates to OpenGL coordinates
    Vec2 opengl_pos = Coords::pixel_to_opengl(Vec2(pos.x, pos.y), render_width, render_height);
    Vec2 opengl_size = Vec2(
        (size.x / (float)render_width) * 2.0f,
        (size.y / (float)render_height) * 2.0f
    );
    
    // Calculate offset from pivot point to center
    Vec2 pivot_offset = Coords::get_pivot_offset(pivot, size.x, size.y);
    Vec2 pivot_offset_opengl = Vec2(
        (pivot_offset.x / (float)render_width) * 2.0f,
        -(pivot_offset.y / (float)render_height) * 2.0f  // Negate because Y is inverted
    );
    
    // Shader expects center point
    Vec2 opengl_center = Vec2(
        opengl_pos.x + pivot_offset_opengl.x,
        opengl_pos.y + pivot_offset_opengl.y
    );
    
    glUseProgram(colorShaderProgram);
    
    GLint posLoc = glGetUniformLocation(colorShaderProgram, "spritePos");
    GLint sizeLoc = glGetUniformLocation(colorShaderProgram, "spriteSize");
    GLint zLoc = glGetUniformLocation(colorShaderProgram, "spriteZ");
    GLint colorLoc = glGetUniformLocation(colorShaderProgram, "rectColor");
    
    glUniform2f(posLoc, opengl_center.x, opengl_center.y);
    glUniform2f(sizeLoc, opengl_size.x, opengl_size.y);
    glUniform1f(zLoc, pos.z);  // Use z component from Vec3
    glUniform4f(colorLoc, color.x, color.y, color.z, color.w);
    
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void render_line(Vec3 start, Vec3 end, Vec4 color, float thickness)
{
    // Draw line as a series of small quads along the path
    Vec2 diff = Vec2(end.x - start.x, end.y - start.y);
    float length = std::sqrt(diff.x * diff.x + diff.y * diff.y);
    
    if (length < 0.01f) return;
    
    int segment_count = (int)(length / (thickness * 2.0f)) + 1;
    Vec2 step = Vec2(diff.x / segment_count, diff.y / segment_count);
    
    // Use the average z-depth for the line
    float z_avg = (start.z + end.z) * 0.5f;
    
    for (int i = 0; i < segment_count; i++) {
        Vec3 pos = Vec3(start.x + step.x * i, start.y + step.y * i, z_avg);
        render_rect(pos, Vec2(thickness * 2.0f, thickness * 2.0f), color);
    }
}

// =============================================================================
// PROJECTOR LIGHTS (Global state for window/cookie lights)
// =============================================================================

void set_projector_lights(const ProjectorLightData* lights, uint32_t count)
{
    g_num_projector_lights = count > MAX_PROJECTOR_LIGHTS ? MAX_PROJECTOR_LIGHTS : count;
    for (uint32_t i = 0; i < g_num_projector_lights; i++) {
        g_projector_lights[i] = lights[i];
    }
}

void clear_projector_lights()
{
    g_num_projector_lights = 0;
}

// Upload projector light uniforms to the lit shader (called internally)
static void upload_projector_light_uniforms()
{
    glUniform1i(glGetUniformLocation(litShaderProgram, "numProjectorLights"), g_num_projector_lights);
    
    if (g_num_projector_lights > 0) {
        uint32_t render_width = get_render_width();
        uint32_t render_height = get_render_height();
        
        std::vector<Vec3> positions;
        std::vector<Vec3> directions;
        std::vector<Vec3> ups;
        std::vector<Vec3> colors;
        std::vector<float> intensities;
        std::vector<float> fovs;
        std::vector<float> aspects;
        std::vector<float> ranges;
        
        for (uint32_t i = 0; i < g_num_projector_lights; i++) {
            // Convert position from pixel to OpenGL coords
            Vec2 opengl_pos = Coords::pixel_to_opengl(
                Vec2(g_projector_lights[i].position.x, g_projector_lights[i].position.y),
                render_width, render_height);
            positions.push_back(Vec3(opengl_pos.x, opengl_pos.y, g_projector_lights[i].position.z));
            
            // Direction and up are already normalized, pass through
            directions.push_back(g_projector_lights[i].direction);
            ups.push_back(g_projector_lights[i].up);
            colors.push_back(g_projector_lights[i].color);
            intensities.push_back(g_projector_lights[i].intensity);
            
            // Convert FOV from degrees to radians
            float fovRadians = g_projector_lights[i].fov * 3.14159265f / 180.0f;
            fovs.push_back(fovRadians);
            aspects.push_back(g_projector_lights[i].aspect_ratio);
            
            // Convert range from pixels to OpenGL coords (rough approximation)
            // Range is specified in depth units which map to [-1, 1] Z range
            ranges.push_back(g_projector_lights[i].range);
        }
        
        glUniform3fv(glGetUniformLocation(litShaderProgram, "projectorPositions"), g_num_projector_lights, (float*)positions.data());
        glUniform3fv(glGetUniformLocation(litShaderProgram, "projectorDirections"), g_num_projector_lights, (float*)directions.data());
        glUniform3fv(glGetUniformLocation(litShaderProgram, "projectorUps"), g_num_projector_lights, (float*)ups.data());
        glUniform3fv(glGetUniformLocation(litShaderProgram, "projectorColors"), g_num_projector_lights, (float*)colors.data());
        glUniform1fv(glGetUniformLocation(litShaderProgram, "projectorIntensities"), g_num_projector_lights, intensities.data());
        glUniform1fv(glGetUniformLocation(litShaderProgram, "projectorFovs"), g_num_projector_lights, fovs.data());
        glUniform1fv(glGetUniformLocation(litShaderProgram, "projectorAspects"), g_num_projector_lights, aspects.data());
        glUniform1fv(glGetUniformLocation(litShaderProgram, "projectorRanges"), g_num_projector_lights, ranges.data());
        
        // Bind cookie textures to texture units 11-14 (after shadow casters 3-10)
        for (uint32_t i = 0; i < g_num_projector_lights; i++) {
            glActiveTexture(GL_TEXTURE11 + i);
            glBindTexture(GL_TEXTURE_2D, g_projector_lights[i].cookie);
        }
        
        // Set sampler uniforms for projector cookie textures
        glUniform1i(glGetUniformLocation(litShaderProgram, "projectorCookie0"), 11);
        glUniform1i(glGetUniformLocation(litShaderProgram, "projectorCookie1"), 12);
        glUniform1i(glGetUniformLocation(litShaderProgram, "projectorCookie2"), 13);
        glUniform1i(glGetUniformLocation(litShaderProgram, "projectorCookie3"), 14);
    }
}

void shutdown()
{
    shutdown_framebuffer();  // Clean up FBO resources
    
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
    }
    if (colorShaderProgram) {
        glDeleteProgram(colorShaderProgram);
    }
    if (litShaderProgram) {
        glDeleteProgram(litShaderProgram);
    }
    if (quadVAO) {
        glDeleteVertexArrays(1, &quadVAO);
    }
    if (quadVBO) {
        glDeleteBuffers(1, &quadVBO);
    }
}

// 2.5D Depth Scaling Implementation

float calculate_depth_scale(float sprite_y, float horizon_y, float scale_gradient, bool inverted)
{
    float depth = inverted ? (horizon_y - sprite_y) : (sprite_y - horizon_y);
    float scale = 1.0f + depth * scale_gradient;
    
    if (scale < Config::SCALE_MIN) scale = Config::SCALE_MIN;
    if (scale > Config::SCALE_MAX) scale = Config::SCALE_MAX;
    
    return scale;
}

void render_sprite_with_depth(TextureID tex, Vec3 pos, Vec2 base_size, float sprite_y,
                              float horizon_y, float scale_gradient, bool inverted,
                              PivotPoint pivot)
{
    render_sprite_with_depth(tex, pos, base_size, Vec4(0.0f, 0.0f, 1.0f, 1.0f),
                           sprite_y, horizon_y, scale_gradient, inverted, pivot);
}

void render_sprite_with_depth(TextureID tex, Vec3 pos, Vec2 base_size, Vec4 tex_coord_range,
                              float sprite_y, float horizon_y, float scale_gradient, bool inverted,
                              PivotPoint pivot)
{
    float depth_scale = calculate_depth_scale(sprite_y, horizon_y, scale_gradient, inverted);
    
    Vec2 scaled_size = Vec2(
        base_size.x * depth_scale,
        base_size.y * depth_scale
    );
    
    render_sprite(tex, pos, scaled_size, tex_coord_range, pivot);
}

void render_sprite_animated_with_depth(const SpriteAnimation* anim, Vec3 pos, Vec2 base_size,
                                       float sprite_y, float horizon_y, float scale_gradient, bool inverted,
                                       PivotPoint pivot)
{
    if (!anim || anim->frames.empty()) {
        DEBUG_ERROR("render_sprite_animated_with_depth() - invalid animation (null or empty frames)");
        return;
    }
    if (anim->current_frame >= anim->frames.size()) {
        DEBUG_ERROR("render_sprite_animated_with_depth() - current_frame (%u) out of bounds (size: %zu)",
                   anim->current_frame, anim->frames.size());
        return;
    }
    
    const SpriteFrame& frame = anim->frames[anim->current_frame];
    Vec4 tex_coord_range(frame.u0, frame.v0, frame.u1, frame.v1);
    
    render_sprite_with_depth(anim->texture, pos, base_size, tex_coord_range,
                           sprite_y, horizon_y, scale_gradient, inverted, pivot);
}

// =============================================================================
// LIT SPRITE RENDERING (ECS-based lighting)
// =============================================================================

// Internal: Core lit sprite rendering with full parameters
static void render_sprite_lit_internal(TextureID tex, Vec3 pos, Vec2 size, Vec4 tex_coord_range,
                                       const LightData* lights, uint32_t num_lights,
                                       TextureID normal_map, PivotPoint pivot, float objectZ)
{
    // Get current render target dimensions (FBO or viewport)
    uint32_t render_width = get_render_width();
    uint32_t render_height = get_render_height();
    
    // Convert pixel coordinates to OpenGL coordinates
    Vec2 opengl_pos = Coords::pixel_to_opengl(Vec2(pos.x, pos.y), render_width, render_height);
    Vec2 opengl_size = Vec2(
        (size.x / (float)render_width) * 2.0f,
        (size.y / (float)render_height) * 2.0f
    );
    
    // Calculate offset from pivot point to center
    Vec2 pivot_offset = Coords::get_pivot_offset(pivot, size.x, size.y);
    Vec2 pivot_offset_opengl = Vec2(
        (pivot_offset.x / (float)render_width) * 2.0f,
        -(pivot_offset.y / (float)render_height) * 2.0f
    );
    
    // Shader expects center point
    Vec2 opengl_center = Vec2(
        opengl_pos.x + pivot_offset_opengl.x,
        opengl_pos.y + pivot_offset_opengl.y
    );
    
    glUseProgram(litShaderProgram);
    
    // Set sprite uniforms
    glUniform2f(glGetUniformLocation(litShaderProgram, "spritePos"), opengl_center.x, opengl_center.y);
    glUniform2f(glGetUniformLocation(litShaderProgram, "spriteSize"), opengl_size.x, opengl_size.y);
    glUniform1f(glGetUniformLocation(litShaderProgram, "spriteZ"), pos.z);
    glUniform4f(glGetUniformLocation(litShaderProgram, "texCoordRange"), 
                tex_coord_range.x, tex_coord_range.y, tex_coord_range.z, tex_coord_range.w);
    
    // Set aspect ratio for correct circular light falloff
    float aspectRatio = (float)render_width / (float)render_height;
    glUniform1f(glGetUniformLocation(litShaderProgram, "aspectRatio"), aspectRatio);
    
    // Set object Z for lighting calculations
    // objectZ = -999.0 means background (use depth map), otherwise it's a prop
    glUniform1f(glGetUniformLocation(litShaderProgram, "objectZ"), objectZ);
    
    // No self-entity exclusion and no shadow casters for basic lit rendering
    glUniform1i(glGetUniformLocation(litShaderProgram, "selfEntityIndex"), -1);
    glUniform1i(glGetUniformLocation(litShaderProgram, "numShadowCasters"), 0);
    
    // Set ambient light
    glUniform3f(glGetUniformLocation(litShaderProgram, "ambientColor"), 0.15f, 0.15f, 0.2f);
    glUniform1f(glGetUniformLocation(litShaderProgram, "ambientIntensity"), 1.0f);
    
    // Set light uniforms (max 8 lights)
    uint32_t actual_lights = num_lights > 8 ? 8 : num_lights;
    glUniform1i(glGetUniformLocation(litShaderProgram, "numLights"), actual_lights);
    
    if (actual_lights > 0 && lights) {
        std::vector<Vec3> positions;
        std::vector<Vec3> colors;
        std::vector<float> intensities;
        std::vector<float> radii;
        
        for (uint32_t i = 0; i < actual_lights; i++) {
            positions.push_back(lights[i].position);
            colors.push_back(lights[i].color);
            intensities.push_back(lights[i].intensity);
            radii.push_back(lights[i].radius);
        }
        
        glUniform3fv(glGetUniformLocation(litShaderProgram, "lightPositions"), actual_lights, (float*)positions.data());
        glUniform3fv(glGetUniformLocation(litShaderProgram, "lightColors"), actual_lights, (float*)colors.data());
        glUniform1fv(glGetUniformLocation(litShaderProgram, "lightIntensities"), actual_lights, intensities.data());
        glUniform1fv(glGetUniformLocation(litShaderProgram, "lightRadii"), actual_lights, radii.data());
    }
    
    // Upload projector light uniforms (global state)
    upload_projector_light_uniforms();
    
    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normal_map ? normal_map : tex);  // Use diffuse as fallback
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, g_depth_map_texture);
    
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void render_sprite_lit(TextureID tex, Vec3 pos, Vec2 size, const LightData* lights, uint32_t num_lights,
                       TextureID normal_map, PivotPoint pivot, float objectZ)
{
    render_sprite_lit_internal(tex, pos, size, Vec4(0.0f, 0.0f, 1.0f, 1.0f), lights, num_lights, normal_map, pivot, objectZ);
}

void render_sprite_lit(TextureID tex, Vec3 pos, Vec2 size, Vec4 tex_coord_range,
                       const LightData* lights, uint32_t num_lights,
                       TextureID normal_map, PivotPoint pivot, float objectZ)
{
    render_sprite_lit_internal(tex, pos, size, tex_coord_range, lights, num_lights, normal_map, pivot, objectZ);
}

void render_sprite_animated_lit(const SpriteAnimation* anim, Vec3 pos, Vec2 size,
                                const LightData* lights, uint32_t num_lights,
                                TextureID normal_map, PivotPoint pivot, float objectZ)
{
    if (!anim || anim->frames.empty()) {
        DEBUG_ERROR("render_sprite_animated_lit() - invalid animation (null or empty frames)");
        return;
    }
    if (anim->current_frame >= anim->frames.size()) {
        DEBUG_ERROR("render_sprite_animated_lit() - current_frame (%u) out of bounds (size: %zu)",
                   anim->current_frame, anim->frames.size());
        return;
    }
    
    const SpriteFrame& frame = anim->frames[anim->current_frame];
    Vec4 tex_coord_range(frame.u0, frame.v0, frame.u1, frame.v1);
    
    render_sprite_lit_internal(anim->texture, pos, size, tex_coord_range, lights, num_lights, normal_map, pivot, objectZ);
}

// Forward declaration for shadowed internal function
static void render_sprite_lit_shadowed_internal(TextureID tex, Vec3 pos, Vec2 size, Vec4 tex_coord_range,
                                                const LightData* lights, uint32_t num_lights,
                                                const ShadowCasterData* shadow_casters, uint32_t num_shadow_casters,
                                                TextureID normal_map, PivotPoint pivot, 
                                                float objectZ, int32_t self_entity_index);

// Animated sprite with lighting AND shadow receiving
void render_sprite_animated_lit_shadowed(const SpriteAnimation* anim, Vec3 pos, Vec2 size,
                                         const LightData* lights, uint32_t num_lights,
                                         const ShadowCasterData* shadow_casters, uint32_t num_shadow_casters,
                                         TextureID normal_map, PivotPoint pivot, 
                                         float objectZ, int32_t self_entity_index)
{
    if (!anim || anim->frames.empty()) {
        DEBUG_ERROR("render_sprite_animated_lit_shadowed() - invalid animation (null or empty frames)");
        return;
    }
    if (anim->current_frame >= anim->frames.size()) {
        DEBUG_ERROR("render_sprite_animated_lit_shadowed() - current_frame (%u) out of bounds (size: %zu)",
                   anim->current_frame, anim->frames.size());
        return;
    }
    
    const SpriteFrame& frame = anim->frames[anim->current_frame];
    Vec4 tex_coord_range(frame.u0, frame.v0, frame.u1, frame.v1);
    
    render_sprite_lit_shadowed_internal(anim->texture, pos, size, tex_coord_range, 
                                        lights, num_lights, 
                                        shadow_casters, num_shadow_casters,
                                        normal_map, pivot, objectZ, self_entity_index);
}

// =============================================================================
// LIT SPRITE RENDERING WITH SHADOWS
// =============================================================================

// Internal: Core lit sprite rendering with shadow casters
static void render_sprite_lit_shadowed_internal(TextureID tex, Vec3 pos, Vec2 size, Vec4 tex_coord_range,
                                                const LightData* lights, uint32_t num_lights,
                                                const ShadowCasterData* shadow_casters, uint32_t num_shadow_casters,
                                                TextureID normal_map, PivotPoint pivot, 
                                                float objectZ, int32_t self_entity_index)
{
    // Get current render target dimensions (FBO or viewport)
    uint32_t render_width = get_render_width();
    uint32_t render_height = get_render_height();
    
    // Convert pixel coordinates to OpenGL coordinates
    Vec2 opengl_pos = Coords::pixel_to_opengl(Vec2(pos.x, pos.y), render_width, render_height);
    Vec2 opengl_size = Vec2(
        (size.x / (float)render_width) * 2.0f,
        (size.y / (float)render_height) * 2.0f
    );
    
    // Calculate offset from pivot point to center
    Vec2 pivot_offset = Coords::get_pivot_offset(pivot, size.x, size.y);
    Vec2 pivot_offset_opengl = Vec2(
        (pivot_offset.x / (float)render_width) * 2.0f,
        -(pivot_offset.y / (float)render_height) * 2.0f
    );
    
    // Shader expects center point
    Vec2 opengl_center = Vec2(
        opengl_pos.x + pivot_offset_opengl.x,
        opengl_pos.y + pivot_offset_opengl.y
    );
    
    glUseProgram(litShaderProgram);
    
    // Set sprite uniforms
    glUniform2f(glGetUniformLocation(litShaderProgram, "spritePos"), opengl_center.x, opengl_center.y);
    glUniform2f(glGetUniformLocation(litShaderProgram, "spriteSize"), opengl_size.x, opengl_size.y);
    glUniform1f(glGetUniformLocation(litShaderProgram, "spriteZ"), pos.z);
    glUniform4f(glGetUniformLocation(litShaderProgram, "texCoordRange"), 
                tex_coord_range.x, tex_coord_range.y, tex_coord_range.z, tex_coord_range.w);
    
    // Set aspect ratio for correct circular light falloff
    float aspectRatio = (float)render_width / (float)render_height;
    glUniform1f(glGetUniformLocation(litShaderProgram, "aspectRatio"), aspectRatio);
    
    // Set object Z for lighting calculations
    glUniform1f(glGetUniformLocation(litShaderProgram, "objectZ"), objectZ);
    
    // Set self entity index for shadow exclusion
    glUniform1i(glGetUniformLocation(litShaderProgram, "selfEntityIndex"), self_entity_index);
    
    // Set ambient light
    glUniform3f(glGetUniformLocation(litShaderProgram, "ambientColor"), 0.15f, 0.15f, 0.2f);
    glUniform1f(glGetUniformLocation(litShaderProgram, "ambientIntensity"), 1.0f);
    
    // Set light uniforms (max 8 lights)
    uint32_t actual_lights = num_lights > 8 ? 8 : num_lights;
    glUniform1i(glGetUniformLocation(litShaderProgram, "numLights"), actual_lights);
    
    if (actual_lights > 0 && lights) {
        std::vector<Vec3> positions;
        std::vector<Vec3> colors;
        std::vector<float> intensities;
        std::vector<float> radii;
        
        for (uint32_t i = 0; i < actual_lights; i++) {
            positions.push_back(lights[i].position);
            colors.push_back(lights[i].color);
            intensities.push_back(lights[i].intensity);
            radii.push_back(lights[i].radius);
        }
        
        glUniform3fv(glGetUniformLocation(litShaderProgram, "lightPositions"), actual_lights, (float*)positions.data());
        glUniform3fv(glGetUniformLocation(litShaderProgram, "lightColors"), actual_lights, (float*)colors.data());
        glUniform1fv(glGetUniformLocation(litShaderProgram, "lightIntensities"), actual_lights, intensities.data());
        glUniform1fv(glGetUniformLocation(litShaderProgram, "lightRadii"), actual_lights, radii.data());
    }
    
    // Set shadow caster uniforms (max MAX_SHADOW_CASTERS)
    uint32_t actual_casters = num_shadow_casters > MAX_SHADOW_CASTERS ? MAX_SHADOW_CASTERS : num_shadow_casters;
    glUniform1i(glGetUniformLocation(litShaderProgram, "numShadowCasters"), actual_casters);
    
    if (actual_casters > 0 && shadow_casters) {
        std::vector<Vec3> casterPositions;
        std::vector<Vec2> casterSizes;
        std::vector<float> casterZDepths;
        std::vector<Vec4> casterUVRanges;
        std::vector<float> casterAlphaThresholds;
        std::vector<float> casterIntensities;
        std::vector<int> casterEntityIndices;
        
        for (uint32_t i = 0; i < actual_casters; i++) {
            casterPositions.push_back(shadow_casters[i].position);
            casterSizes.push_back(shadow_casters[i].size);
            casterZDepths.push_back(shadow_casters[i].position.z);  // Z from position
            casterUVRanges.push_back(shadow_casters[i].uv_range);
            casterAlphaThresholds.push_back(shadow_casters[i].alpha_threshold);
            casterIntensities.push_back(shadow_casters[i].shadow_intensity);
            casterEntityIndices.push_back(shadow_casters[i].entity_index);
        }
        
        glUniform3fv(glGetUniformLocation(litShaderProgram, "shadowCasterPositions"), actual_casters, (float*)casterPositions.data());
        glUniform2fv(glGetUniformLocation(litShaderProgram, "shadowCasterSizes"), actual_casters, (float*)casterSizes.data());
        glUniform1fv(glGetUniformLocation(litShaderProgram, "shadowCasterZDepths"), actual_casters, casterZDepths.data());
        glUniform4fv(glGetUniformLocation(litShaderProgram, "shadowCasterUVRanges"), actual_casters, (float*)casterUVRanges.data());
        glUniform1fv(glGetUniformLocation(litShaderProgram, "shadowCasterAlphaThresholds"), actual_casters, casterAlphaThresholds.data());
        glUniform1fv(glGetUniformLocation(litShaderProgram, "shadowCasterIntensities"), actual_casters, casterIntensities.data());
        glUniform1iv(glGetUniformLocation(litShaderProgram, "shadowCasterEntityIndices"), actual_casters, casterEntityIndices.data());
        
        // Bind shadow caster textures to texture units 3-10
        for (uint32_t i = 0; i < actual_casters; i++) {
            glActiveTexture(GL_TEXTURE3 + i);
            glBindTexture(GL_TEXTURE_2D, shadow_casters[i].texture);
        }
        
        // Set sampler uniforms for shadow caster textures
        glUniform1i(glGetUniformLocation(litShaderProgram, "shadowCasterTex0"), 3);
        glUniform1i(glGetUniformLocation(litShaderProgram, "shadowCasterTex1"), 4);
        glUniform1i(glGetUniformLocation(litShaderProgram, "shadowCasterTex2"), 5);
        glUniform1i(glGetUniformLocation(litShaderProgram, "shadowCasterTex3"), 6);
        glUniform1i(glGetUniformLocation(litShaderProgram, "shadowCasterTex4"), 7);
        glUniform1i(glGetUniformLocation(litShaderProgram, "shadowCasterTex5"), 8);
        glUniform1i(glGetUniformLocation(litShaderProgram, "shadowCasterTex6"), 9);
        glUniform1i(glGetUniformLocation(litShaderProgram, "shadowCasterTex7"), 10);
    } else {
        // No shadow casters - set count to 0
        glUniform1i(glGetUniformLocation(litShaderProgram, "numShadowCasters"), 0);
    }
    
    // Upload projector light uniforms (global state)
    upload_projector_light_uniforms();
    
    // Bind textures (units 0-2 for main sprite)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normal_map ? normal_map : tex);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, g_depth_map_texture);
    
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void render_sprite_lit_shadowed(TextureID tex, Vec3 pos, Vec2 size, 
                                const LightData* lights, uint32_t num_lights,
                                const ShadowCasterData* shadow_casters, uint32_t num_shadow_casters,
                                TextureID normal_map, PivotPoint pivot, 
                                float objectZ, int32_t self_entity_index)
{
    render_sprite_lit_shadowed_internal(tex, pos, size, Vec4(0.0f, 0.0f, 1.0f, 1.0f), 
                                        lights, num_lights, shadow_casters, num_shadow_casters, 
                                        normal_map, pivot, objectZ, self_entity_index);
}

void render_sprite_lit_shadowed(TextureID tex, Vec3 pos, Vec2 size, Vec4 tex_coord_range,
                                const LightData* lights, uint32_t num_lights,
                                const ShadowCasterData* shadow_casters, uint32_t num_shadow_casters,
                                TextureID normal_map, PivotPoint pivot, 
                                float objectZ, int32_t self_entity_index)
{
    render_sprite_lit_shadowed_internal(tex, pos, size, tex_coord_range, 
                                        lights, num_lights, shadow_casters, num_shadow_casters, 
                                        normal_map, pivot, objectZ, self_entity_index);
}

// =============================================================================
// FRAMEBUFFER OBJECT (FBO) IMPLEMENTATION
// =============================================================================
// Renders scene at base resolution (320x180) then upscales to viewport (1280x720)
// Benefits:
//   - 3.2x fewer fragment shader invocations (better shadow performance)
//   - Pixel-perfect retro look with GL_NEAREST upscaling
//   - Consistent rendering regardless of viewport size

void init_framebuffer(uint32_t base_width, uint32_t base_height)
{
    g_fbo_width = base_width;
    g_fbo_height = base_height;
    
    // Create framebuffer
    glGenFramebuffers(1, &g_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
    
    // Create color texture attachment
    glGenTextures(1, &g_fbo_texture);
    glBindTexture(GL_TEXTURE_2D, g_fbo_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, base_width, base_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    
    // GL_NEAREST for pixel-perfect upscaling (no blurring)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_fbo_texture, 0);
    
    // Create depth/stencil renderbuffer
    glGenRenderbuffers(1, &g_fbo_depth_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, g_fbo_depth_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, base_width, base_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, g_fbo_depth_rbo);
    
    // Check framebuffer completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        DEBUG_ERROR("Framebuffer not complete! Status: 0x%X", status);
    } else {
        DEBUG_INFO("Framebuffer created: %ux%u (base resolution)", base_width, base_height);
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Load upscale shader
    std::string upscaleVertSrc = load_shader_source("upscale.vert");
    std::string upscaleFragSrc = load_shader_source("upscale.frag");
    upscaleShaderProgram = compile_and_link_shader(upscaleVertSrc.c_str(), upscaleFragSrc.c_str());
    
    if (upscaleShaderProgram == 0) {
        DEBUG_ERROR("Failed to compile upscale shader program!");
    } else {
        DEBUG_INFO("Successfully loaded upscale shader program (ID: %u)", upscaleShaderProgram);
        glUseProgram(upscaleShaderProgram);
        glUniform1i(glGetUniformLocation(upscaleShaderProgram, "screenTexture"), 0);
        glUseProgram(0);
    }
}

void begin_render_to_framebuffer()
{
    g_rendering_to_fbo = true;
    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
    glViewport(0, 0, g_fbo_width, g_fbo_height);
}

void end_render_to_framebuffer()
{
    g_rendering_to_fbo = false;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, g_viewport_width, g_viewport_height);
}

void render_framebuffer_to_screen()
{
    // Disable depth test for fullscreen quad
    glDisable(GL_DEPTH_TEST);
    
    glUseProgram(upscaleShaderProgram);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_fbo_texture);
    
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    
    // Re-enable depth test
    glEnable(GL_DEPTH_TEST);
}

void shutdown_framebuffer()
{
    if (g_fbo_texture) {
        glDeleteTextures(1, &g_fbo_texture);
        g_fbo_texture = 0;
    }
    if (g_fbo_depth_rbo) {
        glDeleteRenderbuffers(1, &g_fbo_depth_rbo);
        g_fbo_depth_rbo = 0;
    }
    if (g_fbo) {
        glDeleteFramebuffers(1, &g_fbo);
        g_fbo = 0;
    }
    if (upscaleShaderProgram) {
        glDeleteProgram(upscaleShaderProgram);
        upscaleShaderProgram = 0;
    }
}

uint32_t get_render_width()
{
    return g_rendering_to_fbo ? g_fbo_width : g_viewport_width;
}

uint32_t get_render_height()
{
    return g_rendering_to_fbo ? g_fbo_height : g_viewport_height;
}

}
