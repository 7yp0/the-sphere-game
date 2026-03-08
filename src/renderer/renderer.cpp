#include "renderer.h"
#include "renderer_utils.h"
#include "shader_loader.h"
#include "texture_loader.h"
#include "debug/debug_log.h"
#include "opengl_compat.h"
#include "config.h"
#include "../scene/scene.h"
#include <cmath>

namespace Renderer {

static GLuint quadVAO = 0;
static GLuint quadVBO = 0;
static GLuint shaderProgram = 0;
static GLuint colorShaderProgram = 0;
static GLuint litShaderProgram = 0;  // Shader program for point-lit rendering
static uint32_t g_viewport_width = 0;
static uint32_t g_viewport_height = 0;
static TextureID g_height_map_texture = 0;
static uint32_t g_scene_width = 0;
static uint32_t g_scene_height = 0;

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
    
    // Load point-lit shader for dynamic lighting
    std::string litVertSrc = load_shader_source("basic_lit.vert");
    std::string litFragSrc = load_shader_source("basic_lit.frag");
    litShaderProgram = compile_and_link_shader(litVertSrc.c_str(), litFragSrc.c_str());
    
    if (litShaderProgram == 0) {
        DEBUG_ERROR("Failed to compile basic_lit shader program!");
    } else {
        DEBUG_INFO("Successfully loaded basic_lit shader program (ID: %u)", litShaderProgram);
    }
    
    glUseProgram(litShaderProgram);
    glUniform1i(glGetUniformLocation(litShaderProgram, "texture0"), 0);
    glUniform1i(glGetUniformLocation(litShaderProgram, "normalMap"), 1);  // Texture unit 1 for normal maps
    glUniform1i(glGetUniformLocation(litShaderProgram, "heightMap"), 2);  // Texture unit 2 for height maps
    glUseProgram(0);
}

