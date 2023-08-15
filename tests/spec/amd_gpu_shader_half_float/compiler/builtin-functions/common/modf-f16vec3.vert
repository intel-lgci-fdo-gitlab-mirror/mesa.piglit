// [config]
// expect_result: pass
// glsl_version: 4.00
// require_extensions: GL_AMD_gpu_shader_half_float
// [end config]
//
// Tests use of half float with modf() builtin

#version 400
#extension GL_AMD_gpu_shader_half_float : enable

f16vec3 test() {

	float16_t x = 0.999HF;
	f16vec3 i = f16vec3(0.567HF);

	return modf(f16vec3(x), i);
}
