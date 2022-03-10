/*
 * Copyright 2022 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file point-smooth.c
 * Test GL_POINT_SMOOTH by comparing the rendering with a shader based
 * emulation of smoothing
 *
 * @author Arvind Yadav<arvind.yadav@amd.com>
 */

#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_compat_version = 10;

	config.window_width = 400;
	config.window_height = 200;
	config.window_visual = PIGLIT_GL_VISUAL_DOUBLE | PIGLIT_GL_VISUAL_RGBA;
	config.khr_no_error_support = PIGLIT_NO_ERRORS;

PIGLIT_GL_TEST_CONFIG_END

#define POINTSIZE 40
static GLuint prog;

static const char* fShader = "                                              \n\
#version 120                                                                \n\
                                                                            \n\
void main()                                                                 \n\
{                                                                           \n\
   float pointSize = 1/dFdx(gl_PointCoord.x);                               \n\
   float radius = pointSize/2;                                              \n\
                                                                            \n\
   vec2 point = (gl_PointCoord.xy -0.5) * pointSize;                        \n\
                                                                            \n\
   /* point(x, y) distance from centre(0.5, 0.5)*/                          \n\
   float distance = length(point);                                          \n\
                                                                            \n\
   float alpha = clamp(radius - distance, 0.0, 1.0);                        \n\
   if (alpha <= 0.0) return;                                                 \n\
                                                                            \n\
   gl_FragColor = vec4(1.0f, 1.0f, 1.0f , alpha);                           \n\
}";

static void
draw_line()
{
	glBegin(GL_LINES);
	glVertex3f(-0.5f, 0.30f, 0.0f);
	glVertex3f(0.5f, 0.30f, 0.0f);
	glEnd();
}

static void
draw_triangle()
{
	glBegin(GL_TRIANGLES);
	glVertex3f(0.0f, -0.30f, 0.0f);
	glVertex3f(-0.5f, -0.75f, 0.0f);
	glVertex3f(0.5f, -0.75f, 0.0f);
	glEnd();
}

static void
draw_polygon()
{
	glBegin(GL_POLYGON);
	glVertex3f(-0.5f, 0.75f, 0.0f);
	glVertex3f(0.5f, 0.75f, 0.0f);
	glVertex3f(-0.5f, 0.5f, 0.0f);
	glVertex3f(0.5f, 0.5f, 0.0f);
	glEnd();
}



static void
draw_point(bool point_smooth_enabled)
{
	if (!point_smooth_enabled) {
		glEnable(GL_POINT_SPRITE);
		glUseProgram(prog);

	}
	glPointSize(POINTSIZE);

	glBegin(GL_POINTS);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glEnd();

	if (!point_smooth_enabled) {
		glUseProgram(0);
		glDisable(GL_POINT_SPRITE);
	}
}

static void
left_screen_draw()
{
	glViewport(0, 0, piglit_width/2, piglit_height);
	draw_polygon();
	draw_line();
	draw_point(false);
	draw_triangle();
}


static void
right_screen_draw()
{
	glViewport(piglit_width/2, 0, piglit_width/2, piglit_height);
	glEnable(GL_POINT_SMOOTH);

	/* Draw different primitives with POINT_SMOOTH enabled.
	 * Only the POINTS prim should be affected by this state
 	 */
	draw_polygon();
	draw_line();
	draw_point(true);
	draw_triangle();

	glDisable(GL_POINT_SMOOTH);
}

enum piglit_result
piglit_display()
{
	GLboolean pass;

	glClear(GL_COLOR_BUFFER_BIT);

	left_screen_draw();
	right_screen_draw();

	pass = piglit_probe_rect_halves_equal_rgba(0, 0, piglit_width,
						   piglit_height);
	piglit_present_results();
	return pass ? PIGLIT_PASS : PIGLIT_WARN;
}

void
piglit_init(int argc, char **argv)
{
	piglit_require_GLSL_version(120);
	prog = piglit_build_simple_program(NULL, fShader);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
}
