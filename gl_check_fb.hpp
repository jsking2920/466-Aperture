//from https://github.com/15-466/15-466-f20-framebuffer/blob/master/gl_check_fb.hpp
#pragma once

#include "GL.hpp"
#include <stdexcept>
#include <string>

//Helper: throw exception if currently bound framebuffer isn't complete:
inline void gl_check_fb() {
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status == GL_FRAMEBUFFER_COMPLETE) {
    } else
#define CHECK( S ) \
	if (status == S) { \
		throw std::runtime_error("Framebuffer status: " #S); \
	} else
    CHECK(GL_FRAMEBUFFER_UNDEFINED)
    CHECK(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
    CHECK(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
    CHECK(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER)
    CHECK(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER)
    CHECK(GL_FRAMEBUFFER_UNSUPPORTED)
    CHECK(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
    CHECK(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS)
    {
        throw std::runtime_error("Framebuffer status: " + std::to_string(status) + " (unknown)");
    }
#undef CHECK
}