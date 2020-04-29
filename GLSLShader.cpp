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
#include "GLSLShader.h"
#include <string.h>
#include <string>
#include <fstream>
#include "nvh/nvprint.hpp"

GLSLShader::GLSLShader()
{
    m_linkNeeded = false;
    m_program = 0;
}

//*NVTL*
GLSLShader::~GLSLShader()
{
    cleanup();
}
//*NVTL*
void GLSLShader::cleanup()
{
    m_fragFiles.clear();
    m_vertFiles.clear();
    m_fragSrc.clear();
    m_vertSrc.clear();
    if(m_program) glDeleteProgram(m_program);
    m_program = 0;
}
bool GLSLShader::compileShaderFromString(const char *shader, GLenum type)
{    
    bool bRes = true;
    if(!shader)
        return false;
    if(0 == m_program)
        m_program = glCreateProgram();
    GLuint obj = glCreateShader(type);

    // set source
    GLint size = (GLint)strlen(shader);
    const GLchar* progString = (const GLchar*)shader;
    glShaderSource(obj, 1, &progString, &size);
    glCompileShader(obj);
    bRes = outputShaderLog(obj);

    glAttachShader(m_program, obj);
  glDeleteShader(obj);

    m_linkNeeded = true;
    return bRes;
}

bool GLSLShader::compileShader(const char *filename, GLenum type)
{    
    bool bRes;
    if(0 == m_program)
        m_program = glCreateProgram();

    std::ifstream ifs;
    ifs.open(filename);
    if(ifs.bad())
        return false;
    
    std::string file((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    //*NVTL* added the checking here because ifs.bad() doesn't fail when the file doesn't exist...
    if(file.size()==0)
    {
        //*NVTL* : added the message. Because I lost quite some time because of this missing feature ;-P
        if(filename)    LOGE("\nGLSL ERROR: loading file %s \n", filename ? filename : "NULL");
        fflush(stdout);
//#        ifdef _DEBUG
//        _asm {int 3 }
//#        endif
        return false;
    }

    //std::string file;
    //while(!ifs.eof()) file += ifs.get();


    GLuint obj = glCreateShader(type);

    // set source
    GLint size = (GLint)file.size();
    const GLchar* progString = (const GLchar*)file.c_str();
    glShaderSource(obj, 1, &progString, &size);
    glCompileShader(obj);
    bRes = outputShaderLog(obj);

    glAttachShader(m_program, obj);
  glDeleteShader(obj);

    ifs.close();
    m_linkNeeded = true;
    return bRes;
}

bool GLSLShader::addFragmentShader(const char *filename, bool isNew)
{
    bool bRes;

    bRes = compileShader(filename, GL_FRAGMENT_SHADER);
    
    if(isNew) m_fragFiles.push_back(filename);
    return bRes;
}


bool GLSLShader::addVertexShader(const char *filename, bool isNew)
{
    bool bRes;

    bRes = compileShader(filename, GL_VERTEX_SHADER);

    if(isNew) m_vertFiles.push_back(filename);
    return bRes;
}

//----> *NVTL*
bool GLSLShader::addFragmentShaderFromString(const char *shader)
{
    bool bRes;

    bRes = compileShaderFromString(shader, GL_FRAGMENT_SHADER);
    
    m_fragSrc.push_back(shader);
    return bRes;
}


bool GLSLShader::addVertexShaderFromString(const char *shader)
{
    bool bRes;

    bRes = compileShaderFromString(shader, GL_VERTEX_SHADER);

    m_vertSrc.push_back(shader);
    return bRes;
}
// <---- *NVTL*
bool GLSLShader::link()
{
    bool bRes = true;
    if(m_linkNeeded)
    {
        glLinkProgram(m_program);
        bRes = outputProgramLog(m_program);
        m_linkNeeded = false;
    }
    return bRes;
}

bool GLSLShader::bindShader()
{
    bool bRes = true;
   
    if(m_linkNeeded)
        bRes = link();

    glUseProgram(m_program);

    //GL_FLOAT_RGBA32_NV;
    //GL_RGBA_FLOAT32_ATI;
    return bRes;
}

void GLSLShader::unbindShader()
{
    glUseProgram(0);
}

bool GLSLShader::outputProgramLog(GLuint obj)
{
    char buf[1024];
    int len;
    glGetProgramInfoLog(obj, 1024, &len, buf);
    if(len)
    {
        LOGW("Log for %d:\n%s\n\n", obj, buf);
        if(strstr(buf, "error") != nullptr)
            return false;
#    ifdef _DEBUG
        //if(strstr(buf, "error") > 0)
        //{
        //    _asm {int 3 }
        //}
#    endif
    }
    return true;
}

bool GLSLShader::outputShaderLog(GLuint obj)
{
    char buf[1024];
    int len;
    glGetShaderInfoLog(obj, 1024, &len, buf);
    if(len)
    {
        LOGW("Log for %d:\n%s\n\n", obj, buf);
        if(strstr(buf, "error") != nullptr)
            return false;
#    ifdef _DEBUG
        //if(strstr(buf, "error") > 0)
        //{
        //    _asm {int 3 }
        //}
#    endif
    }
    return true;
}

void GLSLShader::setUniformFloat(const char *name, float val)
{
    glUniform1f(glGetUniformLocation(m_program, name), val);
}

void GLSLShader::setUniformInt(const char *name, int val)
{
    glUniform1i(glGetUniformLocation(m_program, name), val);
}

//----> *NVTL*
void GLSLShader::setUniformVector(const char * name, float* val, int count)
{
    GLint id = glGetUniformLocation(m_program, name);
    if (id == -1) {
        return;
    }
    switch (count) {
        case 1:
            glUniform1fv(id, 1, val);
            break;
        case 2:
            glUniform2fv(id, 1, val);
            break;
        case 3:
            glUniform3fv(id, 1, val);
            break;
        case 4:
            glUniform4fv(id, 1, val);
            break;
    }
}

void GLSLShader::setTextureUnit(const char * texname, int texunit)
{
    GLint linked;
    glGetProgramiv(m_program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        return;
    }
    GLint id = glGetUniformLocation(m_program, texname);
    if (id == -1) {
        return;
    }
    glUniform1i(id, texunit);
}

void GLSLShader::bindTexture(GLenum target, const char * texname, GLuint texid, int texunit)
{
    glActiveTexture(GL_TEXTURE0 + texunit);
    glBindTexture(target, texid);
    setTextureUnit(texname, texunit);
    glActiveTexture(GL_TEXTURE0);
}
//<---- *NVTL*

void GLSLShader::reloadShader()
{
    glDeleteProgram(m_program);

    //We should really also detach the old fragment and vertex shaders
    //and delete them as well...

    m_program = 0;

    for(unsigned int i = 0; i < m_vertFiles.size(); ++i)
        addVertexShader(m_vertFiles[i].c_str(), false);
    for(unsigned int i = 0; i < m_vertSrc.size(); ++i)
        addVertexShader(m_vertSrc[i].c_str(), false);

    for(unsigned int i = 0; i < m_fragFiles.size(); ++i)
        addFragmentShader(m_fragFiles[i].c_str(), false);
    for(unsigned int i = 0; i < m_fragSrc.size(); ++i)
        addFragmentShader(m_fragSrc[i].c_str(), false);
}
