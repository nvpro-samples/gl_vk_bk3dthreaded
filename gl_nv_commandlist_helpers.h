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

//
// Shader stages for command-list
//
enum ShaderStages {
    STAGE_VERTEX,
    STAGE_TESS_CONTROL,
    STAGE_TESS_EVALUATION,
    STAGE_GEOMETRY,
    STAGE_FRAGMENT,
    STAGES,
};

//
// Put together all what is needed to give to the extension function
// for a token buffer
//
struct TokenBuffer
{
    GLuint                  bufferID;   // buffer containing all
    GLuint64EXT             bufferAddr; // buffer GPU-pointer
    std::string             data;       // bytes of data containing the structures to send to the driver
    void release() {
        glDeleteBuffers(1, &bufferID);
        bufferAddr  = 0;
        bufferID    = 0;
        data.clear();
    }
};
//
// Grouping together what is needed to issue a single command made of many states, fbos and Token Buffer pointers
//
struct CommandStatesBatch
{
    void release()
    {
        dataGPUPtrs.clear();
        dataPtrs.clear();
        sizes.clear();
        stateGroups.clear();
        fbos.clear();
        numItems = 0;
    }
    void pushBatch(GLuint stateGroup_, GLuint fbo_, GLuint64EXT dataGPUPtr_, const GLvoid* dataPtr_, GLsizei size_)
    {
        dataGPUPtrs.push_back(dataGPUPtr_);
        dataPtrs.push_back(dataPtr_);
        sizes.push_back(size_);
        stateGroups.push_back(stateGroup_);
        fbos.push_back(fbo_);
        numItems = fbos.size();
    }
    CommandStatesBatch& operator+= (CommandStatesBatch &cb)
    {
        // TODO: do better than that...
        size_t sz = cb.fbos.size();
        for(int i=0; i<sz; i++)
        {
            dataGPUPtrs.push_back(cb.dataGPUPtrs[i]);
            dataPtrs.push_back(cb.dataPtrs[i]);
            sizes.push_back(cb.sizes[i]);
            stateGroups.push_back(cb.stateGroups[i]);
            fbos.push_back(cb.fbos[i]);
        }
        numItems += sz;
        return *this;
    }
    std::vector<GLuint64EXT> dataGPUPtrs;   // pointer in data where to locate each separate groups (for glListDrawCommandsStatesClientNV)
    std::vector<const GLvoid*> dataPtrs;   // pointer in data where to locate each separate groups (for glListDrawCommandsStatesClientNV)
    std::vector<GLsizei>    sizes;      // sizes of each groups
    std::vector<GLuint>       stateGroups;// state-group IDs used for each groups
    std::vector<GLuint>       fbos;       // FBOs being used for each groups
    size_t                  numItems;   // == fbos.size() or sizes.size()...

    //CommandStatesBatch& operator+= (CommandStatesBatch &cb)
    //{
    //    dataGPUPtrs += cb.dataGPUPtrs;
    //    dataPtrs += cb.dataPtrs;
    //    sizes += cb.sizes;
    //    stateGroups += cb.stateGroups;
    //    fbos += cb.fbos;
    //    numItems += fbos.size();
    //    return *this;
    //}
    //std::basic_string<GLuint64EXT, std::char_traits<GLuint64EXT>, std::allocator<GLuint64EXT> > dataGPUPtrs;   // pointer in data where to locate each separate groups (for glListDrawCommandsStatesClientNV)
    //std::basic_string<const GLvoid*, std::char_traits<const GLvoid*>, std::allocator<const GLvoid*> > dataPtrs;   // pointer in data where to locate each separate groups (for glListDrawCommandsStatesClientNV)
    //std::basic_string<GLsizei, std::char_traits<GLsizei>, std::allocator<GLsizei> >    sizes;      // sizes of each groups
    //std::basic_string<GLuint, std::char_traits<GLuint>, std::allocator<GLuint> >       stateGroups;// state-group IDs used for each groups
    //std::basic_string<GLuint, std::char_traits<GLuint>, std::allocator<GLuint> >       fbos;       // FBOs being used for each groups
    //size_t                  numItems;   // == fbos.size() or sizes.size()...
};

//-----------------------------------------------------------------------------
// Useful stuff for Command-list
//-----------------------------------------------------------------------------
static GLuint   s_header[GL_FRONT_FACE_COMMAND_NV+1] = { 0 };
static GLuint   s_headerSizes[GL_FRONT_FACE_COMMAND_NV+1] = { 0 };

