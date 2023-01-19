/*
 * Copyright © 2019 Intel Corporation
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

#include "piglit-util-egl.h"
#include "piglit-util-gl.h"

/**
 * @file egl-ext_egl_image_storage.c
 */

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_es_version = 30;

PIGLIT_GL_TEST_CONFIG_END

/* dummy */
enum piglit_result
piglit_display(void)
{
	return PIGLIT_FAIL;
}

static bool
verify_rgbw_texture()
{
	int width, height;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

	float *expect = piglit_rgbw_image(GL_RGBA, width, height, true,
					  GL_UNSIGNED_NORMALIZED);
	unsigned hf = width / 2;
	unsigned color_stride = hf * 4; // one color width in image
	unsigned box = color_stride * hf; // one color box

	float *r = expect;
	float *g = expect + color_stride;
	float *b = expect + 2 * box;
	float *w = b + color_stride;

	bool pass = true;

	/* Verify texture contents by probing each color box. */
	pass = piglit_probe_texel_rect_rgba(GL_TEXTURE_2D, 0, 0, 0, hf, hf, r) && pass;
	pass = piglit_probe_texel_rect_rgba(GL_TEXTURE_2D, 0, hf, 0, hf, hf, g) && pass;
	pass = piglit_probe_texel_rect_rgba(GL_TEXTURE_2D, 0, 0, hf, hf, hf, b) && pass;
	pass = piglit_probe_texel_rect_rgba(GL_TEXTURE_2D, 0, hf, hf, hf, hf, w) && pass;

	free(expect);
	return pass;
}

void
test_invalid_params(EGLImageKHR egl_image)
{
	const int none_attr[] = { GL_NONE };
	const int some_attr = GL_ONE;

	/* "If <attrib_list> is neither NULL nor a pointer to the value
	 * GL_NONE, the error INVALID_VALUE is generated."
	 */
	glEGLImageTargetTexStorageEXT(GL_TEXTURE_2D, egl_image, &some_attr);
	if (!piglit_check_gl_error(GL_INVALID_VALUE)) {
		piglit_report_result(PIGLIT_FAIL);
	}

	/* "If <image> is NULL, the error INVALID_VALUE is generated." */
	glEGLImageTargetTexStorageEXT(GL_TEXTURE_2D, 0x0, none_attr);
	if (!piglit_check_gl_error(GL_INVALID_VALUE)) {
		piglit_report_result(PIGLIT_FAIL);
	}

	/* "If the GL is unable to specify a texture object using the supplied
	 * eglImageOES <image> (if, for example, <image> refers to a
	 * multisampled eglImageOES, or <target> is GL_TEXTURE_2D but <image>
	 * contains a cube map), the error INVALID_OPERATION is generated.
	 */
	glEGLImageTargetTexStorageEXT(GL_TEXTURE_3D, egl_image, none_attr);
	if (!piglit_check_gl_error(GL_INVALID_OPERATION)) {
		piglit_report_result(PIGLIT_FAIL);
	}
}

static bool
test_invalid_target_dmabuf(EGLDisplay dpy)
{
	/*
	 * EXT_EGL_image_storage
	 *
	 * If the EGL image was created using EGL_EXT_image_dma_buf_import,
	 * then the following applies:
	 *
	 *    - <target> must be GL_TEXTURE_2D or GL_TEXTURE_EXTERNAL_OES.
	 *    Otherwise, the error INVALID_OPERATION is generated.
	 */

	if (!piglit_is_egl_extension_supported(dpy,
			"EGL_MESA_image_dma_buf_export") ||
			!piglit_is_egl_extension_supported(dpy,
			"EGL_EXT_image_dma_buf_import")) {
		piglit_logi("dma_buf export and/or import ext not available\n");
		return true;
	}

	int w = 32, h = 32;

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_3D, tex);
	glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA8, w, h, 1);

	EGLImage img = eglCreateImage(dpy, eglGetCurrentContext(),
		EGL_GL_TEXTURE_3D, (EGLClientBuffer)(uintptr_t)tex, NULL);
	if (!img) {
		piglit_loge("failed to create EGLImage\n");
		return false;
	}
	glDeleteTextures(1, &tex);

	PFNEGLEXPORTDMABUFIMAGEQUERYMESAPROC dmabuf_query;
	PFNEGLEXPORTDMABUFIMAGEMESAPROC dmabuf_export;

	dmabuf_query =
		(PFNEGLEXPORTDMABUFIMAGEQUERYMESAPROC) eglGetProcAddress(
			"eglExportDMABUFImageQueryMESA");
	dmabuf_export =
		(PFNEGLEXPORTDMABUFIMAGEMESAPROC) eglGetProcAddress(
			"eglExportDMABUFImageMESA");

	if (!dmabuf_query || !dmabuf_export) {
		piglit_loge("could not find extension entrypoints\n");
		return false;
	}

	int fourcc = -1;
	int num_planes = -1;
	EGLuint64KHR modifier;
	int fd = -1;
	EGLint stride = -1;
	EGLint offset = -1;

	/* Query the image properties, verify fourcc and num planes. */
	if (!dmabuf_query(dpy, img, &fourcc, &num_planes, &modifier) ||
			!piglit_check_egl_error(EGL_SUCCESS)) {
		piglit_loge("failed to query EGLImage dmabuf params\n");
		return false;
	}

	/* Export the image, verify success. */
	if (!dmabuf_export(dpy, img, &fd, &stride, &offset) ||
			!piglit_check_egl_error(EGL_SUCCESS)) {
		piglit_loge("failed to export EGLImage dmabuf\n");
		return false;
	}

	eglDestroyImage(dpy, img);
	
	const EGLAttrib attrs[] = {
		EGL_IMAGE_PRESERVED, EGL_TRUE,
		EGL_WIDTH, w,
		EGL_HEIGHT, h,
		EGL_LINUX_DRM_FOURCC_EXT, fourcc,
		EGL_DMA_BUF_PLANE0_FD_EXT, fd,
		EGL_DMA_BUF_PLANE0_OFFSET_EXT, offset,
		EGL_DMA_BUF_PLANE0_PITCH_EXT, stride,
		EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, modifier,
		EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, modifier >> 32,
		EGL_NONE, EGL_NONE
	};

	img = eglCreateImage(eglGetCurrentDisplay(), EGL_NO_CONTEXT,
		EGL_LINUX_DMA_BUF_EXT, (EGLClientBuffer)0, attrs);

	if (!piglit_check_egl_error(EGL_SUCCESS)) {
		piglit_loge("failed to import dmabuf EGLImage\n");
		return false;
	}

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_3D, tex);
	glEGLImageTargetTexStorageEXT(GL_TEXTURE_3D, img, NULL);

	bool ret = piglit_check_gl_error(GL_INVALID_OPERATION);

	eglDestroyImage(dpy, img);
	glDeleteTextures(1, &tex);

	return ret;
}

