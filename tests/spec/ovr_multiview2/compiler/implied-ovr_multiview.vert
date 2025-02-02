// [config]
// expect_result: pass
// glsl_version: 3.30
// require_extensions: GL_OVR_multiview2
// check_link: false
// [end config]
//
// From the OVR_multiview2 spec:
//
//    "If the OVR_multiview2 extension is enabled, the OVR_multiview extension
//    is also implicitly enabled."
//

#version 330
#extension GL_OVR_multiview2 : require

layout(num_views = 2) in;

void main()
{
   gl_Position = vec4(1.0);
}
