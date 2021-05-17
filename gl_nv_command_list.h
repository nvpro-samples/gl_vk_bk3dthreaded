/*
 * Copyright (c) 2016-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2016-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
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

