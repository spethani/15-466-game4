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

// For file reading
#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>

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
	{ // Initialize for harfbuzz and freetype
		std::string fontfile = data_path("OpenSans-Medium.ttf");

		// Initialize FreeType and create the font
		ft_error = FT_Init_FreeType(&ft_library); // freetype library init
		if (ft_error) std::cerr << "Failed to init FreeType" << std::endl;
		ft_error = FT_New_Face(ft_library, fontfile.c_str(), 0, &ft_face); // create new face object
		if (ft_error) std::cerr << "Failed create font face" << std::endl;
		ft_error = FT_Set_Char_Size(ft_face, FONT_SIZE*64, FONT_SIZE*64, 0, 0); // set current pixel size
		if (ft_error) std::cerr << "Failed to set char size" << std::endl;
		
		// Create the font
		hb_font = hb_ft_font_create(ft_face, NULL);

		// Create buffer and populate
		hb_buffer = hb_buffer_create();
	}
	{ // Parse story file
		std::string current_line;
		std::ifstream story_file(data_path("story.txt"));
		unsigned int story_index = 0;

		// Read scene
		while (std::getline(story_file, current_line)) {
			StoryScene story_scene;

			// Check index for scene matches file
			unsigned int line_index;
			std::stringstream(current_line) >> line_index;
			if (line_index != story_index) {
				std::cout << "Warning: index for scene in file does not match actual index";
			}

			// Get lines for the scene description
			std::string scene_text = "";
			while (std::getline(story_file, current_line)) {
				if (isdigit(current_line[0])) {
					break;
				}
				else {
					scene_text += current_line;
					scene_text += "\n";
				}
			}
			story_scene.scene_text = scene_text;

			// Get lines for the choices
			std::vector<StoryChoice> story_choices;
			unsigned int transition_index;
			std::string ss_buffer;

			{ 
				StoryChoice story_choice;
				std::string choice_text = "";
				std::stringstream ss(current_line);
				ss >> transition_index;
				while (ss >> ss_buffer) {
					choice_text += ss_buffer + " ";
				}
				story_choice.story_index = transition_index;
				story_choice.choice_text = choice_text;
				story_choices.push_back(story_choice);
			}

			while (std::getline(story_file, current_line)) {
				if (current_line.empty()) {
					break;
				}

				StoryChoice story_choice;
				std::string choice_text = "";
				std::stringstream ss(current_line);
				ss >> transition_index;
				while (ss >> ss_buffer) {
					choice_text += ss_buffer + " ";
				}
				story_choice.story_index = transition_index;
				story_choice.choice_text = choice_text;
				story_choices.push_back(story_choice);
			}

			story_scene.story_choices = story_choices;

			// Add scene to story_scenes
			story_scenes.push_back(story_scene);
			story_index++;
		}
	}
	{ // DEBUG: Print story scenes
		for (StoryScene story_scene : story_scenes) {
			std::cout << story_scene.scene_text << std::endl;
			for (StoryChoice story_choice : story_scene.story_choices) {
				std::cout << story_choice.story_index << " " << story_choice.choice_text << std::endl;
			}
			std::cout << std::endl;
		}
	}
}

PlayMode::~PlayMode() {
	// Clean up resources
	FT_Done_FreeType(ft_library);
	FT_Done_Face(ft_face);
	hb_buffer_destroy(hb_buffer);
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	return false;
}

void PlayMode::update(float elapsed) {
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	// Display text
	render_text("How are you\ndoing today?", 100.0f, 100.0f, glm::vec3(0.5, 0.8f, 0.2f), (float)drawable_size.x, (float)drawable_size.y);
	GL_ERRORS();
}

void PlayMode::render_text(std::string text, double x, double y, glm::vec3 color, float screen_width, float screen_height) {
	double current_x = x;
	double current_y = y;

	/* Below code based off of https://learnopengl.com/in-Practice/text-rendering */ 
    // Activate corresponding render state	
    glUseProgram(text_rendering_program->program);
    glUniform3f(glGetUniformLocation(text_rendering_program->program, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

	// Orthographic projection matrix allows us to specify all vertex coordinates in screen coordinates
	glm::mat4 projection = glm::ortho(0.0f, screen_width, 0.0f, screen_height);
	glUniformMatrix4fv(glGetUniformLocation(text_rendering_program->program, "projection"), 1, GL_FALSE, &projection[0][0]); // set projection value in shader to be our matrix

	// Shape text
	hb_buffer_clear_contents(hb_buffer);
	hb_buffer_add_utf8(hb_buffer, text.c_str(), -1, 0, -1);
	hb_buffer_guess_segment_properties(hb_buffer);
	hb_shape(hb_font, hb_buffer, NULL, 0);

	unsigned int len = hb_buffer_get_length(hb_buffer);
	hb_glyph_info_t *info = hb_buffer_get_glyph_infos(hb_buffer, NULL);
	hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (hb_buffer, NULL);

    // Iterate through all characters in buffer and render/display
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
    for (unsigned int i = 0; i < len; i++) {
		hb_codepoint_t gid = info[i].codepoint;

		if (Characters.find(gid) == Characters.end()) {
			// Get positions for text
			double x_advance = pos[i].x_advance / 64.;
			double y_advance = pos[i].y_advance / 64.;
			double x_offset  = pos[i].x_offset / 64.;
			double y_offset  = pos[i].y_offset / 64.;

			// Render the text, taken from https://freetype.org/freetype2/docs/tutorial/step1.html
			ft_error = FT_Load_Glyph(ft_face, gid, 0); // Load glyph with FreeType
			if (ft_error) std::cerr << "Failed to load glyph" << std::endl;
			ft_error = FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL); // Render glyph into bitmap
			if (ft_error) std::cerr << "Failed to render glyph" << std::endl;

			// Code taken from https://learnopengl.com/in-Practice/text-rendering
			// Generate texture
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
			// Set texture options
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			// Store character for later use
			Character character = {
				texture,
				glm::ivec2(ft_face->glyph->bitmap.width, ft_face->glyph->bitmap.rows),
				glm::ivec2(ft_face->glyph->bitmap_left, ft_face->glyph->bitmap_top),
				glm::dvec2(x_offset, y_offset),
				glm::dvec2(x_advance, y_advance)
			};
			Characters.insert(std::pair<hb_codepoint_t, Character>(gid, character));
		}

		assert(Characters.find(gid) != Characters.end());
        Character ch = Characters[gid];
		
		if (gid == 0) { // New line character
			current_x = x;
			current_y -= ft_face->size->metrics.height / 64.;
		}

        float xpos = (float)(current_x + ch.bearing.x + ch.offset.x);
        float ypos = (float)(current_y - (ch.size.y - ch.bearing.y) + ch.offset.y);

        float w = (float)ch.size.x;
        float h = (float)ch.size.y;
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
        glBindTexture(GL_TEXTURE_2D, ch.texture_id);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		if (gid != 0) { // check that it isn't a newline
			current_x += ch.advance.x;
			current_y += ch.advance.y;
		}
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}