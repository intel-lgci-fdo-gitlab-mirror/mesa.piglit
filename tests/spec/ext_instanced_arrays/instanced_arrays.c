/*
 * Copyright 2023 Collabora, Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT.  IN NO EVENT SHALL VMWARE AND/OR THEIR SUPPLIERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file
 * Tests GL_EXT_instanced_arrays
 */

#include "piglit-util-gl.h"
#include "piglit-matrix.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_es_version = 20;

	config.window_width = 500;
	config.window_height = 500;
	config.window_visual = PIGLIT_GL_VISUAL_RGBA | PIGLIT_GL_VISUAL_DOUBLE;
	config.khr_no_error_support = PIGLIT_NO_ERRORS;

PIGLIT_GL_TEST_CONFIG_END

#define NUM_TRIANGLES 10

static const char *VertShaderText =
   "attribute vec3 Vertex; \n"
   "attribute mat4 Matrix; \n"
   "void main() \n"
   "{ \n"
   "   gl_Position = Matrix*vec4(Vertex, 1.0); \n"
   "} \n";

static const char *FragShaderText =
   "void main() \n"
   "{ \n"
   "   gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0); \n"
   "} \n";


static GLuint Program;
static GLuint vbo, mbo;
static GLint VertexAttrib, MatrixAttrib;

enum piglit_result
piglit_display(void)
{
	float white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float transparent[] = { 0.0f, 0.0f, 0.0f, 0.0f };
   float vertices[9] = {-0.1f, -0.1f , 0.0f,
                        0.1f, -0.1f , 0.0f,
                        0.0f,  0.15f, 0.0f};
   glGenBuffers(1, &vbo);
   glBindBuffer(GL_ARRAY_BUFFER, vbo);
   glBufferData(GL_ARRAY_BUFFER, 9*sizeof(float), vertices, GL_STATIC_DRAW);
   glVertexAttribPointer(VertexAttrib, 3, GL_FLOAT, GL_FALSE, (GLsizei)3*sizeof(float), NULL);
   glEnableVertexAttribArray(VertexAttrib);

   float mat[NUM_TRIANGLES * 16];
   for (int matrix_id=0; matrix_id < NUM_TRIANGLES; matrix_id++) {
      float pos_x = cos(2*M_PI*matrix_id / NUM_TRIANGLES);
      float pos_y = sin(2*M_PI*matrix_id / NUM_TRIANGLES);
      int i = 16 * matrix_id;
		piglit_identity_matrix (&mat[i]);
		mat[i+12] = pos_x;
		mat[i+13] = pos_y;
   }
   glGenBuffers(1, &mbo);
   glBindBuffer(GL_ARRAY_BUFFER, mbo);
   glBufferData(GL_ARRAY_BUFFER, NUM_TRIANGLES * 16 * sizeof(float),
                mat, GL_DYNAMIC_DRAW);
   for (unsigned int i = 0; i < 4; i++) {
      glEnableVertexAttribArray(MatrixAttrib + i);
      glVertexAttribPointer(MatrixAttrib + i, 4, GL_FLOAT, GL_FALSE,
                            16 * sizeof(float),
                            (const GLvoid *)(sizeof(GLfloat) * i * 4));
      glVertexAttribDivisorEXT(MatrixAttrib + i, 1);
		if (!piglit_check_gl_error(GL_NO_ERROR)) {
         piglit_present_results();
         return PIGLIT_FAIL;
      }
   }

   glClear(GL_COLOR_BUFFER_BIT);

   glDrawArraysInstancedEXT(GL_TRIANGLES, 0, 3, (GLsizei)NUM_TRIANGLES);
	if (!piglit_check_gl_error(GL_NO_ERROR)) {
      piglit_present_results();
      return PIGLIT_FAIL;
   }

   if (!piglit_probe_pixel_rgba(piglit_width - 1, piglit_height/2, white)) {
      piglit_present_results();
      return PIGLIT_FAIL;
   }

   if (!piglit_probe_pixel_rgba(0, 0, transparent)) {
      piglit_present_results();
      return PIGLIT_FAIL;
   }

   piglit_present_results();

   return PIGLIT_PASS;
}


void
piglit_init(int argc, char **argv)
{
   piglit_require_extension("GL_EXT_instanced_arrays");

   Program = piglit_build_simple_program(VertShaderText, FragShaderText);

   glUseProgram(Program);

   VertexAttrib = glGetAttribLocation(Program, "Vertex");
   MatrixAttrib = glGetAttribLocation(Program, "Matrix");
}
