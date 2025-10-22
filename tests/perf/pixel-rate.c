/*
 * Copyright (C) 2025  Advanced Micro Devices, Inc.
 * All Rights Reserved.
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
 * Measure pixel throughput with different numbers of fragment shader inputs,
 * qualifiers, and system values.
 */

#include "common.h"
#include <stdbool.h>
#undef NDEBUG
#include <assert.h>
#include "piglit-util-gl.h"

/* this must be a power of two to prevent precision issues */
#define WINDOW_SIZE 1024

/* This should be low enough to stay cached, so that we are not limited by bandwidth. */
#define TEST_FBO_SIZE		256
#define TEST_FBO_SAMPLES	8
#define TEST_FBO_SAMPLES_STR	"8"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_compat_version = 10;
	config.window_width = WINDOW_SIZE;
	config.window_height = WINDOW_SIZE;
	config.window_visual = PIGLIT_GL_VISUAL_RGBA | PIGLIT_GL_VISUAL_DOUBLE;

PIGLIT_GL_TEST_CONFIG_END

static unsigned gpu_freq_mhz;

typedef struct {
	const char *name;
	bool sample_shading;
	const char *vs;
	const char *fs;
	GLuint prog;
} prog_info;

#define VS_SET_POSITION "   gl_Position = vec4(gl_VertexID % 2 == 1 ? 1.0 : -1.0, gl_VertexID / 2 == 1 ? 1.0 : -1.0, 0.0, 1.0);\n"

#define VS_POS \
	"#version 400\n" \
	"void main() {\n" \
	VS_SET_POSITION \
	"}\n"

#define FS_OUT(v) \
	"#version 400\n" \
	"void main() {\n" \
	"	gl_FragColor = vec4(" v ");\n" \
	"}"

#define INPUTS_PROG(n, sample_shading, qual, qual_code, linear, linear_code, sysval, sysval_code) \
	{#n " inputs " qual linear sysval, \
	sample_shading, \
	\
	"#version 400\n" \
	"noperspective out vec4 varnp;\n" \
	qual_code " out vec4 var["#n"];\n" \
	"void main() {\n" \
	"	float id = float(gl_VertexID);\n" \
	"	varnp = vec4(id, id*id, id*id*id, id*id*id*id);\n" \
	"	var[0].x = id;\n" \
	"	for (int i = 1; i < ("#n" - (" linear_code " ? 1 : 0)) * 4; i++) var[i / 4][i % 4] = var[(i - 1) / 4][(i - 1) % 4] * id;\n" \
	VS_SET_POSITION \
	"}\n", \
	\
	"#version 400\n" \
	"noperspective in vec4 varnp;\n" \
	qual_code " in vec4 var["#n"];\n" /* the last one is unused, it will be eliminated by the GLSL compiler */ \
	"void main() {\n" \
	"	vec4 v = " linear_code " ? varnp : vec4(1.0);\n" \
	"	for (int i = 0; i < ("#n" - (" linear_code " ? 1 : 0)); i++) v *= var[i];\n" \
	"	gl_FragColor = v + " sysval_code ";\n" \
	"}\n"}

