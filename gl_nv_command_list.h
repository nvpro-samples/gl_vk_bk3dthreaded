/* Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NV_COMMANDLIST_H__
#define NV_COMMANDLIST_H__

#include <nvgl/extensions_gl.hpp>


#  if defined(__MINGW32__) || defined(__CYGWIN__)
#    define GLEXT_APIENTRY __stdcall
#  elif (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED) || defined(__BORLANDC__)
#    define GLEXT_APIENTRY __stdcall
#  else
#    define GLEXT_APIENTRY
#  endif

/*
#pragma pack(push,1)

typedef struct {
    GLuint  header;
  } TerminateSequenceCommandNV;

  typedef struct {
    GLuint  header;
  } NOPCommandNV;

  typedef  struct {
    GLuint  header;
    GLuint  count;
    GLuint  firstIndex;
    GLuint  baseVertex;
  } DrawElementsCommandNV;

  typedef  struct {
    GLuint  header;
    GLuint  count;
    GLuint  first;
  } DrawArraysCommandNV;

  typedef  struct {
    GLuint  header;
    GLenum  mode;
    GLuint  count;
    GLuint  instanceCount;
    GLuint  firstIndex;
    GLuint  baseVertex;
    GLuint  baseInstance;
  } DrawElementsInstancedCommandNV;

  typedef  struct {
    GLuint  header;
    GLenum  mode;
    GLuint  count;
    GLuint  instanceCount;
    GLuint  first;
    GLuint  baseInstance;
  } DrawArraysInstancedCommandNV;

  typedef struct {
    GLuint  header;
    GLuint  addressLo;
    GLuint  addressHi;
    GLuint  typeSizeInByte;
  } ElementAddressCommandNV;

  typedef struct {
    GLuint  header;
    GLuint  index;
    GLuint  addressLo;
    GLuint  addressHi;
  } AttributeAddressCommandNV;

  typedef struct {
    GLuint    header;
    GLushort  index;
    GLushort  stage;
    GLuint    addressLo;
    GLuint    addressHi;
  } UniformAddressCommandNV;

  typedef struct {
    GLuint  header;
    float   red;
    float   green;
    float   blue;
    float   alpha;
  } BlendColorCommandNV;

  typedef struct {
    GLuint  header;
    GLuint  frontStencilRef;
    GLuint  backStencilRef;
  } StencilRefCommandNV;

  typedef struct {
    GLuint  header;
    float   lineWidth;
  } LineWidthCommandNV;

  typedef struct {
    GLuint  header;
    float   scale;
    float   bias;
  } PolygonOffsetCommandNV;

  typedef struct {
    GLuint  header;
    float   alphaRef;
  } AlphaRefCommandNV;

  typedef struct {
    GLuint  header;
    GLuint  x;
    GLuint  y;
    GLuint  width;
    GLuint  height;
  } ViewportCommandNV;  // only ViewportIndex 0

  typedef struct {
    GLuint  header;
    GLuint  x;
    GLuint  y;
    GLuint  width;
    GLuint  height;
  } ScissorCommandNV;   // only ViewportIndex 0

  typedef struct {
    GLuint  header;
    GLuint  frontFace;  // 0 for CW, 1 for CCW
  } FrontFaceCommandNV;

#pragma pack(pop)
*/

#endif

