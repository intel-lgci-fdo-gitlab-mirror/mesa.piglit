/* Copyright (c) 2024 Collabora Ltd
 * SPDX-License-Identifier: MIT
 */

/**
 * \file
 * \brief Tests that the driver can cope with a single resource backing the
 * UBO and SSBO bindings within a single dispatch.
 */

#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_es_version = 31;

	config.window_visual = PIGLIT_GL_VISUAL_RGB | PIGLIT_GL_VISUAL_DOUBLE;

PIGLIT_GL_TEST_CONFIG_END

static const char cs_with_store[] =
	"#version 310 es\n"
	"\n"
	"layout(binding = 0) uniform UBO { int src; };\n"
	"layout(binding = 1) coherent buffer SSBO { int dst; };\n"
	"\n"
	"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
	"\n"
	"void main() {\n"
	"	dst = src + 100;\n"
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

	glDispatchCompute(1, 1, 1);

	uint32_t *dst = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 256, 4, GL_MAP_READ_BIT);

	int pass = *dst == dst_expectation;

	glDeleteBuffers(1, &buffer);

	piglit_present_results();

	return pass ? PIGLIT_PASS : PIGLIT_FAIL;
}

void
piglit_init(int argc, char **argv)
{
	prog = piglit_build_compute_program(cs_with_store);
}
