/* Copyright 2017 Grigori Goronzy <greg@chown.ath.cx>
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
#include "piglit-util.h"
#include "piglit-util-gl.h"
#include "piglit-glx-util.h"
#include "common.h"

#define BOOLSTR(x) ((x) ? "yes" : "no")

#ifndef GLX_CONTEXT_OPENGL_NO_ERROR_ARB
#define GLX_CONTEXT_OPENGL_NO_ERROR_ARB 0x31B3
#endif

static enum piglit_result check_no_error(bool debug, bool robust)
{
	int ctx_flags = 0;
	ctx_flags |= debug ? GLX_CONTEXT_DEBUG_BIT_ARB : 0;
	ctx_flags |= robust ? GLX_CONTEXT_ROBUST_ACCESS_BIT_ARB : 0;
	const int attribs[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, 2,
		GLX_CONTEXT_MINOR_VERSION_ARB, 0,
		GLX_CONTEXT_OPENGL_NO_ERROR_ARB, 1,
		GLX_CONTEXT_FLAGS_ARB, ctx_flags,
		None
	};
	static bool is_dispatch_init = false;
	GLXContext ctx = NULL;
	enum piglit_result result = PIGLIT_SKIP;

	printf("info: debug=%s, robustness=%s\n", BOOLSTR(debug), BOOLSTR(robust));

	if (robust && !piglit_is_glx_extension_supported(dpy, "GLX_ARB_create_context_robustness")) {
		printf("info: GLX_ARB_create_context_robustness not supported\n");
		result = PIGLIT_SKIP;
		goto done;
	}

	ctx = glXCreateContextAttribsARB(dpy, fbconfig, NULL, True, attribs);
	XSync(dpy, 0);

	/* KHR_no_error doesn't allow the no error mode to be enabled
	 * with KHR_debug or ARB_robustness, so context creation is
	 * expected to fail in these cases.
	 *
	 * From the ARB_create_context_no_error spec:
	 *
	 *    "BadMatch is generated if the GLX_CONTEXT_OPENGL_NO_ERROR_ARB is TRUE at
	 *    the same time as a debug or robustness context is specified."
	 */
	if (debug || robust) {
		if (ctx) {
			printf("error: context creation should have failed\n");
			result = PIGLIT_FAIL;
			goto done;
		}
		if (!validate_glx_error_code(BadMatch, -1)) {
			printf("error: incorrect context creation error code\n");
			result = PIGLIT_FAIL;
			goto done;
		}
		result = PIGLIT_PASS;
		goto done;
	} else {
		if (!ctx) {
			printf("error: context creation failed\n");
			result = PIGLIT_FAIL;
			goto done;
		}
		if (!glXMakeContextCurrent(dpy, glxWin, glxWin, ctx)) {
			printf("error: created OpenGL context, but could not make it "
			       "current\n");
			result = PIGLIT_FAIL;
			goto done;
		}
	}

	if (!is_dispatch_init) {
		/* We must postpone initialization of piglit-dispatch until
		 * a context is current.
		 */
		piglit_dispatch_default_init(PIGLIT_DISPATCH_GL);
		is_dispatch_init = true;
	}

	if (!piglit_is_extension_supported("GL_KHR_no_error")) {
		printf("warning: context does not report GL_KHR_no_error "
		       "availability\n");
		result = PIGLIT_WARN;
		goto done;
	}

	if (piglit_get_gl_version() >= 30) {
		GLint context_flags = 0;
		glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
		if (!(context_flags & GL_CONTEXT_FLAG_NO_ERROR_BIT_KHR)) {
			printf("error: context does not have "
			       "GL_CONTEXT_FLAG_NO_ERROR_BIT_KHR set\n");
			result = PIGLIT_FAIL;
			goto done;
		}
	}

	result = PIGLIT_PASS;
done:
	printf("info: %s\n", piglit_result_to_string(result));
	glXMakeContextCurrent(dpy, None, None, NULL);
	glXDestroyContext(dpy, ctx);
	return result;
}

