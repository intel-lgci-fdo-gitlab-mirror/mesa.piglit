/* Copyright (c) 2024 Collabora Ltd
 * SPDX-License-Identifier: MIT
 */

/**
 * \file
 * \brief Tests that the driver can cope with a single resource backing the
 * UBO and SSBO bindings within the fragment stage of a single draw.
 */

#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_es_version = 31;

	config.window_visual = PIGLIT_GL_VISUAL_RGB | PIGLIT_GL_VISUAL_DOUBLE;

PIGLIT_GL_TEST_CONFIG_END

static const char vs_triangle_text[] =
	"#version 310 es\n"
	"\n"
	"void main() {\n"
	"	float x = float((0x2 >> gl_VertexID) & 1);\n"
	"	float y = float((0x4 >> gl_VertexID) & 1);\n"
	"	gl_Position = vec4(x, y, 0.0, 1.0);\n"
	"}\n";

static const char fs_with_store[] =
	"#version 310 es\n"
	"\n"
	"layout(binding = 0) uniform UBO { int src; };\n"
	"layout(binding = 1) coherent buffer SSBO { int dst; };\n"
	"\n"
	"layout(location = 0) out mediump vec4 color;\n"
	"\n"
	"void main() {\n"
	"	dst = src + 100;\n"
	"	color = vec4(1.0);\n"
	"};\n";

static GLint prog;

enum piglit_result
piglit_display(void)
{
	uint32_t src = 42;
	uint32_t dst_expectation = src + 100;

	GLuint buffer;
	glGenBuffers(1, &buffer);

	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, 512, NULL, GL_STATIC_DRAW);

	/* UBO part: src = 1 */
	glBufferSubData(GL_ARRAY_BUFFER, 0, 4, &src);

	/* SSBO part: dst = 0  */
	glBufferSubData(GL_ARRAY_BUFFER, 256, 4, NULL);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glUseProgram(prog);

	glBindBufferRange(GL_UNIFORM_BUFFER, 0, buffer, 0, 256);
	glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, buffer, 256, 256);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	uint32_t *dst = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 256, 4, GL_MAP_READ_BIT);

	int pass = *dst == dst_expectation;

	glDeleteBuffers(1, &buffer);

	piglit_present_results();

	return pass ? PIGLIT_PASS : PIGLIT_FAIL;
}

void
piglit_init(int argc, char **argv)
{
	prog = piglit_build_simple_program(vs_triangle_text, fs_with_store);
}
