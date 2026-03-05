#include "renderer.h"
#include "renderer_utils.h"
#include "shader_loader.h"
#include "texture_loader.h"
#include "debug/debug_log.h"
#include <OpenGL/gl3.h>

namespace Renderer {

static GLuint quadVAO = 0;
static GLuint quadVBO = 0;
static GLuint shaderProgram = 0;
static GLuint colorShaderProgram = 0;
static uint32_t g_viewport_width = 0;
static uint32_t g_viewport_height = 0;

void init_renderer(uint32_t width, uint32_t height)
{
    g_viewport_width = width;
    g_viewport_height = height;
    glViewport(0, 0, width, height);
    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
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
}

void clear_screen()
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void render_sprite(TextureID tex, Vec2 pos, Vec2 size, float z_depth, PivotPoint pivot)
{
    // Default texture coordinate range (full texture)
    render_sprite(tex, pos, size, Vec4(0.0f, 0.0f, 1.0f, 1.0f), z_depth, pivot);
}

void render_sprite(TextureID tex, Vec2 pos, Vec2 size, Vec4 tex_coord_range, float z_depth, PivotPoint pivot)
{
    // Convert pixel coordinates to OpenGL coordinates
    Vec2 opengl_pos = Coords::pixel_to_opengl(pos, g_viewport_width, g_viewport_height);
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
    glUniform1f(zLoc, z_depth);
    glUniform4f(texCoordLoc, tex_coord_range.x, tex_coord_range.y, tex_coord_range.z, tex_coord_range.w);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void render_sprite_animated(const SpriteAnimation* anim, Vec2 pos, Vec2 size, float z_depth, PivotPoint pivot)
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
    TextureID current_tex = anim->frames[anim->current_frame];
    render_sprite(current_tex, pos, size, z_depth, pivot);
}

void render_rect(Vec2 pos, Vec2 size, Vec4 color, float z_depth, PivotPoint pivot)
{
    // Convert pixel coordinates to OpenGL coordinates
    Vec2 opengl_pos = Coords::pixel_to_opengl(pos, g_viewport_width, g_viewport_height);
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
    glUniform1f(zLoc, z_depth);
    glUniform4f(colorLoc, color.x, color.y, color.z, color.w);
    
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void shutdown()
{
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
    }
    if (colorShaderProgram) {
        glDeleteProgram(colorShaderProgram);
    }
    if (quadVAO) {
        glDeleteVertexArrays(1, &quadVAO);
    }
    if (quadVBO) {
        glDeleteBuffers(1, &quadVBO);
    }
}
}
