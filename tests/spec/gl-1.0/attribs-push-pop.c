/*
 * Copyright (C) 2024 Intel Corporation.
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
 * @file attribs-push-pop.c
 *
 * Test glPushAttrib() and glPopAttrib().
 */

#include "piglit-util-gl.h"


PIGLIT_GL_TEST_CONFIG_BEGIN
      config.supports_gl_compat_version = 10;
      config.khr_no_error_support = PIGLIT_NO_ERRORS;
PIGLIT_GL_TEST_CONFIG_END


enum piglit_result
piglit_display(void)
{
   /* never get here */
   return PIGLIT_PASS;
}

void
set_depth_range(GLdouble near, GLdouble far) {
   glDepthRange(near, far);

   if (!piglit_check_gl_error(GL_NO_ERROR)) {
      fprintf(stderr, "Failed to set depth range\n");
      piglit_report_result(PIGLIT_FAIL);
   }
}

void
expect_depth_range(GLdouble expected_near, GLdouble expected_far)
{
   GLdouble vals[2];

   const GLdouble epsilon = 1e-5;

   glGetDoublev(GL_DEPTH_RANGE, vals);
   if (!piglit_check_gl_error(GL_NO_ERROR)) {
      fprintf(stderr, "Failed to get depth range\n");
      piglit_report_result(PIGLIT_FAIL);
   }

   if (fabs(vals[0] - expected_near) > epsilon ||
       fabs(vals[1] - expected_far) > epsilon) {
      fprintf(stderr, "Expected [%f, %f], got [%f, %f]\n",
              expected_near, expected_far, vals[0], vals[1]);
      piglit_report_result(PIGLIT_FAIL);
   }
}

void
piglit_init(int argc, char **argv)
{
   /* Init value and sanity-check that range even changes */
   set_depth_range(0.1, 0.9);
   expect_depth_range(0.1, 0.9);

   /* Test simplest valid push-pop restore */
   glPushAttrib(GL_VIEWPORT_BIT);
   set_depth_range(0.2, 0.8);
   glPopAttrib();
   expect_depth_range(0.1, 0.9);

   /* Test unrelated state mask not restoring value */
   glPushAttrib(GL_FOG_BIT);
   set_depth_range(0.2, 0.8);
   glPopAttrib();
   expect_depth_range(0.2, 0.8);

   /* Test double-depth push-pop restore with same mask */
   glPushAttrib(GL_VIEWPORT_BIT);
   set_depth_range(0.3, 0.7);
   glPushAttrib(GL_VIEWPORT_BIT);
   set_depth_range(0.4, 0.6);
   glPopAttrib();
   expect_depth_range(0.3, 0.7);
   glPopAttrib();
   expect_depth_range(0.2, 0.8);

   /**
    * Test double-depth, but the masks are different, and value changed
    * inside the "unrelated" scope should be restored by the first scope.
    * Refs https://gitlab.freedesktop.org/mesa/mesa/-/issues/11417
    */
   glPushAttrib(GL_VIEWPORT_BIT);
   glPushAttrib(GL_FOG_BIT);
   set_depth_range(0.3, 0.7);
   glPopAttrib();
   expect_depth_range(0.3, 0.7);
   glPopAttrib();
   expect_depth_range(0.2, 0.8);

   piglit_report_result(PIGLIT_PASS);
}
