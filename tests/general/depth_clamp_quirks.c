/*
 * Copyright © 2019 Collabora, Ltd.
 *
 * SPDX-License-Identifier: MIT
 *
 * Authors:
 *    Erik Faye-Lund <erik.faye-lund@collabora.com>
 */

/** @file depth_clamp_quirks.c
 * Tests ARB_depth_clamp functionality by drawing side-by-side triangles,
 * lines, points, and raster images that go behind the near plane, and
 * testing that when DEPTH_CLAMP is enabled they get rasterized as they should.
 *
 * An extension of this test would be to test that the depth values are
 * correctly clamped to the near/far plane, not just unclipped, and to test
 * the same operations against the far plane.
 */

#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_core_version = 32;

	config.window_visual = PIGLIT_GL_VISUAL_RGB | PIGLIT_GL_VISUAL_DOUBLE | PIGLIT_GL_VISUAL_DEPTH;

PIGLIT_GL_TEST_CONFIG_END

static GLint
prog1, prog2, prog3, prog4;

static GLint
color_loc;

void
piglit_init(int argc, char **argv)
{
	GLint projection_loc;
	piglit_require_extension("GL_ARB_depth_clamp");

	prog1 = piglit_build_simple_program(
		"uniform mat4 projection;\n"
		"attribute vec4 piglit_vertex;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = projection * piglit_vertex;\n"
		"}\n",
		"void main()\n"
		"{\n"
		"  gl_FragColor = vec4(vec3(gl_FragCoord.z != clamp(gl_FragCoord.z, 0.0, 1.0)), 1.0);\n"
		"}\n");
	glUseProgram(prog1);
	projection_loc = glGetUniformLocation(prog1, "projection");
	piglit_ortho_uniform(projection_loc, piglit_width, piglit_height);

	prog2 = piglit_build_simple_program(
		"uniform mat4 projection;\n"
		"attribute vec4 piglit_vertex;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = projection * piglit_vertex;\n"
		"}\n",
		"void main()\n"
		"{\n"
		"  gl_FragDepth = -1.0;\n"
		"  gl_FragColor = vec4(1.0);\n"
		"}\n");
	glUseProgram(prog2);
	projection_loc = glGetUniformLocation(prog2, "projection");
	piglit_ortho_uniform(projection_loc, piglit_width, piglit_height);

	prog3 = piglit_build_simple_program(
		"uniform mat4 projection;\n"
		"attribute vec4 piglit_vertex;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = projection * piglit_vertex;\n"
		"}\n",
		"uniform vec4 color;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = vec4(color);\n"
		"}\n");
	glUseProgram(prog3);
	projection_loc = glGetUniformLocation(prog3, "projection");
	color_loc = glGetUniformLocation(prog3, "color");
	piglit_ortho_uniform(projection_loc, piglit_width, piglit_height);

	prog4 = piglit_build_simple_program_multiple_shaders(
		GL_VERTEX_SHADER,
		"uniform mat4 projection;\n"
		"attribute vec4 piglit_vertex;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = projection * piglit_vertex;\n"
		"}\n",
		GL_GEOMETRY_SHADER,
		"#version 150\n"
		"layout(triangles) in;\n"
		"layout(triangle_strip, max_vertices = 3) out;\n"
		"void main()\n"
		"{\n"
		"  for (int i = 0; i < gl_in.length(); i++) {\n"
		"    gl_Position = gl_in[i].gl_Position;\n"
		"    EmitVertex();\n"
		"  }\n"
		"  EndPrimitive();\n"
		"}\n",
		GL_FRAGMENT_SHADER,
		"void main()\n"
		"{\n"
		"  gl_FragColor = vec4(1.0);\n"
		"}\n",
		0);
	glUseProgram(prog4);
	projection_loc = glGetUniformLocation(prog4, "projection");
	piglit_ortho_uniform(projection_loc, piglit_width, piglit_height);
}

enum piglit_result
piglit_display(void)
{
	GLboolean pass = GL_TRUE;
	float white[3] = {1.0, 1.0, 1.0};
	float black[3] = {0.0, 0.0, 0.0};
	static const float quad1[4][4] = {
		{ 10, 10, 2, 1 },
		{ 30, 10, 2, 1 },
		{ 10, 30, -2, 1 },
		{ 30, 30, -2, 1 },
	};

	static const float quad2[4][4] = {
		{ 40, 10, 2, 1 },
		{ 60, 10, 2, 1 },
		{ 40, 30, -2, 1 },
		{ 60, 30, -2, 1 }
	};

	static const float quad3[4][4] = {
		{ 15, 40, 2, 1 },
		{ 25, 40, 2, 1 },
		{ 15, 60, -2, 1 },
		{ 25, 60, -2, 1 },
	};

	static const float quad4[4][4] = {
		{ 10, 45, 2, 1 },
		{ 30, 45, -2, 1 },
		{ 10, 55, 2, 1 },
		{ 30, 55, -2, 1 },
	};

	static const float quad5[4][4] = {
		{ 40, 40, 2, 1 },
		{ 60, 40, 2, 1 },
		{ 40, 60, -2, 1 },
		{ 60, 60, -2, 1 }
	};

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_CLAMP);

	glUseProgram(prog1);
	piglit_draw_rect_from_arrays(quad1, NULL, false, 1);

	glUseProgram(prog2);
	piglit_draw_rect_from_arrays(quad2, NULL, false, 1);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glUseProgram(prog3);
	glDepthRange(0.0, 0.75);
	glUniform4f(color_loc, 1, 1, 1, 1);
	piglit_draw_rect_from_arrays(quad3, NULL, false, 1);
	glDepthRange(0.25, 1.0);
	glUniform4f(color_loc, 1, 0, 0, 1);
	piglit_draw_rect_from_arrays(quad4, NULL, false, 1);
	glDisable(GL_DEPTH_TEST);
	glDepthRange(0.0, 1.0);

	glUseProgram(prog4);
	piglit_draw_rect_from_arrays(quad5, NULL, false, 1);

	/* verify that gl_FragCoord.z didn't get clamped */
	pass = piglit_probe_rect_rgb(10, 10, 20, 5, white) && pass;
	pass = piglit_probe_rect_rgb(10, 15, 20, 10, black) && pass;
	pass = piglit_probe_rect_rgb(10, 25, 20, 5, white) && pass;

	/* verify that writing gl_FragDepth doesn't cause clipping */
	pass = piglit_probe_rect_rgb(40, 10, 20, 20, white) && pass;

	/* verify that glDepthRange has an effect */
	pass = piglit_probe_pixel_rgb(20, 50, white) && pass;

	/* verify that clamping still happens with geometry shader */
	pass = piglit_probe_rect_rgb(40, 40, 20, 20, white) && pass;

	piglit_present_results();

	return pass ? PIGLIT_PASS : PIGLIT_FAIL;
}
