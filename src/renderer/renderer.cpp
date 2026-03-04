#include "renderer.h"
#include "renderer_utils.h"
#include <OpenGL/gl3.h>

namespace Renderer {

static GLuint quadVAO = 0;
static GLuint quadVBO = 0;
static GLuint shaderProgram = 0;

void init_renderer(uint32_t width, uint32_t height)
{
    // Set viewport
    glViewport(0, 0, width, height);

    float vertices[] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
        -0.5f,  0.5f,
         0.5f,  0.5f
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Shader sources (OpenGL 3.3 Core)
    const char* vertexSrc = "#version 330 core\nlayout(location=0) in vec2 aPos;\nvoid main() { gl_Position = vec4(aPos,0.0,1.0); }";
    const char* fragSrc   = "#version 330 core\nout vec4 FragColor;\nvoid main() { FragColor = vec4(1.0,0.0,0.0,1.0); }";

    shaderProgram = compile_and_link_shader(vertexSrc, fragSrc);
}

void clear_screen()
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void render_quad()
{
    glUseProgram(shaderProgram);
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
