//Framebuffer code, adjusted from https://github.com/15-466/15-466-f20-framebuffer and https://learnopengl.com/Advanced-OpenGL/Anti-Aliasing

#include "Framebuffers.hpp"
#include "Load.hpp"
#include "gl_compile_program.hpp"
#include "gl_check_fb.hpp"
#include "gl_errors.hpp"

#include <array>

Framebuffers framebuffers;

void Framebuffers::realloc(glm::uvec2 const &drawable_size, glm::uvec2 const &new_shadow_size) {

    // return early if resizing is not needed
    if (drawable_size == size) return;
    size = drawable_size;

    // Resize ms_color_tex
    {
        //name texture if not yet named:
        if (ms_color_tex == 0) glGenTextures(1, &ms_color_tex);

        //resize texture:
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, ms_color_tex); // multisampled texture to support msaa
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
            msaa_samples, // number of samples per pixel
            GL_RGB16F, //<-- storage will be RGB 16-bit half-float
            size.x, size.y, //width, height
            GL_TRUE //<-- use identical sample locations and the same number of samples per texel
        );
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    }
   
    // Resize ms_depth_rb
    {

        //name renderbuffer if not yet named:
        if (ms_depth_rb == 0) glGenRenderbuffers(1, &ms_depth_rb);

        //resize renderbuffer:
        glBindRenderbuffer(GL_RENDERBUFFER, ms_depth_rb);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER,
            msaa_samples, // number of samples per pixel
            GL_DEPTH_COMPONENT24, //<-- storage will be 24-bit fixed point depth values
            size.x, size.y);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }
    
    // Resize ms_fb
    {
        //set up ms_fb if not yet named:
        if (ms_fb == 0) {
            glGenFramebuffers(1, &ms_fb);
            glBindFramebuffer(GL_FRAMEBUFFER, ms_fb);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, ms_color_tex, 0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, ms_depth_rb);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // make sure ms_fb isn't borked
        glBindFramebuffer(GL_FRAMEBUFFER, ms_fb);
        gl_check_fb(); //<-- helper function to check framebuffer completeness
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    // Resize screen_texture
    {
        // Set up screen_texture if not yet named
        if (screen_texture == 0) glGenTextures(1, &screen_texture);

        // resize screen_texture and set parameters
        glBindTexture(GL_TEXTURE_2D, screen_texture);
        glTexImage2D(GL_TEXTURE_2D, 0,
            GL_RGB16F, //<-- storage will be RGB 16-bit half-float
            size.x, size.y, 0, //width, height, border
            GL_RGB, GL_FLOAT, //<-- source data (if we were uploading it) would be floating point RGB
            nullptr //<-- don't upload data, just allocate on-GPU storage
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Resize pp_fb
    {
        // set up pp_fb if not yet named
        if (pp_fb == 0) {
            glGenFramebuffers(1, &pp_fb);
            glBindFramebuffer(GL_FRAMEBUFFER, pp_fb);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screen_texture, 0);
        }

        // make sure pp_fb isn't borked
        glBindFramebuffer(GL_FRAMEBUFFER, pp_fb);
        gl_check_fb(); //<-- helper function to check framebuffer completeness
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    // Resize blur textures
    {
        // set up tex if not yet named
        if (blur_x_tex == 0) glGenTextures(1, &blur_x_tex);

        // resize texture
        glBindTexture(GL_TEXTURE_2D, blur_x_tex);
        glTexImage2D(GL_TEXTURE_2D, 0,
            GL_RGB16F, //<-- storage will be RGB 16-bit half-float
            size.x, size.y, 0, //width, height, border
            GL_RGB, GL_FLOAT, //<-- source data (if we were uploading it) would be floating point RGB
            nullptr //<-- don't upload data, just allocate on-GPU storage
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        // set up framebuffer if not yet named
        if (blur_x_fb == 0) {
            glGenFramebuffers(1, &blur_x_fb);
            glBindFramebuffer(GL_FRAMEBUFFER, blur_x_fb);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blur_x_tex, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // make sure blur_x_fb isn't borked
        glBindFramebuffer(GL_FRAMEBUFFER, blur_x_fb);
        gl_check_fb(); //<-- helper function to check framebuffer completeness
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    //allocate shadow map framebuffer, from https://github.com/ixchow/15-466-f18-base3
    if (shadow_size != new_shadow_size) {
        shadow_size = new_shadow_size;

        if (shadow_color_tex == 0) glGenTextures(1, &shadow_color_tex);
        glBindTexture(GL_TEXTURE_2D, shadow_color_tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, shadow_size.x, shadow_size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);


        if (shadow_depth_tex == 0) glGenTextures(1, &shadow_depth_tex);
        glBindTexture(GL_TEXTURE_2D, shadow_depth_tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, shadow_size.x, shadow_size.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (shadow_fb == 0) glGenFramebuffers(1, &shadow_fb);
        glBindFramebuffer(GL_FRAMEBUFFER, shadow_fb);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadow_color_tex, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_depth_tex, 0);
        gl_check_fb();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        GL_ERRORS();
    }

    // Check for gl errors
    GL_ERRORS();
}

struct ToneMapProgram {
    ToneMapProgram() {
        program = gl_compile_program(
                //vertex shader -- draws a fullscreen triangle using no attribute streams
                "#version 330\n"
                "void main() {\n"
                "	gl_Position = vec4(4 * (gl_VertexID & 1) - 1,  2 * (gl_VertexID & 2) - 1, 0.0, 1.0);\n"
                "}\n"
                ,
                //fragment shader -- reads a HDR texture, maps to output pixel colors
                "#version 330\n"
                "uniform sampler2D TEX;\n"
                "out vec4 fragColor;\n"
                "void main() {\n"
                "	vec3 color = texelFetch(TEX, ivec2(gl_FragCoord.xy), 0).rgb;\n"
                //exposure-correction-style range compression:
//                "		color = vec3(log(color.r + 1.0), log(color.g + 1.0), log(color.b + 1.0)) / log(2.0 + 1.0);\n"
                //weird color effect:
                //"		color = vec3(color.rg, gl_FragCoord.x/textureSize(TEX,0).x);\n"
                //basic gamma-style range compression:
                //"		color = vec3(pow(color.r, 0.45), pow(color.g, 0.45), pow(color.b, 0.45));\n"
                //raw values:
                "		color = color;\n"
                "	fragColor = vec4(color, 1.0);\n"
                "}\n"
        );

        //set TEX to texture unit 0:
        GLuint TEX_sampler2D = glGetUniformLocation(program, "TEX");
        glUseProgram(program);
        glUniform1i(TEX_sampler2D, 0);
        glUseProgram(0);

        GL_ERRORS();
    }

    GLuint program = 0;

    //uniforms:
    //none

    //textures:
    //texture0 -- texture to copy
};

GLuint empty_vao = 0;
Load< ToneMapProgram > tone_map_program(LoadTagEarly, []() -> ToneMapProgram const * {
    glGenVertexArrays(1, &empty_vao);
    return new ToneMapProgram();
});

void Framebuffers::tone_map() {
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    glUseProgram(tone_map_program->program);
    glBindVertexArray(empty_vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screen_texture);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindTexture(GL_TEXTURE_2D, 0);

    glBindVertexArray(0);
    glUseProgram(0);
    GL_ERRORS();
}

constexpr uint32_t KERNEL_RADIUS = 30;
std::array< float, KERNEL_RADIUS > bloom_kernel = ([](){
    std::array< float, KERNEL_RADIUS > weights;
    //compute a bloom kernel as a somewhat peaked distribution,
    // hand authored to look a'ight:

    // NOTE: you could probably compute this more exactly by:
    //  - simulating the scattering behavior in human eyes to get
    //    a 2D PSF (point spread function).
    //    (this probably also depends on the expected on-screen
    //     size of the image.)
    //  - splitting this PSF into an outer product of two 1D blurs
    //    (doable with an SVD).

    for (uint32_t i = 0; i < weights.size(); ++i) {
        float pos = i / float(weights.size());
        weights[i] = std::pow(20.0f, 1.0f - pos) - 1.0f;
    }

    //normalize:
    float sum = 0.0f;
    for (auto const &w : weights) sum += w;
    sum = 2.0f * sum - weights[0]; //account for the fact that all taps other than center are used 2x
    float inv_sum = 1.0f / sum;
    for (auto &w : weights) w *= inv_sum;

    return weights;
})();

struct BlurXProgram {
    BlurXProgram() {
        program = gl_compile_program(
                //vertex shader -- draws a fullscreen triangle using no attribute streams
                "#version 330\n"
                "void main() {\n"
                "	gl_Position = vec4(4 * (gl_VertexID & 1) - 1,  2 * (gl_VertexID & 2) - 1, 0.0, 1.0);\n"
                "}\n"
                ,
                //fragment shader -- blur in X direction with a given kernel
                "#version 330\n"
                "uniform sampler2D TEX;\n"
                "const int KERNEL_RADIUS = " + std::to_string(KERNEL_RADIUS) + ";\n"
                                                                               "uniform float KERNEL[KERNEL_RADIUS];\n"
                                                                               "out vec4 fragColor;\n"
                                                                               "void main() {\n"
                                                                               "	ivec2 c = ivec2(gl_FragCoord.xy);\n"
                                                                               "	int limit = textureSize(TEX, 0).x-1;\n"
                                                                               "	vec3 acc = KERNEL[0] * texelFetch(TEX, c, 0).rgb;\n"
                                                                               "	for (int ofs = 1; ofs < KERNEL_RADIUS; ++ofs) {\n"
                                                                               "		acc += KERNEL[ofs] * (\n"
                                                                               "			  texelFetch(TEX, ivec2(min(c.x+ofs, limit), c.y), 0).rgb\n"
                                                                               "			+ texelFetch(TEX, ivec2(max(c.x-ofs, 0), c.y), 0).rgb\n"
                                                                               "		);\n"
                                                                               "	}\n"
                                                                               "	fragColor = vec4(acc,1.0);\n"
                                                                               "}\n"
        );

        glUseProgram(program);
        //set KERNEL:
        GLuint KERNEL_float_array = glGetUniformLocation(program, "KERNEL");
        glUniform1fv(KERNEL_float_array, KERNEL_RADIUS, bloom_kernel.data());


        //set TEX to texture unit 0:
        GLuint TEX_sampler2D = glGetUniformLocation(program, "TEX");
        glUniform1i(TEX_sampler2D, 0);

        glUseProgram(0);

        GL_ERRORS();
    }

    GLuint program = 0;

    //uniforms:
    //none

    //textures:
    //texture0 -- texture to copy
};

Load< BlurXProgram > blur_x_program(LoadTagEarly);


struct BlurYProgram {
    BlurYProgram() {
        program = gl_compile_program(
                //vertex shader -- draws a fullscreen triangle using no attribute streams
                "#version 330\n"
                "void main() {\n"
                "	gl_Position = vec4(4 * (gl_VertexID & 1) - 1,  2 * (gl_VertexID & 2) - 1, 0.0, 1.0);\n"
                "}\n"
                ,
                //fragment shader -- blur in X direction with a given kernel
                "#version 330\n"
                "uniform sampler2D TEX;\n"
                "const int KERNEL_RADIUS = " + std::to_string(KERNEL_RADIUS) + ";\n"
                                                                               "uniform float KERNEL[KERNEL_RADIUS];\n"
                                                                               "out vec4 fragColor;\n"
                                                                               "void main() {\n"
                                                                               "	ivec2 c = ivec2(gl_FragCoord.xy);\n"
                                                                               "	int limit = textureSize(TEX, 0).y-1;\n"
                                                                               "	vec3 acc = KERNEL[0] * texelFetch(TEX, c, 0).rgb;\n"
                                                                               "	for (int ofs = 1; ofs < KERNEL_RADIUS; ++ofs) {\n"
                                                                               "		acc += KERNEL[ofs] * (\n"
                                                                               "			  texelFetch(TEX, ivec2(c.x, min(c.y+ofs, limit)), 0).rgb\n"
                                                                               "			+ texelFetch(TEX, ivec2(c.x, max(c.y-ofs, 0)), 0).rgb\n"
                                                                               "		);\n"
                                                                               "	}\n"
                                                                               "	fragColor = vec4(acc,0.1);\n" //<-- alpha here controls strength of effect, because blending used on this pass
                                                                               "}\n"
        );

        glUseProgram(program);
        //set KERNEL:
        GLuint KERNEL_float_array = glGetUniformLocation(program, "KERNEL");
        glUniform1fv(KERNEL_float_array, KERNEL_RADIUS, bloom_kernel.data());


        //set TEX to texture unit 0:
        GLuint TEX_sampler2D = glGetUniformLocation(program, "TEX");
        glUniform1i(TEX_sampler2D, 0);

        glUseProgram(0);

        GL_ERRORS();
    }

    GLuint program = 0;

    //uniforms:
    //none

    //textures:
    //texture0 -- texture to copy
};

Load< BlurYProgram > blur_y_program(LoadTagEarly);

void Framebuffers::add_bloom() {
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    // blur screen_texture in the X direction, store into blur_x_tex:
    glBindFramebuffer(GL_FRAMEBUFFER, blur_x_fb);

    glUseProgram(blur_x_program->program);
    glBindVertexArray(empty_vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screen_texture);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindTexture(GL_TEXTURE_2D, 0);

    glBindVertexArray(0);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // blur blur_x_tex in the Y direction, store back into screen_texture
    glBindFramebuffer(GL_FRAMEBUFFER, screen_texture);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(blur_y_program->program);
    glBindVertexArray(empty_vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, blur_x_tex);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindTexture(GL_TEXTURE_2D, 0);

    glBindVertexArray(0);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDisable(GL_BLEND);

    GL_ERRORS();
}
