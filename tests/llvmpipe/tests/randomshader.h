/*
 * Copyright © 2012-2021 VMware, Inc.
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

#pragma once

#include "../glsl/shader.h"
#include "../glsl/functions.h"

class RandomShader {
private:
   static const unsigned int swizzle_chance   = 50;
   static const unsigned int multiply_chance  = 50;
   static const unsigned int writemask_chance = 50;

   enum InputType {
      Immediate,
      Uniform,
      Varying,
      Texture,
      NoTextureInputMax = Texture,
      InputMax
   };

public:
   RandomShader()
      : mShader(*(new glsl::Shader())), mProgram(new glsl::Program(NULL, &mShader)),
        mUsesUniform(false), mUsesTexture(false), mUsesVarying(false)
   {
      createFragment();
      createVertex();
   }

   ~RandomShader()
   {
      /* mShader and the vertex shader are deleted by mProgram's destructor. */
      delete mProgram;
   }

   glsl::Program* program()
   {
      return mProgram;
   }

   bool usesTexture()
   {
      return mUsesTexture;
   }

   bool usesUniform()
   {
      return mUsesUniform;
   }

   bool usesVarying()
   {
      return mUsesVarying;
   }

private:
   void createFragment()
   {
      using namespace glsl;

      Operator* color = generateInput();
      if (gRandom.percent() < multiply_chance) {
         color = new BinaryOperator(BinaryOperator::MUL,
                                    color,
                                    generateInput());
      }

      Constant gl_FragColor = mShader["gl_FragColor"];
      if (gRandom.percent() < swizzle_chance) {
         mShader << new BinaryOperator(BinaryOperator::MOV,
                                      new Constant(gl_FragColor, generateSwizzle()),
                                      color);
      } else {
         mShader << new BinaryOperator(BinaryOperator::MOV,
                                      new Constant(gl_FragColor),
                                      color);
      }

      if (gRandom.percent() < writemask_chance) {
         mShader << new BinaryOperator(BinaryOperator::MOV,
                                      new Constant(gl_FragColor, generateWritemask()),
                                      generateInput(1));
      }
   }

   void createVertex()
   {
      using namespace glsl;

      glsl::Shader& vertex = *(new glsl::Shader());

      Constant gl_Color = vertex["gl_Color"];
      Constant gl_Vertex = vertex["gl_Vertex"];
      Constant gl_Position = vertex["gl_Position"];
      Constant gl_TexCoord0 = vertex["gl_TexCoord[0]"];
      Constant gl_FrontColor = vertex["gl_FrontColor"];
      Constant gl_MultiTexCoord0 = vertex["gl_MultiTexCoord0"];
      Constant gl_ModelViewProjectionMatrix = vertex["gl_ModelViewProjectionMatrix"];

      if (usesTexture()) {
         vertex << new BinaryOperator(BinaryOperator::MOV,
                                      new Constant(gl_TexCoord0),
                                      new Constant(gl_MultiTexCoord0));
      }

      if (usesVarying()) {
         vertex << new BinaryOperator(BinaryOperator::MOV,
                                      new Constant(gl_FrontColor),
                                      new Constant(gl_Color));
      }

      vertex << new BinaryOperator(BinaryOperator::MOV,
                                   new Constant(gl_Position),
                                   new BinaryOperator(BinaryOperator::MUL,
                                                      new Constant(gl_ModelViewProjectionMatrix),
                                                      new Constant(gl_Vertex)));

      mProgram->setVertexShader(&vertex);
   }

   glsl::Selector generateWritemask()
   {
      return glsl::Selector((glsl::Selector::Channels)gRandom.random(0, 4));
   }

   glsl::Selector generateSwizzle()
   {
      return glsl::Selector(gRandom.shuffle(glsl::Selector::xyzw.mask()));
   }

   glsl::Operator* generateInput(int channels = 4)
   {
      int rand;
      int max = channels == 4 ? InputMax : NoTextureInputMax;

      do {
         rand = gRandom.random(0, max);
      } while ((mUsesTexture && rand == 2) || (mUsesVarying && rand == 3));

      switch (rand) {
         case Immediate:
         {
            if (channels == 4) {
               return new glsl::Immediate(gRandom.random<Colour>());
            } else {
               return new glsl::Immediate(gRandom.random<float>(0.0f, 1.0f));
            }
         }

         case Uniform:
         {
            mUsesUniform = true;

            glsl::Constant colour = mShader.addUniformVec4("colour");
            return new glsl::Constant(colour, channels == 4 ? glsl::Selector::xyzw : generateWritemask());
         }

         case Varying:
         {
            mUsesVarying = true;

            glsl::Constant gl_Color = mShader["gl_Color"];
            return new glsl::Constant(gl_Color, channels == 4 ? glsl::Selector::xyzw : generateWritemask());
         }

         case Texture:
         {
            mUsesTexture = true;

            glsl::Constant gl_TexCoord0 = mShader["gl_TexCoord[0]"];
            glsl::Constant texture_id = mShader.addUniformSampler2D("texture_id");

            return glsl::texture2D(new glsl::Constant(texture_id),
                                   new glsl::Constant(gl_TexCoord0, glsl::Selector::xy));
         }

         default:
         {
            assert(false);
            return NULL;
         }
      }
   }

private:
   glsl::Shader& mShader;
   glsl::Program* mProgram;

   bool mUsesUniform;
   bool mUsesTexture;
   bool mUsesVarying;
};
