/*
 * Copyright © 2026 Valve Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/**
 * Tests binding both samplers to different texture units on the same texture.
 * This tests a previous double free in Mesa drivers see:
 *    https://gitlab.freedesktop.org/mesa/mesa/-/issues/15045
 */

#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN
	config.supports_gl_core_version = 33;
	config.window_visual = PIGLIT_GL_VISUAL_DOUBLE | PIGLIT_GL_VISUAL_RGBA;
PIGLIT_GL_TEST_CONFIG_END

static GLuint prog;

static const char *vs =
"#version 330\n"
"in vec4 piglit_vertex;\n"
"in vec2 piglit_texcoord;\n"
"out vec2 tc;\n"
"void main() {\n"
"    gl_Position = piglit_vertex;\n"
"    tc = piglit_texcoord;\n"
"}";

static const char *fs =
"#version 330\n"
"in vec2 tc;\n"
"uniform sampler2D decode_tex;\n"
"uniform sampler2D skip_tex;\n"
"uniform bool skip_decode;\n"
"out vec4 color;\n"
"void main() {\n"
"    vec4 a = texture(decode_tex, tc);\n"
"    vec4 b = texture(skip_tex, tc);\n"
"    if (skip_decode)\n"
"       color = b;\n"
"    else\n"
"       color = a;\n"
"}";

enum piglit_result
piglit_display(void)
{
	bool pass = true;
	GLuint tex;
	GLuint sampler_decode;
	GLuint sampler_skip;
	const float tex_data[4] = {0.2, 0.4, 0.6, 0.8};
	float decoded_tex_data[4];

	for (int i = 0; i < 3; i++) {
		decoded_tex_data[i] = piglit_srgb_to_linear(tex_data[i]);
	}
	decoded_tex_data[3] = tex_data[3];

	glClear(GL_COLOR_BUFFER_BIT);

	glGenSamplers(1, &sampler_decode);
	glSamplerParameteri(sampler_decode, GL_TEXTURE_SRGB_DECODE_EXT,
			    GL_DECODE_EXT);

	glGenSamplers(1, &sampler_skip);
	glSamplerParameteri(sampler_skip, GL_TEXTURE_SRGB_DECODE_EXT,
			    GL_SKIP_DECODE_EXT);

	/* Create the texture (sRGB) */
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, 1, 1, 0,
	             GL_RGBA, GL_FLOAT, tex_data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	/* Bind both samplers to different texture units on the same texture */
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);
	glBindSampler(0, sampler_decode);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, tex);
	glBindSampler(1, sampler_skip);

	/* Shader setup */
	glUseProgram(prog);
	glUniform1i(glGetUniformLocation(prog, "decode_tex"), 0);
	glUniform1i(glGetUniformLocation(prog, "skip_tex"), 1);

	/* Test changing the sampler between the left and right side of
	 * the draw.
	 */
	glUniform1i(glGetUniformLocation(prog, "skip_decode"), 0);
	piglit_draw_rect_tex(-1, -1, 1, 2, 0.0f, 0.0f, 1.0f, 1.0f);
	pass = piglit_probe_pixel_rgb(piglit_width / 4, piglit_height / 2, decoded_tex_data);


	glUniform1i(glGetUniformLocation(prog, "skip_decode"), 1);
	piglit_draw_rect_tex(0, -1, 1, 2, 0.0f, 0.0f, 1.0f, 1.0f);
	pass = piglit_probe_pixel_rgb(piglit_width * 3 / 4, piglit_height / 2, tex_data) && pass;

	piglit_present_results();

	glDeleteSamplers(1, &sampler_decode);
	glDeleteSamplers(1, &sampler_skip);
	glDeleteTextures(1, &tex);

	return pass ? PIGLIT_PASS : PIGLIT_FAIL;
}

void piglit_init(int argc, char **argv)
{
    piglit_require_extension("GL_ARB_sampler_objects");
    piglit_require_extension("GL_EXT_texture_sRGB");
    piglit_require_extension("GL_EXT_texture_sRGB_decode");

    prog = piglit_build_simple_program(vs, fs);
}
