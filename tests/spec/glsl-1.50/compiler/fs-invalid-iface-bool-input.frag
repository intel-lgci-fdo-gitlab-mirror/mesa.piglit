// [config]
// expect_result: fail
// glsl_version: 1.50
// [end config]
//
// Check that a bool can't be used as input in GLSL 1.50.
//
// From section 4 ("Variables and Types") of the GLSL 1.30 spec:
//
// Fragment inputs can only be signed and unsigned integers
// and integer vectors, float, floating-point vectors, matrices,
// or arrays or structures of these.

#version 150

in data {
    vec3 position;
    flat bool value;
} iface;

out vec4 color;

void main()
{
	color = vec4(iface.position, iface.value);
}
