//Framebuffer code, adjusted from https://github.com/15-466/15-466-f20-framebuffer and https://learnopengl.com/Advanced-OpenGL/Anti-Aliasing

#include "Framebuffers.hpp"
#include "Load.hpp"
#include "gl_compile_program.hpp"
#include "gl_check_fb.hpp"
#include "gl_errors.hpp"

#include <glm/gtc/type_ptr.hpp>
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
        GL_ERRORS();
    }
   
    // Resize ms_depth_rb
    {

        //name renderbuffer if not yet named:
        if (ms_depth_tex == 0) glGenTextures(1, &ms_depth_tex);

        //resize renderbuffer:
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, ms_depth_tex);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
            msaa_samples, // number of samples per pixel
            GL_DEPTH_COMPONENT24, //<-- storage will be 24-bit fixed point depth values
            size.x, size.y, GL_TRUE);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
        GL_ERRORS();
    }
    
    // Resize ms_fb
    {
//        glTexImage2DM(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, shadow_size.x, shadow_size.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
        //set up ms_fb if not yet named:
        if (ms_fb == 0) {
            glGenFramebuffers(1, &ms_fb);
            glBindFramebuffer(GL_FRAMEBUFFER, ms_fb);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, ms_color_tex, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, ms_depth_tex, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // make sure ms_fb isn't borked
        glBindFramebuffer(GL_FRAMEBUFFER, ms_fb);
        gl_check_fb(); //<-- helper function to check framebuffer completeness
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        GL_ERRORS();
    }

    // Resize depth_effect_tex
    {
        //name texture if not yet named:
        if (depth_effect_tex == 0) glGenTextures(1, &depth_effect_tex);

        //resize texture:
        glBindTexture(GL_TEXTURE_2D, depth_effect_tex);
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
        GL_ERRORS();
    }

    // Resize depth_effect_fb
    {
        //set up depth_effect_fb if not yet named:
        if (depth_effect_fb == 0) {
            glGenFramebuffers(1, &depth_effect_fb);
            glBindFramebuffer(GL_FRAMEBUFFER, depth_effect_fb);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, depth_effect_tex, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // make sure depth_effect_fb isn't borked
        glBindFramebuffer(GL_FRAMEBUFFER, depth_effect_fb);
        gl_check_fb(); //<-- helper function to check framebuffer completeness
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        GL_ERRORS();
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
        GL_ERRORS();
    }


    //Resize pp_depth
    {
        if(pp_depth == 0) glGenTextures(1, &pp_depth);

        glBindTexture(GL_TEXTURE_2D, pp_depth);
        glTexImage2D(GL_TEXTURE_2D, 0,
                     GL_DEPTH_COMPONENT24,
                     size.x, size.y, 0, //width, height, border
                     GL_DEPTH_COMPONENT, GL_FLOAT,
                     nullptr //<-- don't upload data, just allocate on-GPU storage
        );
        glBindTexture(GL_TEXTURE_2D, 0);
        GL_ERRORS();
    }

    // Resize pp_fb
    {
        // set up pp_fb if not yet named
        if (pp_fb == 0) {
            glGenFramebuffers(1, &pp_fb);
            glBindFramebuffer(GL_FRAMEBUFFER, pp_fb);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screen_texture, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pp_depth, 0);
        }

        // make sure pp_fb isn't borked
        glBindFramebuffer(GL_FRAMEBUFFER, pp_fb);
        gl_check_fb(); //<-- helper function to check framebuffer completeness
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        GL_ERRORS();
    }

    //Resize oc_position_rb
    {
        if(vertex_position_tex == 0) glGenTextures(1, &vertex_position_tex);

        glBindTexture(GL_TEXTURE_2D, vertex_position_tex);
        glTexImage2D(GL_TEXTURE_2D, 0,
                     GL_RGBA32F,
                     size.x, size.y, 0, //width, height, border
                     GL_RGBA, GL_FLOAT, //<-- source data (if we were uploading it) would be floating point RGBA
                     nullptr //<-- don't upload data, just allocate on-GPU storage
        );
//        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
//                                  4, // number of samples per pixel
//                                GL_RGBA32F, //<-- storage will be RGBA 32-bit float
//                                  size.x, size.y, //width, height
//                                  GL_TRUE //<-- use identical sample locations and the same number of samples per texel
//        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        GL_ERRORS();
    }

    //Resize oc_fb
    {
        if (oc_fb == 0) {
            glGenFramebuffers(1, &oc_fb);
            glBindFramebuffer(GL_FRAMEBUFFER, oc_fb);
            //rgba texture to hold positions
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, vertex_position_tex, 0);
            //depth texture for querying for occlusion
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, pp_depth, 0); //comes pre-filled from blitting (used for occlusion)
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // make sure pp_fb isn't borked
        glBindFramebuffer(GL_FRAMEBUFFER, oc_fb);
        gl_check_fb(); //<-- helper function to check framebuffer completeness
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        GL_ERRORS();
    }
    
    // Resize blur textures
    {
        // set up tex if not yet named
        if (blur_tex == 0) glGenTextures(1, &blur_tex);

        // resize texture
        glBindTexture(GL_TEXTURE_2D, blur_tex);
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
        if (blur_fb == 0) {
            glGenFramebuffers(1, &blur_fb);
            glBindFramebuffer(GL_FRAMEBUFFER, blur_fb);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blur_tex, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // make sure blur_x_fb isn't borked
        glBindFramebuffer(GL_FRAMEBUFFER, blur_fb);
        gl_check_fb(); //<-- helper function to check framebuffer completeness
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        GL_ERRORS();
    }

    //allocate shadow map framebuffer, from https://github.com/ixchow/15-466-f18-base3
    if (shadow_size != new_shadow_size) {
        shadow_size = new_shadow_size;

        //for debug shadow color
//        if (shadow_color_tex == 0) glGenTextures(1, &shadow_color_tex);
//        glBindTexture(GL_TEXTURE_2D, shadow_color_tex);
//        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, shadow_size.x, shadow_size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//        glBindTexture(GL_TEXTURE_2D, 0);


        if (shadow_depth_tex == 0) glGenTextures(1, &shadow_depth_tex);
        glBindTexture(GL_TEXTURE_2D, shadow_depth_tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, shadow_size.x, shadow_size.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        //border color method from https://learnopengl.com/Getting-started/Textures
        float border_color[] = { 1.0, 0.0, 0.0, 0.0 };
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (shadow_fb == 0) glGenFramebuffers(1, &shadow_fb);
        glBindFramebuffer(GL_FRAMEBUFFER, shadow_fb);
//        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadow_color_tex, 0);
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
                "vec3 adjustContrast(vec3 color, float value) {\n"
                "  return 0.5 + value * (color - 0.5);\n"
                "}\n"
                "uniform sampler2D TEX;\n"
                "out vec4 fragColor;\n"
                "void main() {\n"
                "	vec3 color = texelFetch(TEX, ivec2(gl_FragCoord.xy), 0).rgb;\n"
                //exposure-correction-style range compression:
//                "		color = vec3(log(color.r + 1.0), log(color.g + 1.0), log(color.b + 1.0)) / log(2.0 + 1.0);\n"
                //weird color effect:
//                "		color = vec3(color.rg, gl_FragCoord.x/textureSize(TEX,0).x);\n"
                //basic gamma-style range compression:
//                "		color = vec3(pow(color.r, 0.45), pow(color.g, 0.45), pow(color.b, 0.45));\n"
                //raw values:
//                "		color = vec3(color.r - color.r % 0.01, color.r - color.g % 0.01, color.r - color.b % 0.01);\n"
//                "		color = vec3(pow(color.r, 2) * 2, pow(color.g, 2)* 2, pow(color.b, 2)* 2);\n"
                "   color = adjustContrast(color, 1.2f);\n"
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

//Tone Map & Blur program adjusted from https://github.com/15-466/15-466-f20-framebuffer

GLuint empty_vao = 0;
Load< ToneMapProgram > tone_map_program(LoadTagEarly, []() -> ToneMapProgram const * {
    if(empty_vao == 0) glGenVertexArrays(1, &empty_vao);
    return new ToneMapProgram();
});

void Framebuffers::tone_map_to_screen(GLuint texture) {
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    glUseProgram(tone_map_program->program);
    glBindVertexArray(empty_vao);

    glActiveTexture(GL_TEXTURE0);
//    glBindTexture(GL_TEXTURE_2D, shadow_color_tex);
    glBindTexture(GL_TEXTURE_2D, texture);

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
               "	fragColor = vec4(acc,1.0);\n" //<-- alpha here controls strength of effect, because blending used on this pass
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

//assisted by this tutorial https://lettier.github.io/3d-game-shaders-for-beginners/depth-of-field.html
struct DepthOfFieldProgram {
    DepthOfFieldProgram() {
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
                "uniform sampler2D POS_TEX;\n"
                "uniform sampler2D BLUR_TEX;\n"
                "uniform vec3 PLAYER_POS;\n"
                "uniform float FOCAL_DISTANCE;\n"
                "out vec4 fragColor;\n"
                "void main() {\n"
                "	ivec2 c = ivec2(gl_FragCoord.xy);\n"
                "	vec3 location = vec3(texelFetch(POS_TEX, c, 0));\n"
                "   float blur;"
                "   if(location == vec3(0, 0, 0)) {\n"
                "       blur = 1.0f;\n"
                "   } else {\n"
                "	    float distance = distance(PLAYER_POS, location);\n"
                "       float range = 1.0f;\n"
                "       float offset = FOCAL_DISTANCE/2;\n"
                "	    blur = clamp((abs(offset + FOCAL_DISTANCE - distance) - range)/(offset), 0, 1);\n" //returns 0-1, first arg can be changed to increase "in focus" range
                "   }\n"
                "	fragColor = mix(texelFetch(TEX, c, 0), texelFetch(BLUR_TEX, c, 0), blur);\n"
//                "	fragColor = vec4(blur, blur, blur, 1.0);\n"
//                "	fragColor = texelFetch(TEX, c, 0);\n"
                "}\n"
        );

        glUseProgram(program);

        //set uniforms
        PLAYER_POS_vec3 = glGetUniformLocation(program, "PLAYER_POS");
        FOCAL_DISTANCE_float = glGetUniformLocation(program, "FOCAL_DISTANCE");

        //set TEX to texture unit 0:
        GLuint TEX_sampler2D = glGetUniformLocation(program, "TEX");
        glUniform1i(TEX_sampler2D, 0);

        //set POS_TEX to texture unit 1:
        GLuint POS_TEX_sampler2D = glGetUniformLocation(program, "POS_TEX");
        glUniform1i(POS_TEX_sampler2D, 1);

        GLuint BLUR_TEX_sampler2D = glGetUniformLocation(program, "BLUR_TEX");
        glUniform1i(BLUR_TEX_sampler2D, 2);

        glUseProgram(0);

        GL_ERRORS();
    }

    GLuint program = 0;

    //uniforms:
    GLuint PLAYER_POS_vec3 = -1U;
    GLuint FOCAL_DISTANCE_float = -1U;

    //textures:
    //texture0 -- texture to copy
    //texture1 -- position texture
    //texture2 -- blurred texture
};

Load< DepthOfFieldProgram > depth_of_field(LoadTagEarly, []() -> DepthOfFieldProgram const * {
    if (empty_vao == 0) glGenVertexArrays(1, &empty_vao);
    return new DepthOfFieldProgram();
});

void Framebuffers::add_depth_of_field(float focal_distance, glm::vec3 player_pos) {
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    // blur depth_effect_tex in the X direction, store into screen_texture:
    glBindFramebuffer(GL_FRAMEBUFFER, pp_fb);

    glUseProgram(blur_x_program->program);
    glBindVertexArray(empty_vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depth_effect_tex);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindTexture(GL_TEXTURE_2D, 0);

    glBindVertexArray(0);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // blur screen_texture in the Y direction, store into blur_tex
    glBindFramebuffer(GL_FRAMEBUFFER, blur_fb);

    //blending disabled?
//    glEnable(GL_BLEND);
//    glBlendEquation(GL_FUNC_ADD);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(blur_y_program->program);
    glBindVertexArray(empty_vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screen_texture);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindTexture(GL_TEXTURE_2D, 0);

    glBindVertexArray(0);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

//    glDisable(GL_BLEND);
    GL_ERRORS();



    //Add depth of field, store into pp_fb/screen_texture
    glBindFramebuffer(GL_FRAMEBUFFER, pp_fb);

    glUseProgram(depth_of_field->program);
    glBindVertexArray(empty_vao);

    //set uniforms
    glUniform1f(depth_of_field->FOCAL_DISTANCE_float, focal_distance );
    glUniform3fv(depth_of_field->PLAYER_POS_vec3, 1, glm::value_ptr(player_pos));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depth_effect_tex);

    //texture1 is vertex position texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, vertex_position_tex);

    //texture2 is blurred texture
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, blur_tex);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    //unbind textures
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindVertexArray(0);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    GL_ERRORS();
}

struct DepthEffectsProgram {
    DepthEffectsProgram() {
        program = gl_compile_program(
                //vertex shader -- draws a fullscreen triangle using no attribute streams
                "#version 330\n"
                "void main() {\n"
                "	gl_Position = vec4(4 * (gl_VertexID & 1) - 1,  2 * (gl_VertexID & 2) - 1, 0.0, 1.0);\n"
                "}\n"
                ,
                //fragment shader -- add color based on depth
                "#version 330\n"
                "uniform sampler2D TEX;\n"
                "uniform sampler2DMS DEPTH_TEX;\n"
                "uniform float FOG_EXP;\n"
                "uniform float FOG_INTENSITY;\n"
                "uniform vec3 FOG_COLOR;\n"
                "out vec4 fragColor;\n"
                "void main() {\n"
                "	ivec2 c = ivec2(gl_FragCoord.xy);\n"
                "	vec4 color = texelFetch(TEX, c, 0);\n"
                "   float depth = pow((texelFetch(DEPTH_TEX, c, 0).r + texelFetch(DEPTH_TEX, c, 1).r + texelFetch(DEPTH_TEX, c, 2).r + texelFetch(DEPTH_TEX, c, 3).r)/4.0, FOG_EXP);\n"
                "   float intensity = depth * FOG_INTENSITY;\n"
//                "   fragColor = vec4(vec3(intensity), 1.0);\n"
                "	fragColor = vec4(mix(color.rgb, FOG_COLOR, intensity), 1.0);\n"
                "}\n"
        );

        glUseProgram(program);

        //set uniform locaitons:
        FOG_EXP_float = glGetUniformLocation(program, "FOG_EXP");
        FOG_INTENSITY_float = glGetUniformLocation(program, "FOG_INTENSITY");
        FOG_COLOR_vec3 = glGetUniformLocation(program, "FOG_COLOR");

        //set TEX to texture unit 0:
        GLuint TEX_sampler2D = glGetUniformLocation(program, "TEX");
        GLuint DEPTH_TEX_sampler2D = glGetUniformLocation(program, "DEPTH_TEX");

        glUniform1i(TEX_sampler2D, 0);
        glUniform1i(DEPTH_TEX_sampler2D, 1); //set DEPTH_TEX to sample from GL_TEXTURE1

        glUseProgram(0);

        GL_ERRORS();
    }

    GLuint program = 0;

    //uniforms:
    GLuint FOG_EXP_float = -1U;
    GLuint FOG_INTENSITY_float = -1U;
    GLuint FOG_COLOR_vec3 = -1U;

    //textures:
    //texture0 -- texture to copy
};

Load< DepthEffectsProgram > depth_effects_program(LoadTagEarly, []() -> DepthEffectsProgram const * {
    if (empty_vao == 0) glGenVertexArrays(1, &empty_vao);
    return new DepthEffectsProgram();
});

void Framebuffers::add_depth_effects(float fog_intensity, float fog_exp, glm::vec3 fog_color) {
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glBindFramebuffer(GL_FRAMEBUFFER, depth_effect_fb);

    glUseProgram(depth_effects_program->program);
    glBindVertexArray(empty_vao);

    //set up uniforms
    glUniform1f(depth_effects_program->FOG_INTENSITY_float, fog_intensity);
    glUniform1f(depth_effects_program->FOG_EXP_float, fog_exp);
    glUniform3fv(depth_effects_program->FOG_COLOR_vec3, 1, glm::value_ptr(fog_color));

    //bind color texture to tex0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screen_texture);

    //bind multisampled depth texture to tex1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, ms_depth_tex);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindVertexArray(0);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDepthMask(GL_TRUE);

    GL_ERRORS();
}
