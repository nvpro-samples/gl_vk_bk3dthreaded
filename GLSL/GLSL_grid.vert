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
} matrix;
in layout(location=0)      vec3 pos;
out gl_PerVertex {
    vec4  gl_Position;
};
void main()
{
  vec4 wPos     = /*matrix.mW  **/ ( vec4(pos,0.51) );
  gl_Position   = matrix.mVP * wPos;
}

