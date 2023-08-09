/*
 * Copyright 2023 Google LLC
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
 * @file refcount-multithread.c
 *
 * Mesa drivers normally use a hash table to make sure there is a 1:1 mapping
 * between gem bo handles and userspace bo handles.  When a gem bo handle is
 * imported, and if a userspace bo handle for the gem bo handle already
 * exists, mesa drivers simply increase the reference count of the userspace
 * bo handle.
 *
 * It occurred to multiple drivers in the past where the bo destroy function
 * raced with the bo import function and led to use-after-free.  This test
 * attempts to catch such a driver bug.
 */

#include <pthread.h>

#include "image_common.h"
#include "sample_common.h"

#define THREAD_COUNT 2
#define THREAD_ITER 100000

struct thread_data {
	pthread_t thread;

	struct piglit_dma_buf *buf;
	int fourcc;

	EGLDisplay dpy;
	EGLContext ctx;
	enum piglit_result result;
};

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_es_version = 20;
	config.window_visual = PIGLIT_GL_VISUAL_RGBA;

PIGLIT_GL_TEST_CONFIG_END

static enum piglit_result
create_and_destroy_texture(struct piglit_dma_buf *buf, int fourcc)
{
	enum piglit_result res;
	EGLImageKHR img;
	GLuint tex;

	res = egl_image_for_dma_buf_fd(buf, buf->fd, fourcc, &img);
	if (res != PIGLIT_PASS)
		return res;

	res = texture_for_egl_image(img, &tex);
	eglDestroyImageKHR(eglGetCurrentDisplay(), img);
	if (res != PIGLIT_PASS)
		return res;

	glDeleteTextures(1, &tex);
	glFinish();

	return res;
}

static void
thread_cleanup(struct thread_data *data)
{
	eglDestroyContext(data->dpy, data->ctx);
}

static enum piglit_result
thread_init(struct thread_data *data)
{
	const EGLint attrs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE,
	};
	data->ctx = eglCreateContext(data->dpy, EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT, attrs);
	if (data->ctx == EGL_NO_CONTEXT)
		return PIGLIT_FAIL;

	if (!eglMakeCurrent(data->dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, data->ctx)) {
		eglDestroyContext(data->dpy, data->ctx);
		return PIGLIT_FAIL;
	}

	return PIGLIT_PASS;
}

static void *
thread_main(void *arg)
{
	struct thread_data *data = arg;

	data->result = thread_init(data);
	if (data->result != PIGLIT_PASS)
		return NULL;

	for (int i = 0; i < THREAD_ITER; i++) {
		data->result = create_and_destroy_texture(data->buf, data->fourcc);
		if (data->result != PIGLIT_PASS)
			break;
	}

	thread_cleanup(data);

	return NULL;
}

/* dummy */
enum piglit_result
piglit_display(void)
{
	return PIGLIT_PASS;
}

void
piglit_init(int argc, char **argv)
{
	EGLDisplay egl_dpy = eglGetCurrentDisplay();

	piglit_require_egl_extension(egl_dpy, "EGL_KHR_image_base");
	piglit_require_egl_extension(egl_dpy, "EGL_EXT_image_dma_buf_import");
	piglit_require_egl_extension(egl_dpy, "EGL_KHR_no_config_context");
	piglit_require_egl_extension(egl_dpy, "EGL_KHR_surfaceless_context");

	enum piglit_result res;

	/* Create a DMABUF */
	const uint32_t src_data[2][2] = { 0 };
	const int fourcc = DRM_FORMAT_ABGR8888;
	struct piglit_dma_buf *buf;
	res = piglit_create_dma_buf(2, 2, fourcc, src_data, &buf);
	if (res != PIGLIT_PASS)
		piglit_report_result(PIGLIT_SKIP);

	/* Make image_common.c resolve the entrypoints */
	res = create_and_destroy_texture(buf, fourcc);
	if (res != PIGLIT_PASS)
		piglit_report_result(PIGLIT_SKIP);

	/* Dissociate the DMABUF from the underlying driver */
	struct piglit_dma_buf local_buf = *buf;
	local_buf.fd = dup(buf->fd);
	piglit_destroy_dma_buf(buf);

	if (local_buf.fd < 0)
		piglit_report_result(PIGLIT_FAIL);

	struct thread_data data[THREAD_COUNT];
	for (int i = 0; i < THREAD_COUNT; i++) {
		data[i].buf = &local_buf;
		data[i].fourcc = fourcc;
		data[i].dpy = egl_dpy;
		data[i].result = PIGLIT_PASS;

		if (pthread_create(&data[i].thread, NULL, thread_main,
					&data[i]))
			piglit_report_result(PIGLIT_FAIL);
	}

	for (int i = 0; i < THREAD_COUNT; i++) {
		pthread_join(data[i].thread, NULL);
		if (data[i].result != PIGLIT_PASS)
			piglit_report_result(data[i].result);
	}

	piglit_report_result(PIGLIT_PASS);
}
