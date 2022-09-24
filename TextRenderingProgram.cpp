#include "TextRenderingProgram.hpp"

#include "Load.hpp"
#include "GL.hpp"
#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

Load< TextRenderingProgram > text_rendering_program(LoadTagEarly);

TextRenderingProgram::TextRenderingProgram() {
    // Vertex and fragment shaders pulled from https://learnopengl.com/in-Practice/text-rendering
    program = gl_compile_program(
		//vertex shader:
		"#version 330 core\n"
		"layout (location = 0) in vec4 vertex;\n" // <vec2 pos, vec2 tex>
		"out vec2 TexCoords;\n"
		"uniform mat4 projection;\n"
		"void main()\n"
		"{\n"
		"    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
		"    TexCoords = vertex.zw;\n"
		"}\n"
	,

		//fragment shader:
		"#version 330 core\n"
		"in vec2 TexCoords;\n"
		"out vec4 color;\n"
		"uniform sampler2D text;\n"
		"uniform vec3 textColor;\n"
		"void main() {\n"
		"	vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\n"
		"   color = vec4(textColor, 1.0) * sampled;\n"
		"}\n"
	);
}

TextRenderingProgram::~TextRenderingProgram() {
	if (program != 0) {
		glDeleteProgram(program);
		program = 0;
	}
}