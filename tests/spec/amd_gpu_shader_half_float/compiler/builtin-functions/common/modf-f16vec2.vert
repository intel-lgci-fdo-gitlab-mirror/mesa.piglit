// [config]
// expect_result: pass
// glsl_version: 4.00
// require_extensions: GL_AMD_gpu_shader_half_float
// [end config]
//
// Tests use of half float with modf() builtin

#version 400
#extension GL_AMD_gpu_shader_half_float : enable

f16vec2 test() {

	float16_t x = 0.999HF;
	f16vec2 i = f16vec2(0.567HF);

	return modf(f16vec2(x), i);
}
