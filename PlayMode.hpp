#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <map>

// For text shaping and rendering
#include <ft2build.h>
#include FT_FREETYPE_H  

#include <hb.h>
#include <hb-ft.h>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//for shaping text
	hb_buffer_t *hb_buffer;
	hb_font_t *hb_font;

	//for rendering text
	FT_Library ft_library;
	FT_Face ft_face;
	FT_Error ft_error;

	//for displaying text, taken from https://learnopengl.com/in-Practice/text-rendering
	struct Character {
		unsigned int texture_id;  // ID handle of the glyph texture
		glm::ivec2 size; // Size of glyph
		glm::ivec2 bearing; // Offset from baseline to left/top of glyph
		glm::dvec2 offset; // Offset of the current glyph
		glm::dvec2 advance; // advance to the next glyph
	};
	std::map<hb_codepoint_t, Character> Characters;

	unsigned int VAO, VBO;

	void render_text(std::string text, double x, double y, glm::vec3 color, float screen_width, float screen_height);

	//----- game state -----

};
