/* Copyright © 2025 Charlotte Pabst for Codeweavers
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

/* Test to make sure that replacing the source of a program
 * works without issues.
 */

#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_compat_version = 10;

	config.window_width = 100;
	config.window_height = 100;
	config.window_visual = PIGLIT_GL_VISUAL_DOUBLE | PIGLIT_GL_VISUAL_RGBA;

PIGLIT_GL_TEST_CONFIG_END

enum piglit_result
piglit_display(void)
{
	static const char *fp_source1 =
		"!!ARBfp1.0\n"
		"MOV result.color, {0, 0, 0, 0};\n"
		"END\n";
	static const char *fp_source2 =
		"!!ARBfp1.0\n"
		"TEMP x;"
		"LRP result.color, fragment.texcoord[0], {0.1, 0.1}, {0.3, 0.3};\n"
		"END\n";
	const float expected1[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	const float expected2[] = { 0.2f, 0.2f, 0.0f, 1.0f };
	GLuint prog;
	bool pass = true;

	glEnable(GL_FRAGMENT_PROGRAM_ARB);
	glGenProgramsARB(1, &prog);
	glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, prog);

	glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB,
				GL_PROGRAM_FORMAT_ASCII_ARB,
				strlen(fp_source1),
				(const GLubyte *) fp_source1);
	piglit_draw_rect_tex(-1, -1, 2, 2, 0, 0, 1, 1);
	pass = pass && piglit_probe_pixel_rgba(piglit_width/2, piglit_height/2, expected1);

	glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB,
				GL_PROGRAM_FORMAT_ASCII_ARB,
				strlen(fp_source2),
				(const GLubyte *) fp_source2);
	piglit_draw_rect_tex(-1, -1, 2, 2, 0, 0, 1, 1);
	pass = pass && piglit_probe_pixel_rgba(piglit_width/2, piglit_height/2, expected2);

	piglit_present_results();

	glDisable(GL_FRAGMENT_PROGRAM_ARB);
	glDeleteProgramsARB(1, &prog);

	return pass ? PIGLIT_PASS : PIGLIT_FAIL;
}

void
piglit_init(int argc, char **argv)
{
	piglit_require_extension("GL_ARB_fragment_program");
}