void set_height_map_data(TextureID heightMapTexture, uint32_t sceneWidth, uint32_t sceneHeight)
{
    g_height_map_texture = heightMapTexture;
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
    // Convert pixel coordinates to OpenGL coordinates
    Vec2 opengl_pos = Coords::pixel_to_opengl(Vec2(pos.x, pos.y), g_viewport_width, g_viewport_height);
    Vec2 opengl_size = Vec2(
        (size.x / (float)g_viewport_width) * 2.0f,
        (size.y / (float)g_viewport_height) * 2.0f
    );
    
    // Calculate offset from pivot point to center
    Vec2 pivot_offset = Coords::get_pivot_offset(pivot, size.x, size.y);
    Vec2 pivot_offset_opengl = Vec2(
        (pivot_offset.x / (float)g_viewport_width) * 2.0f,
        -(pivot_offset.y / (float)g_viewport_height) * 2.0f  // Negate because Y is inverted
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
    // Convert pixel coordinates to OpenGL coordinates
    Vec2 opengl_pos = Coords::pixel_to_opengl(Vec2(pos.x, pos.y), g_viewport_width, g_viewport_height);
    Vec2 opengl_size = Vec2(
        (size.x / (float)g_viewport_width) * 2.0f,
        (size.y / (float)g_viewport_height) * 2.0f
    );
    
    // Calculate offset from pivot point to center
    Vec2 pivot_offset = Coords::get_pivot_offset(pivot, size.x, size.y);
    Vec2 pivot_offset_opengl = Vec2(
        (pivot_offset.x / (float)g_viewport_width) * 2.0f,
        -(pivot_offset.y / (float)g_viewport_height) * 2.0f  // Negate because Y is inverted
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

void shutdown()
{
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

// Point Light Rendering Implementation

void render_sprite_lit(TextureID tex, Vec3 pos, Vec2 size, const std::vector<Scene::PointLight>& lights,
                       TextureID normal_map, PivotPoint pivot)
{
    render_sprite_lit(tex, pos, size, Vec4(0.0f, 0.0f, 1.0f, 1.0f), lights, normal_map, pivot);
}

void render_sprite_lit(TextureID tex, Vec3 pos, Vec2 size, Vec4 tex_coord_range, const std::vector<Scene::PointLight>& lights,
                       TextureID normal_map, PivotPoint pivot)
{
    // If no normal map provided, use the diffuse texture as fallback (assumes flat surface)
    if (normal_map == 0) {
        normal_map = tex;
    }
    
    // Convert pixel coordinates to OpenGL coordinates
    Vec2 opengl_pos = Coords::pixel_to_opengl(Vec2(pos.x, pos.y), g_viewport_width, g_viewport_height);
    Vec2 opengl_size = Vec2(
        (size.x / (float)g_viewport_width) * 2.0f,
        (size.y / (float)g_viewport_height) * 2.0f
    );
    
    // Calculate offset from pivot point to center
    Vec2 pivot_offset = Coords::get_pivot_offset(pivot, size.x, size.y);
    Vec2 pivot_offset_opengl = Vec2(
        (pivot_offset.x / (float)g_viewport_width) * 2.0f,
        -(pivot_offset.y / (float)g_viewport_height) * 2.0f  // Negate because Y is inverted
    );
    
    // Shader expects center point
    Vec2 opengl_center = Vec2(
        opengl_pos.x + pivot_offset_opengl.x,
        opengl_pos.y + pivot_offset_opengl.y
    );
    
    glUseProgram(litShaderProgram);
    
    // Set sprite uniforms
    GLint posLoc = glGetUniformLocation(litShaderProgram, "spritePos");
    GLint sizeLoc = glGetUniformLocation(litShaderProgram, "spriteSize");
    GLint zLoc = glGetUniformLocation(litShaderProgram, "spriteZ");
    GLint texCoordLoc = glGetUniformLocation(litShaderProgram, "texCoordRange");
    
    glUniform2f(posLoc, opengl_center.x, opengl_center.y);
    glUniform2f(sizeLoc, opengl_size.x, opengl_size.y);
    glUniform1f(zLoc, pos.z);  // Use z component from Vec3
    glUniform4f(texCoordLoc, tex_coord_range.x, tex_coord_range.y, tex_coord_range.z, tex_coord_range.w);
    
    // Set light uniforms (max 8 lights)
    uint32_t num_lights = lights.size() > 8 ? 8 : (uint32_t)lights.size();
    glUniform1i(glGetUniformLocation(litShaderProgram, "numLights"), num_lights);
    
    // Set scene size for height map sampling
    glUniform2f(glGetUniformLocation(litShaderProgram, "sceneSize"), (float)g_scene_width, (float)g_scene_height);
    
    // Send lights as full 3D vectors
    std::vector<Vec3> light_positions_3d;
    std::vector<float> light_intensities, light_radii;
    std::vector<Vec3> light_colors;
    
    for (uint32_t i = 0; i < num_lights; i++) {
        const Scene::PointLight& light = lights[i];
        
        // Convert light position to OpenGL coordinates (full 3D)
        Vec2 light_xy_opengl = Coords::pixel_to_opengl(Vec2(light.position.x, light.position.y), g_viewport_width, g_viewport_height);
        // Keep z-depth as-is (already in OpenGL space: -1 to 1)
        Vec3 light_pos_opengl(light_xy_opengl.x, light_xy_opengl.y, light.position.z);
        light_positions_3d.push_back(light_pos_opengl);
        light_colors.push_back(light.color);
        light_intensities.push_back(light.intensity);
        // Convert radius to OpenGL scale (in pixel space it's relative to viewport)
        float radius_ndc = (light.radius / (float)g_viewport_width) * 2.0f;
        light_radii.push_back(radius_ndc);
    }
    
    // Set arrays (all 3D now)
    glUniform3fv(glGetUniformLocation(litShaderProgram, "lightPositions"), num_lights, (float*)light_positions_3d.data());
    glUniform3fv(glGetUniformLocation(litShaderProgram, "lightColors"), num_lights, (float*)light_colors.data());
    glUniform1fv(glGetUniformLocation(litShaderProgram, "lightIntensities"), num_lights, light_intensities.data());
    glUniform1fv(glGetUniformLocation(litShaderProgram, "lightRadii"), num_lights, light_radii.data());
    
    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normal_map);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, g_height_map_texture);
    
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void render_sprite_animated_lit(const SpriteAnimation* anim, Vec3 pos, Vec2 size, const std::vector<Scene::PointLight>& lights,
                                TextureID normal_map, PivotPoint pivot)
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
    
    // Get UV coordinates for current frame
    const SpriteFrame& frame = anim->frames[anim->current_frame];
    Vec4 tex_coord_range(frame.u0, frame.v0, frame.u1, frame.v1);
    
    // Render using sprite map texture with UV mapping and lighting
    render_sprite_lit(anim->texture, pos, size, tex_coord_range, lights, normal_map, pivot);
}

}
