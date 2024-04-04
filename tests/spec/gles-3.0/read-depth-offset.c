/*
 * Copyright © 2024 Collabora Ltd
 *
 * Based on read-depth, which has
 * Copyright © 2015 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 */

/** @file read-depth-offset.c
 *
 * Tests glPolygonOffset calculations
 *
 * Test iterates over table of depth buffer formats and expected types to
 * read values back from each format. For each format it renders a rectangle at
 * different depth levels, reads back a pixel and verifies expected depth value.
 *
 * The spec is fairly clear on the calculations for floating point depth
 * buffers, but for fixed point it merely specifies that the value r
 * used for the minimum resolvable difference is "implementation defined".
 * But it must be the same for all depth values, so we can at least
 * check that.
 */

#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_es_version = 30;
	config.window_visual = PIGLIT_GL_VISUAL_DEPTH;

PIGLIT_GL_TEST_CONFIG_END

static GLint prog;

const char *vs_source =
	"attribute vec4 vertex;\n"
	"uniform float depth;\n"
	"void main()\n"
	"{\n"
	"	gl_Position = vec4(vertex.xy, depth, 1.0);\n"
	"}\n";

const char *fs_source =
	"void main()\n"
	"{\n"
	"}\n";

/* this is a fairly arbitrary test for floats being "close" together.
   we don't want to make too many assumptions about how the GPU hardware
   does floating point calculations, so we don't even assume IEEE binary
   compliance
*/
static bool
equals(float a, float b)
{
   return fabs(a - b) < 0.00001;
}

#define DEPTH_BIAS_UNITS 256.0

/*
 * Calculate the expected depth after offset applied.
 * We assume the polygon is facing the screen and
 * that glPolygonOffsetClamp factor is 0.0.
 * For fixed point formats, the minimum resolvable
 * difference in depth values is passed as a
 * pointer; if this value is 0, we do no
 * offsetting and instead return the raw value read
 * (so that the caller can calculate what mrd is
 * being used by the fixed point hardware).
 */

static GLfloat
offset_depth(GLenum type, GLfloat inputZ, double mrd)
{
	GLfloat near = -1.0;
	GLfloat far = 1.0;
	GLfloat zw, offset;
	double work;
	int exp;

	zw = inputZ * (far-near)/2.0 + (far+near)/2.0;

	/* for floats, find the minimum resolvable difference near inputZ */
	if (type == GL_FLOAT) {
		double mant;
		work = (double) zw;
		mant = frexp(work, &exp);
		if (mant == 0)
			mrd = 0;  /* 0 has no exponent, really */
		else {
			mrd = exp2(exp-24);
		}
	}
	offset = (GLfloat)(mrd * DEPTH_BIAS_UNITS);

	zw += offset;

	if (zw < near) zw = near;
	if (zw > far) zw = far;
	return zw;
}

static GLuint
create_depth_fbo(GLenum depth_type)
{
	GLuint fbo, buffer;
	GLenum status;

	glGenRenderbuffers(1, &buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, buffer);
	glRenderbufferStorage(GL_RENDERBUFFER,
		depth_type, piglit_width, piglit_height);

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER,
		GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, buffer);

	status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		fprintf(stderr, "error creating framebuffer, status 0x%x\n",
			status);
		return 0;
	}
	return fbo;
}

static bool
check_depth(GLenum type, float source_depth, double *mrd_ptr)
{
	float expect;
	GLfloat data;
	GLuint uint_pixel;
	double mrd;

	if (type == GL_FLOAT) {
		glReadPixels(0, 0, 1, 1, GL_DEPTH_COMPONENT, type,
			(void *) &data);
	} else {
		glReadPixels(0, 0, 1, 1, GL_DEPTH_COMPONENT, type,
			(void *) &uint_pixel);
		uint_pixel = uint_pixel >> 8;
		data = (1.0 * ((float) uint_pixel)) / 16777215.0;
	}

	if (!piglit_check_gl_error(GL_NO_ERROR))
		return false;

	mrd = *mrd_ptr;
	expect = offset_depth(type, source_depth, mrd);
	/* if we haven't computed the minimum resolvable difference yet,
	   then figure it out here */
	if (type != GL_FLOAT && mrd == 0.0) {
		double delta = fabs(data - source_depth);
		mrd = delta / DEPTH_BIAS_UNITS;
#if 0
		/* useful for debugging failures, but #if'd out for normal usage */
		printf("orig: %a read: %a delta: %a mrd: %a\n", source_depth, data, delta, mrd);
#endif
		*mrd_ptr = mrd;
		expect = data; /* no failure on first time through */
	}

	if (!equals(data, expect)) {
		if (type == GL_FLOAT && source_depth == 0.0 && fabs(data - source_depth)/DEPTH_BIAS_UNITS < 0.000001 ) {
			/* floating point 0.0 upsets some hardware, just accept
			   whatever it gave us */
		} else {
			fprintf(stderr, "%s Z source: %f expected: %f actual: %f\n",
				(type == GL_FLOAT) ? "float" : "fixed",	source_depth, expect, data);
			return false;
		}
	}
	return true;
}

const GLenum tests[] = {
	GL_DEPTH_COMPONENT16, GL_UNSIGNED_INT_24_8_OES,
	GL_DEPTH_COMPONENT24, GL_UNSIGNED_INT_24_8_OES,
	GL_DEPTH_COMPONENT32F, GL_FLOAT,
};

static bool
test_format(GLenum depth_format, GLenum read_type)
{
	const float step = 0.1;
	int steps = (int)round(1.0 / step);
	double mrd = 0.0; /* minimum resolvable difference */
	float expect = 0.0;
	int i;
	bool test_ok;

	GLuint fbo = create_depth_fbo(depth_format);
	if (!fbo)
		return PIGLIT_FAIL;

	/* Step from -1.0 to 1.0, linear depth. Render a rectangle at
	 * depth i, read pixel and verify expected depth value.
	 */
	for (i = -steps; i <= steps; i++) {
		GLfloat depth = (GLfloat)i / steps;
		glDepthRangef(-1.0, 1.0);
		glPolygonOffset(0, DEPTH_BIAS_UNITS);
		glEnable(GL_POLYGON_OFFSET_FILL);

		glClear(GL_DEPTH_BUFFER_BIT);
		glUniform1f(glGetUniformLocation(prog, "depth"), depth);

		piglit_draw_rect(-1, -1, 2, 2);

		test_ok = check_depth(read_type, expect, &mrd);

		glPolygonOffset(0, 0);
		glDisable(GL_POLYGON_OFFSET_FILL);
		glDepthRangef(0.0, 1.0);

		if (!test_ok)
			return false;

		expect += step / 2.0;
	}
	glDeleteFramebuffers(1, &fbo);
	return true;
}

enum piglit_result
piglit_display(void)
{
	unsigned j;

	glEnable(GL_DEPTH_TEST);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	/* Loop through formats listed in 'tests'. */
	for (j = 0; j < ARRAY_SIZE(tests); j += 2) {

		if (!test_format(tests[j], tests[j+1]))
			return PIGLIT_FAIL;
	}
	return PIGLIT_PASS;
}

void
piglit_init(int argc, char **argv)
{
	piglit_require_extension("GL_NV_read_depth");
	prog = piglit_build_simple_program(vs_source, fs_source);
	glUseProgram(prog);
}
