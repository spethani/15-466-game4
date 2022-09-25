#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

// For text
#define FONT_SIZE 36
#define MARGIN (FONT_SIZE * .5)

#include "TextRenderingProgram.hpp"

PlayMode::PlayMode() {
	{// Creating a VBO and VAO for rendering the quads, taken from https://learnopengl.com/in-Practice/text-rendering
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
		glDisable(GL_DEPTH_TEST);

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
	{ // Perform text shaping, rendering
		// Shaping code referenced from https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
		std::string fontfile = data_path("OpenSans-Medium.ttf");
		std::string text = "Hello World!";

		// Initialize FreeType and create the font
		FT_Library ft_library;
		FT_Face ft_face;
		FT_Error ft_error;
		ft_error = FT_Init_FreeType(&ft_library); // freetype library init
		if (ft_error) std::cerr << "Failed to init FreeType" << std::endl;
		ft_error = FT_New_Face(ft_library, fontfile.c_str(), 0, &ft_face); // create new face object
		if (ft_error) std::cerr << "Failed create font face" << std::endl;
		ft_error = FT_Set_Char_Size(ft_face, FONT_SIZE*64, FONT_SIZE*64, 0, 0); // set current pixel size
		if (ft_error) std::cerr << "Failed to set char size" << std::endl;
		
		// Create the font
		hb_font_t *hb_font;
		hb_font = hb_ft_font_create (ft_face, NULL);

		// Create buffer and populate
		hb_buffer = hb_buffer_create();
		hb_buffer_add_utf8(hb_buffer, text.c_str(), -1, 0, -1);
		hb_buffer_guess_segment_properties(hb_buffer);

		// Perform the shaping
  		hb_shape(hb_font, hb_buffer, NULL, 0);
		
		// Get glyph information and positions out of the buffer.
		unsigned int len = hb_buffer_get_length(hb_buffer);
		hb_glyph_info_t *info = hb_buffer_get_glyph_infos(hb_buffer, NULL);
		//hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(hb_buffer, NULL);

		// Get glyph ids and create textures
		std::vector<Character> characters;
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
		for (unsigned int i = 0; i < len; i++) {
			hb_codepoint_t gid = info[i].codepoint;

			// Rendering taken from https://freetype.org/freetype2/docs/tutorial/step1.html
			ft_error = FT_Load_Glyph(ft_face, gid, 0); // Load glyph with FreeType
			if (ft_error) std::cerr << "Failed to load glyph" << std::endl;
			ft_error = FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL); // Render glyph into bitmap
			if (ft_error) std::cerr << "Failed to render glyph" << std::endl;

			if (Characters.find(gid) == Characters.end()) { // Check if we have texture for codepoint
				// Code taken from https://learnopengl.com/in-Practice/text-rendering
				// generate texture
				unsigned int texture;
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
					(unsigned int)ft_face->glyph->advance.x
				};
				Characters.insert(std::pair<hb_codepoint_t, Character>(gid, character));
			}
		}
		// Clean up resources
		/*FT_Done_FreeType(ft_library);
		FT_Done_Face(ft_face);
		hb_buffer_destroy(hb_buffer);*/
	}
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	return false;
}

void PlayMode::update(float elapsed) {
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	// Orthographic projection matrix allows us to specify all vertex coordinates in screen coordinates
	glm::mat4 projection = glm::ortho(0.0f, (float)drawable_size.x, 0.0f, (float)drawable_size.y);
	glUniformMatrix4fv(glGetUniformLocation(text_rendering_program->program, "projection"), 1, GL_FALSE, &projection[0][0]); // set projection value in shader to be our matrix

	// Display text
	render_text(25.0f, 25.0f, 1.0f, glm::vec3(0.5, 0.8f, 0.2f));
	GL_ERRORS();
}

void PlayMode::render_text(float x, float y, float scale, glm::vec3 color) { // Base code taken from https://learnopengl.com/in-Practice/text-rendering
    // Activate corresponding render state	
    glUseProgram(text_rendering_program->program);
    glUniform3f(glGetUniformLocation(text_rendering_program->program, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // Iterate through all characters
	unsigned int len = hb_buffer_get_length(hb_buffer);
	hb_glyph_info_t *info = hb_buffer_get_glyph_infos(hb_buffer, NULL);
    for (unsigned int i = 0; i < len; i++) {
		hb_codepoint_t gid = info[i].codepoint;
        Character ch = Characters[gid];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

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
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}