/*
 * Copyright (c) 2018-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2018-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#ifdef WIN32
#  include <windows.h>
#endif
#include <nvgl/extensions_gl.hpp>
#include <vector>
#include <string>

class GLSLShader 
{
public:
    GLSLShader();
    ~GLSLShader();

    void cleanup();

    bool addFragmentShader(const char* filename, bool isNew=true);
    bool addVertexShader(const char* filename, bool isNew=true);
    bool addFragmentShaderFromString(const char* shader);
    bool addVertexShaderFromString(const char* shader); 
    bool link();

    bool bindShader();
    void unbindShader();

    void setUniformFloat(const char* name, float val);
    void setUniformInt(const char* name, int val);
    void setUniformVector(const char * name, float* val, int count);
    void setTextureUnit(const char * texname, int texunit);
    void bindTexture(GLenum target, const char * texname, GLuint texid, int texunit);
    
    void reloadShader();

    inline GLuint getProgram() {return m_program;}

    inline int getUniformLocation(const char* name) { return glGetUniformLocation(m_program, name); }

private:

    bool compileShader(const char* filename, GLenum type);
    bool compileShaderFromString(const char *shader, GLenum type);
    bool outputProgramLog(GLuint obj);
    bool outputShaderLog(GLuint obj);

    bool m_bound;
    bool m_linkNeeded;

    std::vector<std::string> m_vertFiles;
    std::vector<std::string> m_fragFiles;
    std::vector<std::string> m_vertSrc;
    std::vector<std::string> m_fragSrc;

    GLuint m_program;

};