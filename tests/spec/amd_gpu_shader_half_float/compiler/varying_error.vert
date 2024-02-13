// [config]
// expect_result: fail
// glsl_version: 1.10
// [end config]
//
// Tests an error is thrown when using half float varying without enabling

#version 110

varying float16_t x;
