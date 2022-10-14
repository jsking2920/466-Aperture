/*
 * All text rendering code based on the following sources:
 * https://freetype.org/freetype2/docs/tutorial/step1.html
 * https://freetype.org/freetype2/docs/tutorial/example1.c
 * https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
 * https://learnopengl.com/In-Practice/Text-Rendering
 * https://github.com/15-466/15-466-f22-base1/blob/main/PPU466.cpp
 * https://github.com/15-466/15-466-f22-base4
 */

#include "TextRenderer.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"
#include "glm/ext.hpp"

#include <iostream>

TextRenderer::TextRenderer(std::string font_file, uint8_t font_size)
{
    // Initialization based on: https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
    // Initialize FreeType and create FreeType font face
    FT_Error ft_error;

    ft_error = FT_Init_FreeType(&ft_library);
    if (ft_error) throw std::runtime_error("FT_Init failed!");
    ft_error = FT_New_Face(ft_library, font_file.c_str(), 0, &ft_face);
    if (ft_error) throw std::runtime_error("FT_New_Face failed!");
    ft_error = FT_Set_Char_Size(ft_face, 0, font_size * 64, 0, 0);
    if (ft_error) throw std::runtime_error("FT_Set_Char_Size failed!");

    hb_font = hb_ft_font_create(ft_face, NULL);

    // From: https://learnopengl.com/In-Practice/Text-Rendering
    // For each ASCII 0-128 character, we generate a texture and store its relevant data into a Character struct that we add to the Characters map
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

        for (uint8_t c = 0; c < 128; c++) {
            // load character glyph 
            if (FT_Load_Char(ft_face, c, FT_LOAD_RENDER)) {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }
            // generate buffer
            GLuint texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                ft_face->glyph->bitmap.width,
                ft_face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                ft_face->glyph->bitmap.buffer
            );
            // set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // now store character for later use
            Character character = {
                texture,
                glm::ivec2(ft_face->glyph->bitmap.width, ft_face->glyph->bitmap.rows),
                glm::ivec2(ft_face->glyph->bitmap_left, ft_face->glyph->bitmap_top),
                ft_face->glyph->advance.x
            };
            Characters.insert(std::pair<uint8_t, Character>(c, character));
        }
    }

    // From: https://learnopengl.com/In-Practice/Text-Rendering
    // Create a VBO and VAO for rendering the quads
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    // From: https://learnopengl.com/In-Practice/Text-Rendering && https://github.com/15-466/15-466-f22-base1/blob/main/PPU466.cpp
    // Compile shader for rendering text
    {
        text_shaders = gl_compile_program(
            // Vertex shader:
            "#version 330 core\n"
            "layout (location = 0) in vec4 vertex; \n"
            "out vec2 TexCoords;\n"

            "uniform mat4 projection;\n"

            "void main() {\n"
            "   gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
            "	TexCoords = vertex.zw;\n"
            "}\n"
            ,
            // Fragment shader:
            "#version 330 core\n"
            "in vec2 TexCoords;\n"
            "out vec4 color;\n"

            "uniform sampler2D text;\n"
            "uniform vec3 textColor;\n"
            "void main() {\n"
            "vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\n"
            "color = vec4(textColor, 1.0) * sampled;\n"
            "}\n"
        );
    }
}

TextRenderer::~TextRenderer() {
    if (hb_buffer) hb_buffer_destroy(hb_buffer);
    hb_font_destroy(hb_font);
    FT_Done_Face(ft_face);
    FT_Done_FreeType(ft_library);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}

void TextRenderer::draw(std::string text, float x, float y, float scale, glm::vec3 color, float window_width, float window_height) {

    // If rendering new text, reshape with HarfBuzz (note: would break everything if the first time this was called text == cur_text)
    if (text.compare(cur_text) != 0) {
        
        cur_text = text;
        
        // Based on: https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
        // Destroy and recreate hb-buffer with new text
        if (hb_buffer) {
            hb_buffer_destroy(hb_buffer);
        }
        hb_buffer = hb_buffer_create();
        
        hb_buffer_add_utf8(hb_buffer, text.c_str(), -1, 0, -1);
        hb_buffer_guess_segment_properties(hb_buffer);

        // Shape it
        hb_shape(hb_font, hb_buffer, NULL, 0);

        // Get glyph information and positions out of the buffer
        info = hb_buffer_get_glyph_infos(hb_buffer, NULL); // Not being used for anything currently but it probably should be?
        pos = hb_buffer_get_glyph_positions(hb_buffer, NULL);
    }

    // Based on: https://learnopengl.com/In-Practice/Text-Rendering && https://github.com/15-466/15-466-f22-base1/blob/main/PPU466.cpp
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set shader uniforms/attributes
    glUseProgram(text_shaders);
    glUniform3f(glGetUniformLocation(text_shaders, "textColor"), color.x, color.y, color.z);
    glm::mat4 projection = glm::ortho(0.0f, window_width, 0.0f, window_height); // Orthographic projection matrix
    glUniformMatrix4fv(glGetUniformLocation(text_shaders, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // Based on: https://learnopengl.com/In-Practice/Text-Rendering && https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
    // iterate through all characters
    uint32_t i = 0;
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        // Shaping stuff from HarfBuzz
        float x_advance = pos[i].x_advance / 64.0f;
        float y_advance = pos[i].y_advance / 64.0f;
        float x_offset = pos[i].x_offset / 64.0f;
        float y_offset = pos[i].y_offset / 64.0f;
        i++;

        // FreeType stuff taking into account shaping info
        float xpos = x + ((x_offset + ch.Bearing.x) * scale);
        float ypos = y + ((y_offset - (ch.Size.y - ch.Bearing.y)) * scale);
        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels) (using advances from HarfBuzz)
        x += x_advance * scale;
        y += y_advance * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    GL_ERRORS();
}

std::string TextRenderer::format_time(float seconds) {

    std::string sec = std::to_string(int(seconds) % 60);
    std::string fractional_sec = std::to_string(int(seconds * 100) % 100);

    if (seconds >= 60.0f) {
        std::string minutes = std::to_string(int(seconds) / 60);
        return minutes + ":" + sec + "." + fractional_sec;
    }
    return sec + "." + fractional_sec;
}