static GLushort s_stages[STAGES];

struct Token_Nop {
    static const GLenum   ID = GL_NOP_COMMAND_NV;
    NOPCommandNV      cmd;
    Token_Nop() {
        cmd.header = s_header[ID];
    }
};

struct Token_TerminateSequence {
    static const GLenum   ID = GL_TERMINATE_SEQUENCE_COMMAND_NV;

    TerminateSequenceCommandNV cmd;

    Token_TerminateSequence() {
        cmd.header = s_header[ID];
    }
};

struct Token_DrawElemsInstanced {
    static const GLenum   ID = GL_DRAW_ELEMENTS_INSTANCED_COMMAND_NV;

    DrawElementsInstancedCommandNV   cmd;

    Token_DrawElemsInstanced() {
        cmd.baseInstance = 0;
        cmd.baseVertex = 0;
        cmd.firstIndex = 0;
        cmd.count = 0;
        cmd.instanceCount = 1;

        cmd.header = s_header[ID];
    }
};

struct Token_DrawArraysInstanced {
    static const GLenum   ID = GL_DRAW_ARRAYS_INSTANCED_COMMAND_NV;

    DrawArraysInstancedCommandNV   cmd;

    Token_DrawArraysInstanced() {
        cmd.baseInstance = 0;
        cmd.first = 0;
        cmd.count = 0;
        cmd.instanceCount = 1;

        cmd.header = s_header[ID];
    }
};

struct Token_DrawElements {
    static const GLenum   ID = GL_DRAW_ELEMENTS_COMMAND_NV;

    DrawElementsCommandNV   cmd;

    Token_DrawElements() {
        cmd.baseVertex = 0;
        cmd.firstIndex = 0;
        cmd.count = 0;

        cmd.header = s_header[ID];
    }
};

struct Token_DrawArrays {
    static const GLenum   ID = GL_DRAW_ARRAYS_COMMAND_NV;

    DrawArraysCommandNV   cmd;

    Token_DrawArrays() {
        cmd.first = 0;
        cmd.count = 0;

        cmd.header = s_header[ID];
    }
};

struct Token_DrawElementsStrip {
    static const GLenum   ID = GL_DRAW_ELEMENTS_STRIP_COMMAND_NV;

    DrawElementsCommandNV   cmd;

    Token_DrawElementsStrip() {
        cmd.baseVertex = 0;
        cmd.firstIndex = 0;
        cmd.count = 0;

        cmd.header = s_header[ID];
    }
};

struct Token_DrawArraysStrip {
    static const GLenum   ID = GL_DRAW_ARRAYS_STRIP_COMMAND_NV;

    DrawArraysCommandNV   cmd;

    Token_DrawArraysStrip() {
        cmd.first = 0;
        cmd.count = 0;

        cmd.header = s_header[ID];
    }
};

struct Token_AttributeAddress {
    static const GLenum   ID = GL_ATTRIBUTE_ADDRESS_COMMAND_NV;

    AttributeAddressCommandNV cmd;

    Token_AttributeAddress() {
        cmd.header = s_header[ID];
    }
};

struct Token_ElementAddress {
    static const GLenum   ID = GL_ELEMENT_ADDRESS_COMMAND_NV;

    ElementAddressCommandNV cmd;

    Token_ElementAddress() {
        cmd.header = s_header[ID];
    }
};

struct Token_UniformAddress {
    static const GLenum   ID = GL_UNIFORM_ADDRESS_COMMAND_NV;

    UniformAddressCommandNV   cmd;

    Token_UniformAddress() {
        cmd.header = s_header[ID];
    }
};

struct Token_BlendColor{
    static const GLenum   ID = GL_BLEND_COLOR_COMMAND_NV;

    BlendColorCommandNV     cmd;

    Token_BlendColor() {
        cmd.header = s_header[ID];
    }
};

struct Token_StencilRef{
    static const GLenum   ID = GL_STENCIL_REF_COMMAND_NV;

    StencilRefCommandNV cmd;

    Token_StencilRef() {
        cmd.header = s_header[ID];
    }
};

struct Token_LineWidth{
    static const GLenum   ID = GL_LINE_WIDTH_COMMAND_NV;

    LineWidthCommandNV  cmd;

