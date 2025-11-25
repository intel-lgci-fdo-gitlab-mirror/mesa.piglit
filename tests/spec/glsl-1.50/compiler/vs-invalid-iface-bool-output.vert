// [config]
// expect_result: fail
// glsl_version: 1.50
// [end config]
//
// Check that a bool can't be used as output in GLSL 1.50.
//
// From Section 4.3.6 Outputs of the GLSL 1.50 spec:
//
// "Vertex and geometry output variables output per-vertex data and are
//  declared using the out storage qualifier, the centroid out storage
//  qualifier, or the deprecated varying storage qualifier. They can only
//  be float, floating-point vectors, matrices, signed or unsigned integers
//  or integer vectors, or arrays or structures of any these."

#version 150

out data {
    vec3 position;
    flat bool value;
} iface;

void main()
{
	iface.value = false;
}
