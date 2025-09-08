// [config]
// expect_result: fail
// glsl_version: 1.10
//
// [end config]

#define Pre(x) x

void main()
{
   /* This should expand so that gl_ and FragColor are different tokens
    * hence produce an error. If it does not, the preprocessor is
    * broken
    */
   Pre(gl_)FragColor = vec4(1, 1, 1, 1);
}
