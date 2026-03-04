#include "renderer.h"
#include "renderer_utils.h"
#include "shader_loader.h"
#include "texture_loader.h"
#include <OpenGL/gl3.h>

namespace Renderer {

static GLuint quadVAO = 0;
static GLuint quadVBO = 0;
static GLuint shaderProgram = 0;

void init_renderer(uint32_t width, uint32_t height)
{
    // Set viewport
    glViewport(0, 0, width, height);

    // Vertices with position and texture coordinates
    // Position in range -0.5 to 0.5 (will be scaled by render_sprite)
    float vertices[] = {
        // Position       // TexCoord
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

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    // TexCoord attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    std::string vertexSrc = load_shader_source("src/renderer/shaders/basic.vert");
    std::string fragSrc = load_shader_source("src/renderer/shaders/basic.frag");

    shaderProgram = compile_and_link_shader(vertexSrc.c_str(), fragSrc.c_str());
    
    // Set shader uniform for texture sampler (once, doesn't change)
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture0"), 0);
    glUseProgram(0);
}

void clear_screen()
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void render_sprite(TextureID tex, Vec2 pos, Vec2 size)
{
    glUseProgram(shaderProgram);
    
    // Set sprite position and size uniforms
    GLint posLoc = glGetUniformLocation(shaderProgram, "spritePos");
    GLint sizeLoc = glGetUniformLocation(shaderProgram, "spriteSize");
    glUniform2f(posLoc, pos.x, pos.y);
    glUniform2f(sizeLoc, size.x, size.y);
    
    // Bind and activate texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    
    // Draw
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void shutdown()
{
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
    }
    if (quadVAO) {
        glDeleteVertexArrays(1, &quadVAO);
    }
    if (quadVBO) {
        glDeleteBuffers(1, &quadVBO);
    }
}

}

