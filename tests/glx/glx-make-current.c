/*
 * Copyright ?? Christopher James Halse Rogers <christopher.halse.rogers at canonical.com>
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
 * Authors:
 *    Christopher James Halse Rogers <christopher.halse.rogers at canonical.com>
 *
 */

/** @file glx-make-current.c
 *
 * Test that MakeCurrent can successfully switch a single context between
 * different drawables and back.
 *
 */

#include "piglit-util-gl.h"
#include "piglit-glx-util.h"

int piglit_width = 50, piglit_height = 50;
static Display *dpy;
#define NUM_WINDOWS 3
static Window wins[NUM_WINDOWS];
static XVisualInfo *visinfo;

enum piglit_result
draw(Display *dpy)
{
	GLXContext ctx;
	float color[] = {
		(float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX, 0.0
	};
	GLboolean pass = GL_TRUE;

	ctx = piglit_get_glx_context(dpy, visinfo);
	glXMakeCurrent(dpy, wins[0], ctx);
	piglit_dispatch_default_init(PIGLIT_DISPATCH_GL);
	glClearColor(color[0], color[1], color[2], 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	for (int i = 1; i < NUM_WINDOWS; i++) {
		glXMakeCurrent(dpy, wins[i], ctx);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	for (int i = 0; i < NUM_WINDOWS; i++) {
		glXMakeCurrent(dpy, wins[i], ctx);
		if (!piglit_probe_pixel_rgb(1, 1, color)) {
			printf("  (test failed for window %d)\n", i);
			pass = false;
		}
	}

	for (int i = 0; i < NUM_WINDOWS; i++) {
		glXMakeCurrent(dpy, wins[i], ctx);
		glXSwapBuffers(dpy, wins[i]);
	}

	return pass ? PIGLIT_PASS : PIGLIT_FAIL;
}

int
main(int argc, char **argv)
{
	int i;

	for(i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-auto"))
			piglit_automatic = 1;
		else
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
	}

	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		fprintf(stderr, "couldn't open display\n");
		piglit_report_result(PIGLIT_FAIL);
	}
	visinfo = piglit_get_glx_visual(dpy);
	for (int i = 0; i < NUM_WINDOWS; i++)
		wins[i] = piglit_get_glx_window(dpy, visinfo);

	piglit_glx_event_loop(dpy, draw);

	/* Free our resources when we're done. */
	glXMakeCurrent(dpy, None, NULL);
	glXDestroyContext(dpy, piglit_get_glx_context(dpy, visinfo));

	return 0;
}
