/*
 * Copyright (c) 2010 VMware, Inc.
 * Copyright (c) 2025 Valve Corporation.
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
 * Tests FBO rendering interactions with GL_EXT_texture_integer attachments
 * specifically making sure that color attachments are ignored when not
 * active draw buffers. Mesa bugs:
 *    https://gitlab.freedesktop.org/mesa/mesa/-/issues/13144
 *    https://gitlab.freedesktop.org/mesa/mesa/-/issues/13168
 */

#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN
	config.supports_gl_compat_version = 10;

	config.window_visual = PIGLIT_GL_VISUAL_RGB | PIGLIT_GL_VISUAL_DOUBLE;
	config.khr_no_error_support = PIGLIT_NO_ERRORS;
	config.window_width = 256;
	config.window_height = 256;
PIGLIT_GL_TEST_CONFIG_END

#define PAD 10
#define SIZE 20

/* size of texture/renderbuffer (power of two) */
#define FBO_SIZE 64

static const char *TestName = "texture-integer-attachments";

struct format_info {
   GLenum IntFormat, BaseFormat;
   GLuint BitsPerChannel;
   GLboolean Signed;
};


static const struct format_info Formats[] = {
   /*   { "GL_RGBA", GL_RGBA, GL_RGBA, 8, GL_FALSE },*/
   { GL_RGBA8I_EXT,   GL_RGBA_INTEGER_EXT, 8,  GL_TRUE  },
   { GL_RGBA8UI_EXT , GL_RGBA_INTEGER_EXT, 8,  GL_FALSE },
   { GL_RGBA16I_EXT,  GL_RGBA_INTEGER_EXT, 16, GL_TRUE  },
   { GL_RGBA16UI_EXT, GL_RGBA_INTEGER_EXT, 16, GL_FALSE },
   { GL_RGBA32I_EXT,  GL_RGBA_INTEGER_EXT, 32, GL_TRUE  },
   { GL_RGBA32UI_EXT, GL_RGBA_INTEGER_EXT, 32, GL_FALSE },

   { GL_RGB8I_EXT,   GL_RGB_INTEGER_EXT, 8,  GL_TRUE  },
   { GL_RGB8UI_EXT , GL_RGB_INTEGER_EXT, 8,  GL_FALSE },
   { GL_RGB16I_EXT,  GL_RGB_INTEGER_EXT, 16, GL_TRUE  },
   { GL_RGB16UI_EXT, GL_RGB_INTEGER_EXT, 16, GL_FALSE },
   { GL_RGB32I_EXT,  GL_RGB_INTEGER_EXT, 32, GL_TRUE  },
   { GL_RGB32UI_EXT, GL_RGB_INTEGER_EXT, 32, GL_FALSE },
};

#define NUM_FORMATS  (sizeof(Formats) / sizeof(Formats[0]))

static GLenum
get_datatype(const struct format_info *info)
{
   switch (info->BitsPerChannel) {
   case 8:
      return info->Signed ? GL_BYTE : GL_UNSIGNED_BYTE;
   case 16:
      return info->Signed ? GL_SHORT : GL_UNSIGNED_SHORT;
   case 32:
      return info->Signed ? GL_INT : GL_UNSIGNED_INT;
   default:
      assert(0);
      return 0;
   }
}

static GLboolean
check_error(const char *file, int line)
{
   GLenum err = glGetError();
   if (err) {
      fprintf(stderr, "%s: error 0x%x at %s:%d\n", TestName, err, file, line);
      return GL_TRUE;
   }
   return GL_FALSE;
}

static GLboolean
verify_color_rect(int start_x, int start_y, int w, int h)
{
   float red[] =   {1, 0, 0, 0};
   float green[] = {0, 1, 0, 0};
   float blue[] =  {0, 0, 1, 0};
   float white[] = {1, 1, 1, 0};

   if (!piglit_probe_rect_rgb(start_x, start_y, w / 2, h / 2, red))
      return GL_FALSE;
   if (!piglit_probe_rect_rgb(start_x + w/2, start_y, w/2, h/2, green))
      return GL_FALSE;
   if (!piglit_probe_rect_rgb(start_x, start_y + h/2, w/2, h/2, blue))
      return GL_FALSE;
   if (!piglit_probe_rect_rgb(start_x + w/2, start_y + h/2, w/2, h/2, white))
      return GL_FALSE;

   return GL_TRUE;
}

static void
draw_color_rect(int x, int y, int w, int h)
{
   int x1 = x;
   int x2 = x + w / 2;
   int y1 = y;
   int y2 = y + h / 2;

   glColor4f(1.0, 0.0, 0.0, 0.0);
   piglit_draw_rect(x1, y1, w / 2, h / 2);
   glColor4f(0.0, 1.0, 0.0, 0.0);
   piglit_draw_rect(x2, y1, w / 2, h / 2);
   glColor4f(0.0, 0.0, 1.0, 0.0);
   piglit_draw_rect(x1, y2, w / 2, h / 2);
   glColor4f(1.0, 1.0, 1.0, 0.0);
   piglit_draw_rect(x2, y2, w / 2, h / 2);
}

static GLboolean
create_texture(GLenum internal_format, GLenum base_format, GLenum type,
               GLuint *texObj)
{
   int fbo_width = FBO_SIZE;
   int fbo_height = FBO_SIZE;

   /* Create texture */
   glGenTextures(1, texObj);
   glBindTexture(GL_TEXTURE_2D, *texObj);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

   glTexImage2D(GL_TEXTURE_2D, 0, internal_format, fbo_width, fbo_height, 0,
                base_format, type, NULL);

   if (check_error(__FILE__, __LINE__))
      return GL_FALSE;

   GLint f;
   glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &f);
   assert(f == internal_format);

   return GL_TRUE;
}