    Token_LineWidth() {
        cmd.header = s_header[ID];
    }
};

struct Token_PolygonOffset{
    static const GLenum   ID = GL_POLYGON_OFFSET_COMMAND_NV;

    PolygonOffsetCommandNV  cmd;

    Token_PolygonOffset() {
        cmd.header = s_header[ID];
    }
};

struct Token_AlphaRef{
    static const GLenum   ID = GL_ALPHA_REF_COMMAND_NV;

    AlphaRefCommandNV cmd;

    Token_AlphaRef() {
        cmd.header = s_header[ID];
    }
};

struct Token_Viewport{
    static const GLenum   ID = GL_VIEWPORT_COMMAND_NV;
    ViewportCommandNV cmd;
    Token_Viewport() {
        cmd.header = s_header[ID];
    }
};

struct Token_Scissor {
    static const GLenum   ID = GL_SCISSOR_COMMAND_NV;
    ScissorCommandNV  cmd;
    Token_Scissor() {
        cmd.header = s_header[ID];
    }
};

struct Token_FrontFace {
    static const GLenum   ID = GL_FRONT_FACE_COMMAND_NV;
    FrontFaceCommandNV cmd;
    Token_FrontFace() {
        cmd.header = s_header[ID];
    }
};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
template <class T>
void registerSize()
{
    s_headerSizes[T::ID] = sizeof(T);
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void initTokenInternals()
{
    registerSize<Token_TerminateSequence>();
    registerSize<Token_Nop>();
    registerSize<Token_DrawElements>();
    registerSize<Token_DrawArrays>();
    registerSize<Token_DrawElementsStrip>();
    registerSize<Token_DrawArraysStrip>();
    registerSize<Token_DrawElemsInstanced>();
    registerSize<Token_DrawArraysInstanced>();
    registerSize<Token_AttributeAddress>();
    registerSize<Token_ElementAddress>();
    registerSize<Token_UniformAddress>();
    registerSize<Token_LineWidth>();
    registerSize<Token_PolygonOffset>();
    registerSize<Token_Scissor>();
    registerSize<Token_BlendColor>();
    registerSize<Token_Viewport>();
    registerSize<Token_AlphaRef>();
    registerSize<Token_StencilRef>();
    registerSize<Token_FrontFace>();

    for (int i = 0; i < (GL_FRONT_FACE_COMMAND_NV+1); i++){
        // using i instead of a table of token IDs because the are arranged in the same order as i incrementing.
        // shortcut for the source code. See gl_nv_command_list.h
        s_header[i] = glGetCommandHeaderNV(i/*==Token enum*/, s_headerSizes[i]);
    }
    s_stages[STAGE_VERTEX] = glGetStageIndexNV(GL_VERTEX_SHADER);
    s_stages[STAGE_TESS_CONTROL] = glGetStageIndexNV(GL_TESS_CONTROL_SHADER);
    s_stages[STAGE_TESS_EVALUATION] = glGetStageIndexNV(GL_TESS_EVALUATION_SHADER);
    s_stages[STAGE_GEOMETRY] = glGetStageIndexNV(GL_GEOMETRY_SHADER);
    s_stages[STAGE_FRAGMENT] = glGetStageIndexNV(GL_FRAGMENT_SHADER);
}

//------------------------------------------------------------------------------
// build 
//------------------------------------------------------------------------------
std::string buildLineWidthCommand(float w)
{
    std::string cmd;
    Token_LineWidth lw;
    lw.cmd.lineWidth = w;
    cmd = std::string((const char*)&lw, sizeof(Token_LineWidth));

    return cmd;
}
//------------------------------------------------------------------------------
// build 
//------------------------------------------------------------------------------
std::string buildUniformAddressCommand(int idx, GLuint64 p, GLsizeiptr sizeBytes, ShaderStages stage)
{
    std::string cmd;
    Token_UniformAddress attr;
    attr.cmd.stage = s_stages[stage];
    attr.cmd.index = idx;
    ((GLuint64EXT*)&attr.cmd.addressLo)[0] = p;
    cmd = std::string((const char*)&attr, sizeof(Token_UniformAddress));

    return cmd;
}
//------------------------------------------------------------------------------
// build 
//------------------------------------------------------------------------------
std::string buildAttributeAddressCommand(int idx, GLuint64 p, GLsizeiptr sizeBytes)
{
    std::string cmd;
    Token_AttributeAddress attr;
    attr.cmd.index = idx;
    ((GLuint64EXT*)&attr.cmd.addressLo)[0] = p;
    cmd = std::string((const char*)&attr, sizeof(Token_AttributeAddress));

    return cmd;
}
//------------------------------------------------------------------------------
// build 
//------------------------------------------------------------------------------
std::string buildElementAddressCommand(GLuint64 ptr, GLenum indexFormatGL)
{
    std::string cmd;
    Token_ElementAddress attr;
    ((GLuint64EXT*)&attr.cmd.addressLo)[0] = ptr;
    switch (indexFormatGL)
    {
    case GL_UNSIGNED_INT:
        attr.cmd.typeSizeInByte = 4;
        break;
    case GL_UNSIGNED_SHORT:
        attr.cmd.typeSizeInByte = 2;
        break;
    }
    cmd = std::string((const char*)&attr, sizeof(Token_AttributeAddress));

    return cmd;
}
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string buildDrawElementsCommand(GLenum topologyGL, GLuint indexCount)
{
    std::string cmd;
    Token_DrawElements dc;
    Token_DrawElementsStrip dcstrip;
    switch (topologyGL)
    {
    case GL_TRIANGLE_STRIP:
    case GL_QUAD_STRIP:
    case GL_LINE_STRIP:
        dcstrip.cmd.baseVertex = 0;
        dcstrip.cmd.firstIndex = 0;
        dcstrip.cmd.count = indexCount;
        cmd = std::string((const char*)&dcstrip, sizeof(Token_DrawElementsStrip));
        break;
    default:
        dc.cmd.baseVertex = 0;
        dc.cmd.firstIndex = 0;
        dc.cmd.count = indexCount;
        cmd = std::string((const char*)&dc, sizeof(Token_DrawElements));
        break;
    }
    return cmd;
}
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string buildDrawArraysCommand(GLenum topologyGL, GLuint indexCount)
{
    std::string cmd;
    Token_DrawArrays dc;
    Token_DrawArraysStrip dcstrip;
    switch (topologyGL)
    {
    case GL_TRIANGLE_STRIP:
    case GL_QUAD_STRIP:
    case GL_LINE_STRIP:
        dcstrip.cmd.first = 0;
        dcstrip.cmd.count = indexCount;
        cmd = std::string((const char*)&dcstrip, sizeof(Token_DrawArraysStrip));
        break;
    default:
        dc.cmd.first = 0;
        dc.cmd.count = indexCount;
        cmd = std::string((const char*)&dc, sizeof(Token_DrawArrays));
        break;
    }
    return cmd;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string buildViewportCommand(GLint x, GLint y, GLsizei width, GLsizei height)
{
    std::string cmd;
    Token_Viewport dc;
    dc.cmd.x = x;
    dc.cmd.y = y;
    dc.cmd.width = width;
    dc.cmd.height = height;
    cmd = std::string((const char*)&dc, sizeof(Token_Viewport));
    return cmd;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string buildBlendColorCommand(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    std::string cmd;
    Token_BlendColor dc;
    dc.cmd.red = red;
    dc.cmd.green = green;
    dc.cmd.blue = blue;
    dc.cmd.alpha = alpha;
    cmd = std::string((const char*)&dc, sizeof(Token_BlendColor));
    return cmd;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string buildStencilRefCommand(GLuint frontStencilRef, GLuint backStencilRef)
{
    std::string cmd;
    Token_StencilRef dc;
    dc.cmd.frontStencilRef = frontStencilRef;
    dc.cmd.backStencilRef = backStencilRef;
    cmd = std::string((const char*)&dc, sizeof(Token_StencilRef));
    return cmd;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string buildPolygonOffsetCommand(GLfloat scale, GLfloat bias)
{
    std::string cmd;
    Token_PolygonOffset dc;
    dc.cmd.bias = bias;
    dc.cmd.scale = scale;
    cmd = std::string((const char*)&dc, sizeof(Token_PolygonOffset));
    return cmd;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string buildScissorCommand(GLint x, GLint y, GLsizei width, GLsizei height)
{
    std::string cmd;
    Token_Scissor dc;
    dc.cmd.x = x;
    dc.cmd.y = y;
    dc.cmd.width = width;
    dc.cmd.height = height;
    cmd = std::string((const char*)&dc, sizeof(Token_Scissor));
    return cmd;
}
