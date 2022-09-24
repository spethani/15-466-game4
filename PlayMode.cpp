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
#include <ft2build.h>
#include FT_FREETYPE_H  

#include <hb.h>
#include <hb-ft.h>

#define FONT_SIZE 36
#define MARGIN (FONT_SIZE * .5)

PlayMode::PlayMode() {
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
		hb_buffer_t *hb_buffer;
		hb_buffer = hb_buffer_create();
		hb_buffer_add_utf8(hb_buffer, text.c_str(), -1, 0, -1);
		hb_buffer_guess_segment_properties(hb_buffer);

		// Perform the shaping
  		hb_shape(hb_font, hb_buffer, NULL, 0);
		
		// Get glyph information and positions out of the buffer.
		unsigned int len = hb_buffer_get_length(hb_buffer);
		hb_glyph_info_t *info = hb_buffer_get_glyph_infos(hb_buffer, NULL);
		hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(hb_buffer, NULL);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

		// Get glyph ids and absolute positions
		std::vector<Character> characters;
		double current_x = 0;
		double current_y = 0;
		for (unsigned int i = 0; i < len; i++) {
			hb_codepoint_t gid   = info[i].codepoint;
			double x_position = current_x + pos[i].x_offset / 64.;
			double y_position = current_y + pos[i].y_offset / 64.;

			// Rendering taken from https://freetype.org/freetype2/docs/tutorial/step1.html
			ft_error = FT_Load_Glyph(ft_face, gid, 0); // Load glyph with FreeType
			if (ft_error) std::cerr << "Failed to load glyph" << std::endl;
			ft_error = FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL); // Render glyph into bitmap
			if (ft_error) std::cerr << "Failed to render glyph" << std::endl;
			ft_face->glyph->bitmap_left = (FT_Int)x_position; // position x of glyph
			ft_face->glyph->bitmap_top = (FT_Int)y_position; // position y of glyph

			// Print some info about glyph
			char glyphname[32];
			hb_font_get_glyph_name(hb_font, gid, glyphname, sizeof (glyphname));

			printf("glyph='%s'	glyph id=%d	position=(%g,%g)\n",
				glyphname, gid, x_position, y_position);

			// Update current x and y positions
			current_x += pos[i].x_advance / 64.;
			current_y += pos[i].y_advance / 64.;
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
}