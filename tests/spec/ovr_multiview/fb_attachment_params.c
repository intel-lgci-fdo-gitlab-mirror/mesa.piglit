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
 * Test the reading of framebuffer attachment parameters specified in the
 * OVR_multiview spec.
 */

#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_compat_version = 30;
	config.supports_gl_core_version = 31;
	config.khr_no_error_support = PIGLIT_NO_ERRORS;

PIGLIT_GL_TEST_CONFIG_END

static bool has_layered = false;

static bool
check_attachment_param(GLenum target, GLenum attachment, GLenum pname,
		       GLint expected)
{
	GLint val;
	glGetFramebufferAttachmentParameteriv(target, attachment, pname, &val);
	if (!piglit_check_gl_error(GL_NO_ERROR)) {
		printf("Expected %s %s = %d, got error\n",
		       piglit_get_gl_enum_name(attachment),
		       piglit_get_gl_enum_name(pname),
		       expected);
		return false;
	} else if (val != expected) {
		printf("Expected %s %s = %d, got %d\n",
		       piglit_get_gl_enum_name(attachment),
		       piglit_get_gl_enum_name(pname),
		       expected, val);
		return false;
	}
	return true;
}

static bool
check_attachment_params(GLenum target, GLenum attachment,
			GLint exp_base_view_index, GLint exp_num_views,
			GLint exp_layered, GLint exp_layer)
{
	bool ret = true;

	ret &= check_attachment_param(target, attachment,
				      GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_BASE_VIEW_INDEX_OVR,
				      exp_base_view_index);
	ret &= check_attachment_param(target, attachment,
				      GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_NUM_VIEWS_OVR,
				      exp_num_views);
	if (has_layered) {
		ret &= check_attachment_param(target, attachment,
					      GL_FRAMEBUFFER_ATTACHMENT_LAYERED,
					      exp_layered);
	}
	ret &= check_attachment_param(target, attachment,
				      GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER,
				      exp_layer);
	return ret;
}

void
piglit_init(int argc, char **argv)
{
	GLuint tex_2d, tex_2da, tex_3d, depth_2da, fbo;
	bool ok = true;

	piglit_require_extension("GL_OVR_multiview");

	/* has glFramebufferTexture() */
	has_layered = (piglit_get_gl_version() >= 32 ||
		       piglit_is_extension_supported("GL_ARB_geometry_shader4") ||
		       piglit_is_extension_supported("GL_EXT_geometry_shader4"));

	printf("Has layered FBO attachments = %s\n", has_layered ? "yes" : "no");

	/* generate 2d texture */
	glGenTextures(1, &tex_2d);
	glBindTexture(GL_TEXTURE_2D, tex_2d);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, piglit_width, piglit_height, 0,
		     GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	/* generate 2d array texture */
	glGenTextures(1, &tex_2da);
	glBindTexture(GL_TEXTURE_2D_ARRAY, tex_2da);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB8, piglit_width,
		     piglit_height, 4, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	/* generate 3d array texture */
	glGenTextures(1, &tex_3d);
	glBindTexture(GL_TEXTURE_3D, tex_3d);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB8, piglit_width,
		     piglit_height, 4, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_3D, 0);

	/* generate 2d array texture for depth/stencil */
	glGenTextures(1, &depth_2da);
	glBindTexture(GL_TEXTURE_2D_ARRAY, depth_2da);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH24_STENCIL8, piglit_width,
		     piglit_height, 2, 0, GL_DEPTH_STENCIL,
		     GL_UNSIGNED_INT_24_8, NULL);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	/* generate FBO */
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	if (!piglit_check_gl_error(GL_NO_ERROR))
		ok = false;

	/* set up some attachments */

	/*
	 * COLOR_ATTACHMENT0: non-multiview, 2D
	 * COLOR_ATTACHMENT1: non-multiview, 2D array, 1 layer (1)
	 * COLOR_ATTACHMENT2: non-multiview, 2D array, all layers (0-3)
	 * COLOR_ATTACHMENT3: multiview, 2D array, 1 layer (3)
	 * COLOR_ATTACHMENT4: multiview, 2D array, 2 layers (1-2)
	 * COLOR_ATTACHMENT5: non-multiview, 3D, 1 layer (3)
	 */
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			       GL_TEXTURE_2D, tex_2d, 0);
	glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, tex_2da,
				  0, 1);
	if (has_layered) {
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2,
				     tex_2da, 0);
	}
	glFramebufferTextureMultiviewOVR(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3,
					 tex_2da, 0, 3, 1);
	glFramebufferTextureMultiviewOVR(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4,
					 tex_2da, 0, 1, 2);
	glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT5,
			       GL_TEXTURE_3D, tex_3d, 0, 3);
	if (!piglit_check_gl_error(GL_NO_ERROR))
		ok = false;

	/* read back & validate parameters */

	ok &= check_attachment_params(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				      0, 0, 0, 0);
	ok &= check_attachment_params(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
				      0, 0, 0, 1);
	if (has_layered) {
		ok &= check_attachment_params(GL_FRAMEBUFFER,
					      GL_COLOR_ATTACHMENT2, 0, 0, 1, 0);
	}
	ok &= check_attachment_params(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3,
				      3, 1, 0, 3);
	ok &= check_attachment_params(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4,
				      1, 2, 0, 1);
	ok &= check_attachment_params(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT5,
				      0, 0, 0, 3);

	piglit_report_result(ok ? PIGLIT_PASS : PIGLIT_FAIL);
}

enum piglit_result
piglit_display(void)
{
	/* Should never be reached */
	return PIGLIT_FAIL;
}
