#pragma once 

/*
 * TextRendering -- a shader program for OpenGL text rendering
 * Taken from 15-466-f22-base1 code for PPUTileProgram
 */
#include "Load.hpp"
#include "GL.hpp"

struct TextRenderingProgram {
	TextRenderingProgram();
	~TextRenderingProgram();

	GLuint program = 0;

	//Attribute (per-vertex variable) locations:
	//GLuint Position_vec2 = -1U;
	//GLuint TileCoord_ivec2 = -1U;
	//GLuint Palette_int = -1U;

	//Uniform (per-invocation variable) locations:
	//GLuint OBJECT_TO_CLIP_mat4 = -1U;

	//Textures bindings:
	//TEXTURE0 - the tile table (as a 128x128 R8UI texture)
	//TEXTURE1 - the palette table (as a 4x8 RGBA8 texture)
};

extern Load< TextRenderingProgram > text_rendering_program;