void
piglit_init(int argc, char **argv)
{
	piglit_require_extension("GL_EXT_EGL_image_storage");

	PFNEGLCREATEIMAGEKHRPROC peglCreateImageKHR = NULL;
	peglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)
		eglGetProcAddress("eglCreateImageKHR");
	if (!peglCreateImageKHR) {
		fprintf(stderr, "eglCreateImageKHR missing\n");
		piglit_report_result(PIGLIT_SKIP);
        }

	PFNEGLDESTROYIMAGEKHRPROC peglDestroyImageKHR = NULL;
	peglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC)
		eglGetProcAddress("eglDestroyImageKHR");
	if (!peglDestroyImageKHR) {
		fprintf(stderr, "eglDestroyImageKHR missing\n");
		piglit_report_result(PIGLIT_SKIP);
        }

	/* Require EGL_MESA_platform_surfaceless extension. */
	const char *exts = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
	if (!strstr(exts, "EGL_MESA_platform_surfaceless"))
		piglit_report_result(PIGLIT_SKIP);

	EGLint major, minor;
	EGLDisplay dpy;

	dpy = piglit_egl_get_default_display(EGL_PLATFORM_SURFACELESS_MESA);

	if (!eglInitialize(dpy, &major, &minor))
		piglit_report_result(PIGLIT_FAIL);

	piglit_require_egl_extension(dpy, "EGL_MESA_configless_context");

	EGLint ctx_attr[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	EGLContext ctx =
		eglCreateContext(dpy, EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT,
				 ctx_attr);
	if (ctx == EGL_NO_CONTEXT) {
		fprintf(stderr, "could not create EGL context\n");
		piglit_report_result(PIGLIT_FAIL);
	}

	eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx);

	/* Create a texture. */
	GLuint texture_a = piglit_rgbw_texture(GL_RGBA, 256, 256, true, true,
					       GL_UNSIGNED_BYTE);
	glBindTexture(GL_TEXTURE_2D, texture_a);

	/* Create EGLImage from texture.  */
	EGLint attribs[] = { EGL_NONE };
	EGLImageKHR egl_image;
	egl_image = peglCreateImageKHR(dpy, ctx, EGL_GL_TEXTURE_2D,
				       (EGLClientBuffer) (intptr_t) texture_a,
				       attribs);
	if (!egl_image) {
		fprintf(stderr, "failed to create ImageKHR\n");
		piglit_report_result(PIGLIT_FAIL);
	}

	/* Create another texture. */
	GLuint texture_b;
	glGenTextures(1, &texture_b);
	glBindTexture(GL_TEXTURE_2D, texture_b);

	const int none_attr[] = { GL_NONE };

	/* Specify texture from EGLImage, invalid parameters. */
	test_invalid_params(egl_image);

	/* Specify texture from EGLImage, valid params.  */
	glEGLImageTargetTexStorageEXT(GL_TEXTURE_2D, egl_image, none_attr);
	if (!piglit_check_gl_error(GL_NO_ERROR))
		piglit_report_result(PIGLIT_FAIL);

	if (!verify_rgbw_texture())
		piglit_report_result(PIGLIT_FAIL);

	/* Expected to be immutable. */
	GLint immutable_format;
	glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_IMMUTABLE_FORMAT,
			    &immutable_format);
	if (immutable_format != 1)
		piglit_report_result(PIGLIT_FAIL);

	/* If OES_texture_view supported, attempt to create a view. */
	if (piglit_is_extension_supported("GL_OES_texture_view")) {
		GLuint texture_c;
		glGenTextures(1, &texture_c);
		glTextureViewOES(texture_c, GL_TEXTURE_2D, texture_b, GL_RGBA,
			         0, 1, 0, 1);
		if (!piglit_check_gl_error(GL_NO_ERROR)) {
			fprintf(stderr, "failed to create textureview!\n");
			piglit_report_result(PIGLIT_FAIL);
		}

		glDeleteTextures(1, &texture_c);
	}

	glDeleteTextures(1, &texture_a);
	glDeleteTextures(1, &texture_b);
	peglDestroyImageKHR(dpy, egl_image);

	if (!test_invalid_target_dmabuf(dpy)) {
		fprintf(stderr, "failed to test dma-buf storage\n");
		piglit_report_result(PIGLIT_FAIL);
	}

	piglit_report_result(PIGLIT_PASS);
}
