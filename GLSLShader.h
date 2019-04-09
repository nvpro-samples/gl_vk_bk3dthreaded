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