#define INPUTS_Q(n, sample_shading, qual, qual_code, linear, linear_code) \
	INPUTS_PROG(n, sample_shading, qual, qual_code, linear, linear_code,, "vec4(0.0)"), \
	INPUTS_PROG(n, sample_shading, qual, qual_code, linear, linear_code, "+Face", "vec4(gl_FrontFacing)"), \
	INPUTS_PROG(n, sample_shading, qual, qual_code, linear, linear_code, "+SampleMask", "vec4(gl_SampleMaskIn[0])"), \
	INPUTS_PROG(n, sample_shading, qual, qual_code, linear, linear_code, "+FragPos.x", "gl_FragCoord.xxxx"), \
	INPUTS_PROG(n, sample_shading, qual, qual_code, linear, linear_code, "+FragPos.y", "gl_FragCoord.yyyy"), \
	INPUTS_PROG(n, sample_shading, qual, qual_code, linear, linear_code, "+FragPos.z", "gl_FragCoord.zzzz"), \
	INPUTS_PROG(n, sample_shading, qual, qual_code, linear, linear_code, "+FragPos.w", "gl_FragCoord.wwww"), \
	INPUTS_PROG(n, sample_shading, qual, qual_code, linear, linear_code, "+FragPos.xy", "gl_FragCoord.xyyy"), \
	INPUTS_PROG(n, sample_shading, qual, qual_code, linear, linear_code, "+FragPos.zw", "gl_FragCoord.zwww"), \
	INPUTS_PROG(n, sample_shading, qual, qual_code, linear, linear_code, "+FragPos.xyz", "gl_FragCoord.xyzz"), \
	INPUTS_PROG(n, sample_shading, qual, qual_code, linear, linear_code, "+FragPos.xyw", "gl_FragCoord.xyww"), \
	INPUTS_PROG(n, sample_shading, qual, qual_code, linear, linear_code, "+FragPos.xyzw", "gl_FragCoord"), \
	INPUTS_PROG(n, sample_shading, qual, qual_code, linear, linear_code, "+FragPos.xy+Face", "gl_FragCoord.xyyy + vec4(gl_FrontFacing)"), \
	INPUTS_PROG(n, sample_shading, qual, qual_code, linear, linear_code, "+FragPos.xy+SampleMask", "gl_FragCoord.xyyy + vec4(gl_SampleMaskIn[0])"), \
	INPUTS_PROG(n, sample_shading, qual, qual_code, linear, linear_code, "+FragPos.xy+Face+SampleMask", "gl_FragCoord.xyyy + vec4(gl_FrontFacing) + vec4(gl_SampleMaskIn[0])"), \
	INPUTS_PROG(n, sample_shading, qual, qual_code, linear, linear_code, "+FragPos.xyzw+Face+SampleMask", "gl_FragCoord + vec4(gl_FrontFacing) + vec4(gl_SampleMaskIn[0])"), \
	INPUTS_PROG(n, true, qual, qual_code, linear, linear_code, "+SampleID", "vec4(gl_SampleID)"), \
	INPUTS_PROG(n, true, qual, qual_code, linear, linear_code, "+SampleID+FragPos.xy", "vec4(gl_SampleID) + gl_FragCoord.xyyy"), \
	INPUTS_PROG(n, true, qual, qual_code, linear, linear_code, "+SampleID+SampleMask", "vec4(gl_SampleID) + vec4(gl_SampleMaskIn[0])"), \
	INPUTS_PROG(n, true, qual, qual_code, linear, linear_code, "+SampleID+FragPos.xyzw+Face+SampleMask", "vec4(gl_SampleID) + gl_FragCoord + vec4(gl_FrontFacing) + vec4(gl_SampleMaskIn[0])")

#define INPUT1(n) \
	INPUTS_Q(n, false, "flat", "flat", "", "false"), \
	INPUTS_Q(n, false, "persp",, "", "false"), \
	INPUTS_Q(n, true, "sample", "sample", "", "false")

#define INPUTS(n) \
	INPUTS_Q(n, false, "persp",, "", "false"), \
	INPUTS_Q(n, false, "persp",, "+linear", "true"), \
	INPUTS_Q(n, true, "sample", "sample", "", "false"), \
	INPUTS_Q(n, true, "sample", "sample", "+linear", "true")

static prog_info progs[] = {
	{"Empty", false, VS_POS,
	 "#version 400\n"
	 "void main() {\n"
	 "}"},

	{"Empty+discard", false, VS_POS,
	 "#version 400\n"
	 "void main() {\n"
	 "	discard;\n"
	 "}"},

	{"Const fill", false, VS_POS, FS_OUT("0.5, 0.4, 0.3, 0.2")},

	{"Face", false, VS_POS, FS_OUT("gl_FrontFacing")},
	{"SampleMask", false, VS_POS, FS_OUT("gl_SampleMaskIn[0]")},
	{"FragPos.x", false, VS_POS, FS_OUT("gl_FragCoord.x")},
	{"FragPos.y", false, VS_POS, FS_OUT("gl_FragCoord.y")},
	{"FragPos.z", false, VS_POS, FS_OUT("gl_FragCoord.z")},
	{"FragPos.w", false, VS_POS, FS_OUT("gl_FragCoord.w")},
	{"FragPos.xy", false, VS_POS, FS_OUT("gl_FragCoord.xyyy")},
	{"FragPos.zw", false, VS_POS, FS_OUT("gl_FragCoord.zwww")},
	{"FragPos.xyz", false, VS_POS, FS_OUT("gl_FragCoord.xyzz")},
	{"FragPos.xyw", false, VS_POS, FS_OUT("gl_FragCoord.xyww")},
	{"FragPos.xyzw", false, VS_POS, FS_OUT("gl_FragCoord")},
	{"FragPos.xy+Face", false, VS_POS, FS_OUT("gl_FragCoord.xyyy + vec4(gl_FrontFacing)")},
	{"FragPos.xy+SampleMask", false, VS_POS, FS_OUT("gl_FragCoord.xyyy + vec4(gl_SampleMaskIn[0])")},
	{"FragPos.xy+Face+SampleMask", false, VS_POS, FS_OUT("gl_FragCoord.xyyy + vec4(gl_FrontFacing) + vec4(gl_SampleMaskIn[0])")},
	{"FragPos.xyzw+Face+SampleMask", false, VS_POS, FS_OUT("gl_FragCoord + vec4(gl_FrontFacing) + vec4(gl_SampleMaskIn[0])")},
	{"SampleID", true, VS_POS, FS_OUT("vec4(gl_SampleID)")},
	{"SamplePos", true, VS_POS, FS_OUT("gl_SamplePosition.xyyy")},
	{"SampleID+SamplePos", true, VS_POS, FS_OUT("vec4(gl_SampleID) + gl_SamplePosition.xyyy")},
	{"SampleID+SampleMask", true, VS_POS, FS_OUT("vec4(gl_SampleID) + vec4(gl_SampleMaskIn[0])")},
	{"SampleID+SamplePos+SampleMask", true, VS_POS, FS_OUT("vec4(gl_SampleID) + gl_SamplePosition.xyyy + vec4(gl_SampleMaskIn[0])")},
	{"SampleID+FragPos.xy", true, VS_POS, FS_OUT("vec4(gl_SampleID) + gl_FragCoord.xyyy")},
	{"SampleID+FragPos.z", true, VS_POS, FS_OUT("vec4(gl_SampleID) + gl_FragCoord.zzzz")},
	{"SampleID+FragPos.xyzw", true, VS_POS, FS_OUT("vec4(gl_SampleID) + gl_FragCoord")},
	{"SampleID+SamplePos+SampleMask+FragPos.xyzw+Face", true, VS_POS, FS_OUT("vec4(gl_SampleID) + gl_SamplePosition.xyyy + vec4(gl_SampleMaskIn[0]) + gl_FragCoord.xyzw + vec4(gl_FrontFacing)")},

	INPUT1(1),
	INPUTS(2),
	INPUTS(3),
	INPUTS(4),
	INPUTS(5),
	INPUTS(6),
	INPUTS(7),
	INPUTS(8),
};

