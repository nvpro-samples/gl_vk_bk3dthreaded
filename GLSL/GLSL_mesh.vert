#version 440 core
#extension GL_ARB_separate_shader_objects : enable

#define DSET_GLOBAL  0
#   define BINDING_MATRIX 0
#   define BINDING_LIGHT  1

#define DSET_OBJECT  1
#   define BINDING_MATRIXOBJ   0
#   define BINDING_MATERIAL    1
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
layout(std140, set= DSET_GLOBAL , binding= BINDING_MATRIX ) uniform matrixBuffer {
   mat4 mW;
   mat4 mVP;
   vec3 eyePos;
} matrix;
layout(std140, set= DSET_OBJECT , binding= BINDING_MATRIXOBJ ) uniform matrixObjBuffer {
   mat4 mO;
} object;
layout(location=0) in  vec3 pos;
layout(location=1) in  vec3 N;

layout(location=1) out vec3 outN;
layout(location=2) out vec3 outWPos;
layout(location=3) out vec3 outEyePos;
out gl_PerVertex {
    vec4  gl_Position;
};
void main()
{
  outN = N.xzy;
  vec4 wpos     = matrix.mW  * (object.mO * vec4(pos,1)); 
  gl_Position   = matrix.mVP * wpos;
  outWPos       = wpos.xyz;
  outEyePos     = matrix.eyePos;
}
