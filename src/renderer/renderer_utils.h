#pragma once

#include <cstdint>

namespace Renderer {

using GLuint = uint32_t;

GLuint compile_and_link_shader(const char* vertexSrc, const char* fragSrc);

}
