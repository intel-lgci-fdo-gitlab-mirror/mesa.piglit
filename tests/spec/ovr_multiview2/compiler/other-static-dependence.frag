// [config]
// expect_result: pass
// glsl_version: 1.30
// require_extensions: GL_OVR_multiview2
// check_link: false
// [end config]
//
// From the OVR_multiview2 spec:
//
//    "Delete the paragraph which states:
//
//    It is a compile- or link-time error if any output variable other
//    than gl_Position is statically dependent on gl_ViewID_OVR."
//
// and also:
//
//    "If the OVR_multiview2 extension is enabled, the OVR_multiview extension
//    is also implicitly enabled."
//

#version 130
#extension GL_OVR_multiview2 : require

out vec4 fragColor;

void main()
{
    fragColor = vec4(float(gl_ViewID_OVR));
}
