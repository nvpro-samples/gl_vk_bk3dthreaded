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