//
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// https://creativecommons.org/licenses/by-nc-sa/3.0/us/
// Voronoi part taken from Ben Weston: https://www.shadertoy.com/view/ldsGzl
// few changes made for the purpose of Vulkan code
//

#version 440 core
#extension GL_ARB_separate_shader_objects : enable

#define DSET_GLOBAL  0
#   define BINDING_MATRIX 0
#   define BINDING_LIGHT  1
#   define BINDING_NOISE  2

#define DSET_OBJECT  1
#   define BINDING_MATRIXOBJ   0
#   define BINDING_MATERIAL    1
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
layout(set= DSET_GLOBAL, binding= BINDING_NOISE ) uniform sampler3D iChannel0;

layout(std140, set= DSET_OBJECT , binding= BINDING_MATERIAL ) uniform materialBuffer {
   vec3  diffuse;
   float a;
} material;

layout(location=1) in  vec3 N;
layout(location=2) in  vec3 inWPos;
layout(location=3) in  vec3 inEyePos;

layout(location=0,index=0) out vec4 outColor;

vec3 Sky( vec3 ray )
{
    return mix( vec3(.8), vec3(0), exp2(-(1.0/max(ray.y,.01))*vec3(.4,.6,1.0)) );
}

mat2 mm2(in float a){float c = cos(a), s = sin(a);return mat2(c,-s,s,c);}

vec3 Voronoi( vec3 pos )
{
    vec3 d[8];
    d[0] = vec3(0,0,0);
    d[1] = vec3(1,0,0);
    d[2] = vec3(0,1,0);
    d[3] = vec3(1,1,0);
    d[4] = vec3(0,0,1);
    d[5] = vec3(1,0,1);
    d[6] = vec3(0,1,1);
    d[7] = vec3(1,1,1);
    
    const float maxDisplacement = .7; //tweak this to hide grid artefacts
    
    vec3 pf = floor(pos);

    const float phi = 1.61803398875;

    float closest = 12.0;
    vec3 result;
    for ( int i=0; i < 8; i++ )
    {
        vec3 v = (pf+d[i]);
        vec3 r = fract(phi*v.yzx+17.*fract(v.zxy*phi)+v*v*.03);//Noise(ivec3(floor(pos+d[i])));
        vec3 p = d[i] + maxDisplacement*(r.xyz-.5);
        p -= fract(pos);
        float lsq = dot(p,p);
        if ( lsq < closest )
        {
            closest = lsq;
            result = r;
        }
    }
    return fract(result.xyz);//+result.www); // random colour
}

vec3 shade( vec3 pos, vec3 norm, vec3 rayDir, vec3 lightDir )
{
    vec3 paint = material.diffuse;

    vec3 norm2 = normalize(norm+.02*(Voronoi(pos*800.0)*2.0-1.0));
    
    if ( dot(norm2,rayDir) > 0.0 ) // we shouldn't see flecks that point away from us
        norm2 -= 2.0*dot(norm2,rayDir)*rayDir;


    // diffuse layer, reduce overall contrast
    vec3 result = paint*.6*(pow(max(0.0,dot(norm,lightDir)),2.0)+.2);

    vec3 h = normalize( lightDir-rayDir );
    vec3 s = pow(max(0.0,dot(h,norm2)),50.0)*10.0*vec3(1);

    float rdotn = dot(rayDir,norm2);
    vec3 reflection = rayDir-2.0*rdotn*norm;
    s += Sky( reflection );

    float f = pow(1.0+rdotn,5.0);
    f = mix( .2, 1.0, f );
    
    result = mix(result,paint*s,f);
    
    // gloss layer
    s = pow(max(0.0,dot(h,norm)),1000.0)*32.0*vec3(1);
    
    rdotn = dot(rayDir,norm);
    reflection = rayDir-2.0*rdotn*norm;
    
    return result;
}

void main()
{
   vec3 lightDir = vec3(0, 0.707, 0.707);
   vec3 ray = inWPos;
   ray -= inEyePos;
   ray = normalize(ray);
   vec3 shaded = shade( inWPos, N, ray, lightDir );
   outColor = vec4(shaded, 1);
}