typedef struct {
	const char *name;
	GLenum format;
	unsigned num_samples;
	GLuint fbo;
} fb_info;

static fb_info fbs[] = {
	{"RGBA8 1s", GL_RGBA8, 0},
	{"RGBA8 "TEST_FBO_SAMPLES_STR"s", GL_RGBA8, TEST_FBO_SAMPLES},
	{"RGBA16F 1s", GL_RGBA16F, 0},
	{"RGBA16F "TEST_FBO_SAMPLES_STR"s", GL_RGBA16F, TEST_FBO_SAMPLES},
};

void
piglit_init(int argc, char **argv)
{
	for (unsigned i = 1; i < argc; i++) {
		if (strncmp(argv[i], "-freq=", 6) == 0)
			sscanf(argv[i] + 6, "%u", &gpu_freq_mhz);
	}

	piglit_require_gl_version(40);

	for (unsigned i = 0; i < ARRAY_SIZE(progs); i++)
		progs[i].prog = piglit_build_simple_program(progs[i].vs, progs[i].fs);

	for (unsigned i = 0; i < ARRAY_SIZE(fbs); i++) {
		GLuint rb;
		glGenRenderbuffers(1, &rb);
		glBindRenderbuffer(GL_RENDERBUFFER, rb);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, fbs[i].num_samples,
						 fbs[i].format, TEST_FBO_SIZE, TEST_FBO_SIZE);

		glGenFramebuffers(1, &fbs[i].fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbs[i].fbo);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
					  GL_RENDERBUFFER, rb);
		bool status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		assert(status);
	}
}

static void
run_draw(unsigned iterations)
{
	for (unsigned i = 0; i < iterations; i++)
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

enum piglit_result
piglit_display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (getenv("TEST")) {
		glUseProgram(progs[1].prog);
		run_draw(1);
		piglit_swap_buffers();
		return PIGLIT_PASS;
	}

	printf("  %-60s, %s\n", "Fragment shader", gpu_freq_mhz ? "Samples/clock," : "GSamples/second");

	printf("%*s", 62, "");
	for (unsigned j = 0; j < ARRAY_SIZE(fbs); j++) {
		printf(",%10s", fbs[j].name);
	}
	puts("");

	/* Warm up. */
	glUseProgram(progs[0].prog);
	perf_measure_gpu_rate(run_draw, 0.01);

	for (unsigned i = 0; i < ARRAY_SIZE(progs); i++) {
		printf("  %-60s", progs[i].name);
		glUseProgram(progs[i].prog);

		for (unsigned j = 0; j < ARRAY_SIZE(fbs); j++) {
			if (progs[i].sample_shading && fbs[j].num_samples <= 1) {
				printf(",       n/a");
				fflush(stdout);
				continue;
			}
			glBindFramebuffer(GL_FRAMEBUFFER, fbs[j].fbo);

			double rate = perf_measure_gpu_rate(run_draw, 0.01);
			rate *= (double)TEST_FBO_SIZE * TEST_FBO_SIZE * MAX2(fbs[j].num_samples, 1);

			if (gpu_freq_mhz) {
				rate /= gpu_freq_mhz * 1000000.0;
				printf(",%10.2f", rate);
			} else {
				printf(",%10.2f", rate / 1000000000);
			}
			fflush(stdout);
		}
		puts("");
	}

	exit(0);
	return PIGLIT_SKIP;
}
