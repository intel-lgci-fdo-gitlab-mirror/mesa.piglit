/*
 * Copyright (C) 2025 James Hogan <james@albanarts.com>
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
 *
 */

/**
 * Test the additional conditions required for framebuffer multiview attachment
 * completeness in the OVR_multiview spec.
 */

#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_compat_version = 30;
	config.supports_gl_core_version = 31;
	config.khr_no_error_support = PIGLIT_NO_ERRORS;

PIGLIT_GL_TEST_CONFIG_END

void
piglit_init(int argc, char **argv)
{
	GLuint tex, fbo;
	GLenum fbstatus;
	bool ok = true;

	piglit_require_extension("GL_OVR_multiview");

	/* generate 2d array texture */
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB8, piglit_width,
		     piglit_height, 4, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	/* generate FBO */
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	if (!piglit_check_gl_error(GL_NO_ERROR))
		piglit_report_result(PIGLIT_FAIL);

	/*
	 * OVR_multiview:
	 * "Add the following to the list of conditions required for framebuffer
	 * attachment completeness in section 9.4.1 (Framebuffer Attachment
	 * Completeness):
	 *
	 * "If <image> is a two-dimensional array and the attachment
	 * is multiview, all the selected layers, [<baseViewIndex>,
	 * <baseViewIndex> + <numViews>), are less than the layer count of the
	 * texture."
	 */

	/* layers 3-4 out of 4: incomplete */
	glFramebufferTextureMultiviewOVR(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
					 tex, 0, 3, 2);
	if (!piglit_check_gl_error(GL_NO_ERROR))
		piglit_report_result(PIGLIT_FAIL);
	fbstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fbstatus != GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT) {
		printf("Line %u (views out of layer range): Expected GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT, got %s\n",
		       __LINE__, piglit_get_gl_enum_name(fbstatus));
		ok = false;
	}

	/* layers 0-1 out of 4: complete */
	glFramebufferTextureMultiviewOVR(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
					 tex, 0, 0, 2);
	if (!piglit_check_gl_error(GL_NO_ERROR))
		piglit_report_result(PIGLIT_FAIL);
	fbstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fbstatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("Line %u (views in range, 0-2): Expected GL_FRAMEBUFFER_COMPLETE, got %s\n",
		       __LINE__, piglit_get_gl_enum_name(fbstatus));
		ok = false;
	}

	/* layers 2-3 out of 4: complete */
	glFramebufferTextureMultiviewOVR(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
					 tex, 0, 2, 2);
	if (!piglit_check_gl_error(GL_NO_ERROR))
		piglit_report_result(PIGLIT_FAIL);
	fbstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fbstatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("Line %u (views in range, 2-3): Expected GL_FRAMEBUFFER_COMPLETE, got %s\n",
		       __LINE__, piglit_get_gl_enum_name(fbstatus));
		ok = false;
	}

	piglit_report_result(ok ? PIGLIT_PASS : PIGLIT_FAIL);
}

enum piglit_result
piglit_display(void)
{
	/* Should never be reached */
	return PIGLIT_FAIL;
}