static void
copy(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
     GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1)
{
   GLsizei srcW = srcX1 - srcX0, srcH = srcY1 - srcY0;
   GLsizei dstW = dstX1 - dstX0, dstH = dstY1 - dstY0;
   void *buf = malloc(srcW * srcH * 4);
   glReadPixels(srcX0, srcY0, srcW, srcH,
                GL_RGBA, GL_UNSIGNED_BYTE, buf);
   glPixelZoom((float) dstW / (float) srcW,
               (float) dstH / (float) srcH);
   glWindowPos2i(dstX0, dstY0);
   glDrawPixels(srcW, srcH, GL_RGBA, GL_UNSIGNED_BYTE, buf);
   free(buf);
}

/** \return GL_TRUE for pass, GL_FALSE for fail */
static GLboolean
test_fbo(const struct format_info *info)
{
   GLuint fbo, texObj, texObj2;
   GLenum status;
   GLboolean intMode;
   GLint buf;

   int fbo_width = FBO_SIZE;
   int fbo_height = FBO_SIZE;
   int x0 = PAD;
   int y0 = PAD;
   int y1 = PAD * 2 + SIZE;

   glViewport(0, 0, piglit_width, piglit_height);
   piglit_ortho_projection(piglit_width, piglit_height, GL_FALSE);

   glClearColor(0.5, 0.5, 0.5, 0.5);
   glClear(GL_COLOR_BUFFER_BIT);

   /* Create regular texture for drawing */
   if (!create_texture(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, &texObj))
      return GL_FALSE;

   /* Create integer texture */
   const GLenum type = get_datatype(info);
   if (!create_texture(info->IntFormat, info->BaseFormat, type, &texObj2))
      return GL_FALSE;

   /* Create FBO to render to texture */
   glGenFramebuffers(1, &fbo);
   glBindFramebuffer(GL_FRAMEBUFFER, fbo);
   glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                          GL_TEXTURE_2D, texObj, 0);

   /* Attach integer color attachment without making it a draw buffer */
   glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT,
                          GL_TEXTURE_2D, texObj2, 0);

   if (check_error(__FILE__, __LINE__))
      return GL_FALSE;

   status = glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
   if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      fprintf(stderr, "%s: failure: framebuffer incomplete.\n", TestName);
      return GL_FALSE;
   }

   glGetBooleanv(GL_RGBA_INTEGER_MODE_EXT, &intMode);
   if (check_error(__FILE__, __LINE__))
      return GL_FALSE;
   if (intMode) {
      fprintf(stderr, "%s: GL_RGBA_INTEGER_MODE_EXT returned GL_TRUE\n",
              TestName);
      return GL_FALSE;
   }

   glGetIntegerv(GL_READ_BUFFER, &buf);
   assert(buf == GL_COLOR_ATTACHMENT0_EXT);
   glGetIntegerv(GL_DRAW_BUFFER, &buf);
   assert(buf == GL_COLOR_ATTACHMENT0_EXT);

   glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, fbo);
   glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, piglit_winsys_fbo);
   glViewport(0, 0, fbo_width, fbo_height);
   piglit_ortho_projection(fbo_width, fbo_height, GL_FALSE);
   glClearColor(1.0, 0.0, 1.0, 0.0);
   glClear(GL_COLOR_BUFFER_BIT);

   /* Test fixed-function drawing while we have an integer fbo attached but
    * not active.
    */

   /* Draw the color rect in the FBO */
   draw_color_rect(x0, y0, SIZE, SIZE);

   /* Now that we have correct samples, blit things around. */
   glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, piglit_winsys_fbo);
   glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, fbo);
   copy(x0, y0, x0 + SIZE, y0 + SIZE,
        x0, y1, x0 + SIZE, y1 + SIZE);

   if (!verify_color_rect(PAD, y0, SIZE, SIZE))
      return GL_FALSE;

   /* Test fixed-function drawing while we have an integer fbo attached and
    * active. This should result in a validation errors.
    */
   glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, fbo);
   glDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
   glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, piglit_winsys_fbo);
   copy(x0, y0, x0 + SIZE, y0 + SIZE,
        x0, y1, x0 + SIZE, y1 + SIZE);

   /* The copy above should trigger a validation error in glDrawPixels */
   if (!piglit_check_gl_error(GL_INVALID_OPERATION))
      return GL_FALSE;

   glBegin(GL_LINES);
   glEnd(GL_LINES);
   /* Validation errors are triggered on both the Begin and End calls. So we
    * need to clear them out before the next loop iteration.
    */
   if (!piglit_check_gl_error(GL_INVALID_OPERATION) &&
       !piglit_check_gl_error(GL_INVALID_OPERATION))
      return GL_FALSE;

   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, piglit_winsys_fbo);

   piglit_present_results();

   glDeleteTextures(1, &texObj);
   glDeleteTextures(1, &texObj2);
   glDeleteFramebuffers(1, &fbo);

   return GL_TRUE;
}

enum piglit_result
piglit_display(void)
{
   for (int f = 0; f < NUM_FORMATS; f++) {
      GLboolean pass = test_fbo(&Formats[f]);
      if (!pass)
         return PIGLIT_FAIL;
   }
   return PIGLIT_PASS;
}

void
piglit_init(int argc, char **argv)
{
   piglit_require_extension("GL_ARB_framebuffer_object");
   piglit_require_extension("GL_EXT_texture_integer");

   piglit_ortho_projection(piglit_width, piglit_height, GL_FALSE);

   (void) check_error(__FILE__, __LINE__);
}
