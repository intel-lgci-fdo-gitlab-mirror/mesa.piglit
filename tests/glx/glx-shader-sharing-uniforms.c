/*
 * Copyright (c) 2010 VMware, Inc.
 * Copyright (c) 2025 Valve Corporation
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
 *
 * Create two GLX contexts with shared shaders.  Update the uniform value in
 * different contexts and make sure the correct value is used during drawing.
 *
 * Tests for mesa bug:
 * https://gitlab.freedesktop.org/mesa/mesa/-/issues/14129
 */

#include "piglit-util-gl.h"
#include "piglit-glx-util.h"

int piglit_width = 50, piglit_height = 50;

static const char *test_name = "glx-shader-sharing-uniforms";

static Display *dpy;
static Window win;
static XVisualInfo *visinfo;

static const char *vert_shader_text =
   "void main() \n"
   "{ \n"
   "   gl_Position = ftransform(); \n"
   "} \n";

static const char *frag_shader_text =
   "uniform vec4 c;\n"
   "void main() \n"
   "{ \n"
   "   gl_FragColor = c; \n"
   "} \n";

static void
check_error(int line)
{
   GLenum e = glGetError();
   if (e)
      printf("GL Error 0x%x at line %d\n", e, line);
}

static bool
draw_and_probe(float clear_colour, bool draw_red, unsigned ctx_num,
               const GLuint program) {
   const GLfloat red[3] = {1.0F, 0.0F, 0.0F};
   const GLfloat green[3] = {0.0F, 1.0F, 0.0F};

   glClearColor(clear_colour, clear_colour, clear_colour, 0.0);
   glClear(GL_COLOR_BUFFER_BIT);

   if (draw_red)
      glUniform4f(glGetUniformLocation(program, "c"), 1.0f, 0.0f, 0.0f, 1.0f);
   else
      glUniform4f(glGetUniformLocation(program, "c"), 0.0f, 1.0f, 0.0f, 1.0f);

   piglit_draw_rect(10, 10, piglit_width - 20, piglit_height - 20);
   check_error(__LINE__);

   int ok = piglit_probe_pixel_rgb(piglit_width / 2, piglit_height / 2,
                                   draw_red ? red : green);
   if (!ok) {
      printf("%s: drawing with context %d %s failed\n", test_name, ctx_num,
             draw_red ? "red" : "green");
      return false;
   }

   return true;
}

enum piglit_result
draw(Display *dpy)
{
   GLXContext ctx1 = piglit_get_glx_context(dpy, visinfo);
   GLXContext ctx2 = piglit_get_glx_context_share(dpy, visinfo, ctx1);

   if (!ctx1 || !ctx2) {
      fprintf(stderr, "%s: create contexts failed\n", test_name);
      piglit_report_result(PIGLIT_FAIL);
   }

   /*
    * Bind first context, make some shaders, draw something.
    */
   glXMakeCurrent(dpy, win, ctx1);

   piglit_dispatch_default_init(PIGLIT_DISPATCH_GL);

   if (piglit_get_gl_version() < 20) {
      printf("%s: Requires OpenGL 2.0\n", test_name);
      return PIGLIT_SKIP;
   }

   glClearColor(1.0, 0.0, 0.0, 1.0);
   glClear(GL_COLOR_BUFFER_BIT);

   const GLuint program =
      piglit_build_simple_program(vert_shader_text, frag_shader_text);
   glUseProgram(program);
   check_error(__LINE__);

   piglit_ortho_projection(piglit_width, piglit_height, GL_FALSE);

   if (!draw_and_probe(0.1, true, 1, program))
      return PIGLIT_FAIL;

   if (!draw_and_probe(0.2, false, 1, program))
      return PIGLIT_FAIL;

   glXSwapBuffers(dpy, win);

   /*
    * Update uniforms and draw with second context
    */
   glXMakeCurrent(dpy, win, ctx2);

   glUseProgram(program);
   check_error(__LINE__);

   piglit_ortho_projection(piglit_width, piglit_height, GL_FALSE);

   if (!draw_and_probe(0.3, false, 2, program))
      return PIGLIT_FAIL;

   if (!draw_and_probe(0.4, true, 2, program))
      return PIGLIT_FAIL;

   glXSwapBuffers(dpy, win);

   glXMakeCurrent(dpy, win, ctx1);

   /* Draw with red in ctx1 which was the last colour ctx2 updated the uniform
    * with. ctx1 last set the uniform to green so if gl does not flush the
    * uniform correctly it will incorrectly continue to draw green.
    */
   if (!draw_and_probe(0.5, true, 1, program))
      return PIGLIT_FAIL;

   glXSwapBuffers(dpy, win);

   /*
    * Destroy contexts
    */
   glXDestroyContext(dpy, ctx1);
   glXDestroyContext(dpy, ctx2);

   return PIGLIT_PASS;
}


int
main(int argc, char **argv)
{
   for(int i = 1; i < argc; ++i) {
      if (strcmp(argv[i], "-auto") == 0)
         piglit_automatic = 1;
      else
         fprintf(stderr, "%s bad option: %s\n", test_name, argv[i]);
   }

   dpy = XOpenDisplay(NULL);
   if (dpy == NULL) {
      fprintf(stderr, "%s: open display failed\n", test_name);
      piglit_report_result(PIGLIT_FAIL);
   }

   visinfo = piglit_get_glx_visual(dpy);
   win = piglit_get_glx_window(dpy, visinfo);

   piglit_glx_event_loop(dpy, draw);

   return 0;
}