static enum piglit_result check_no_error_shared_context()
{
	const int attribs[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, 2,
		GLX_CONTEXT_MINOR_VERSION_ARB, 0,
		GLX_CONTEXT_OPENGL_NO_ERROR_ARB, 1,
		GLX_CONTEXT_FLAGS_ARB, 0,
		None
	};

	const int attribs2[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, 2,
		GLX_CONTEXT_MINOR_VERSION_ARB, 0,
		GLX_CONTEXT_OPENGL_NO_ERROR_ARB, 0,
		GLX_CONTEXT_FLAGS_ARB, 0,
		None
	};

	static bool is_dispatch_init = false;
	GLXContext ctx;
	enum piglit_result result = PIGLIT_SKIP;

	printf("info: no error attrib mismatch\n");

	ctx = glXCreateContextAttribsARB(dpy, fbconfig, NULL, True, attribs);
	XSync(dpy, 0);

	if (glx_error_code != -1) {
		/* Most likely the API/version is not supported. */
		result = PIGLIT_SKIP;
		goto done;
	}
	if (ctx == NULL) {
		printf("error: context creation failed\n");
		result = PIGLIT_FAIL;
		goto done;
	}

	if (!glXMakeContextCurrent(dpy, glxWin, glxWin, ctx)) {
		printf("error: created OpenGL context, but could not make it "
		       "current\n");
		result = PIGLIT_FAIL;
		goto done;
	}

	if (!is_dispatch_init) {
		/* We must postpone initialization of piglit-dispatch until
		 * a context is current.
		 */
		piglit_dispatch_default_init(PIGLIT_DISPATCH_GL);
		is_dispatch_init = true;
	}

	if (!piglit_is_extension_supported("GL_KHR_no_error")) {
		printf("warning: context does not report GL_KHR_no_error "
		       "availability\n");
		result = PIGLIT_WARN;
		goto done;
	}

	if (piglit_get_gl_version() >= 30) {
		GLint context_flags = 0;
		glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
		if (!(context_flags & GL_CONTEXT_FLAG_NO_ERROR_BIT_KHR)) {
			printf("error: context does not have "
			       "GL_CONTEXT_FLAG_NO_ERROR_BIT_KHR set\n");
			result = PIGLIT_FAIL;
			goto done;
		}
	}

	glXCreateContextAttribsARB(dpy, fbconfig, ctx, True, attribs2);
	if (validate_glx_error_code(BadMatch, -1)) {
		/* From the ARB_create_context_no_error spec:
		 *
		 *    "BadMatch is generated if the value of GLX_CONTEXT_OPENGL_NO_ERROR_ARB
		 *    used to create <share_context> does not match the value of
		 *    GLX_CONTEXT_OPENGL_NO_ERROR_ARB for the context being created."
		 */
		printf("info: context creation failed (expected)\n");
		result = PIGLIT_PASS;
		goto done;
	}

	result = PIGLIT_FAIL;
done:
	printf("info: %s\n", piglit_result_to_string(result));
	glXMakeContextCurrent(dpy, None, None, NULL);
	glXDestroyContext(dpy, ctx);
	return result;
}

int main(int argc, char **argv)
{
	enum piglit_result result = PIGLIT_SKIP;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-auto"))
			piglit_automatic = 1;
		else
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
	}

	GLX_ARB_create_context_setup();
	piglit_require_glx_extension(dpy, "GLX_ARB_create_context_no_error");

	/* Check that KHR_no_error gets enabled and its interaction with debug and
	 * robustness context flags. */
	piglit_merge_result(&result, check_no_error(false, false));
	piglit_merge_result(&result, check_no_error(true, false));
	piglit_merge_result(&result, check_no_error(false, true));
	piglit_merge_result(&result, check_no_error(true, true));

	piglit_merge_result(&result, check_no_error_shared_context());

	GLX_ARB_create_context_teardown();

	piglit_report_result(result);
}
