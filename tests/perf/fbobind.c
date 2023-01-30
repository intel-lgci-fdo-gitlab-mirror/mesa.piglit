/*
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VMWARE BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * Measure rate of binding/switching between FBO targets.
 * Create two framebuffer objects for rendering to two textures.
 * Ping pong between texturing from one and drawing into the other.
 *
 * Brian Paul
 * 22 Sep 2009
 */

#include <string.h>
#include "common.h"
#include "piglit-util-gl.h"

#define WINDOW_SIZE 100


PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_compat_version = 10;
	config.window_width = WINDOW_SIZE;
	config.window_height = WINDOW_SIZE;
	config.window_visual = PIGLIT_GL_VISUAL_RGBA | PIGLIT_GL_VISUAL_DOUBLE;

PIGLIT_GL_TEST_CONFIG_END

static GLuint VBO;

static GLuint FBO[2], Tex[2];

static const GLsizei TexSize = 512;

static const GLboolean DrawPoint = GL_TRUE;

struct vertex
{
	GLfloat x, y, s, t;
};

static const struct vertex vertices[1] = {
	{ 0.0, 0.0, 0.5, 0.5 },
};

#define VOFFSET(F) ((void *) offsetof(struct vertex, F))


void
piglit_init(int argc, char **argv)
{
	const GLenum filter = GL_LINEAR;
	GLenum stat;
	int i;

	piglit_require_extension("GL_EXT_framebuffer_object");

	/* setup VBO */
	glGenBuffersARB(1, &VBO);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, VBO);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(vertices),
	                vertices, GL_STATIC_DRAW_ARB);

	glVertexPointer(2, GL_FLOAT, sizeof(struct vertex), VOFFSET(x));
	glTexCoordPointer(2, GL_FLOAT, sizeof(struct vertex), VOFFSET(s));
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glGenFramebuffersEXT(2, FBO);
	glGenTextures(2, Tex);

	for (i = 0; i < 2; i++) {
		/* setup texture */
		glBindTexture(GL_TEXTURE_2D, Tex[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
		             TexSize, TexSize, 0,
		             GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

		/* setup fbo */
		glBindFramebufferEXT(GL_FRAMEBUFFER, FBO[i]);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
		                          GL_COLOR_ATTACHMENT0_EXT,
		                          GL_TEXTURE_2D, Tex[i], 0/*level*/);
		stat = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		if (stat != GL_FRAMEBUFFER_COMPLETE_EXT) {
			printf("fboswitch: Error: incomplete FBO!\n");
			exit(1);
		}

		/* clear the FBO */
		glClear(GL_COLOR_BUFFER_BIT);
	}

	glEnable(GL_TEXTURE_2D);
}


static void
FBOBind(unsigned count)
{
	unsigned i;
	for (i = 1; i < count; i++) {
		const GLuint dst = i & 1;
		const GLuint src = 1 - dst;

		/* bind src texture */
		glBindTexture(GL_TEXTURE_2D, Tex[src]);

		/* bind dst fbo */
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, FBO[dst]);
		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

		/* draw something */
		if (DrawPoint)
			glDrawArrays(GL_POINTS, 0, 1);
	}
	glFinish();
}


enum piglit_result
piglit_display(void)
{
	double rate;

	rate = perf_measure_cpu_rate(FBOBind, 1.0);
	printf("  FBO Binding: %1.f binds/sec\n", rate);

	exit(0);
	return PIGLIT_SKIP;
}
