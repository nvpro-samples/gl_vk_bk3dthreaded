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
layout(std140, set= DSET_OBJECT , binding= BINDING_MATERIAL ) uniform materialBuffer {
   uniform vec3 diffuse;
} material;
//layout(std140, set= DSET_GLOBAL , binding= BINDING_LIGHT ) uniform lightBuffer {
//   uniform vec3 dir;
//} light;
layout(location=0) out vec4 outColor;
void main() {

   outColor = vec4(0.5,0.5,0.2,1);
}
