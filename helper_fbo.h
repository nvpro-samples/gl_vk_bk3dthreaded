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

#ifndef __HELPERFBO__
#define __HELPERFBO__
#include <nvgl/extensions_gl.hpp>
namespace fbo
{
    inline bool CheckStatus()
    {
        GLenum status;
        status = (GLenum) glCheckFramebufferStatus(GL_FRAMEBUFFER);
        switch(status) {
            case GL_FRAMEBUFFER_COMPLETE:
                return true;
            case GL_FRAMEBUFFER_UNSUPPORTED:
                LOGE("Unsupported framebuffer format\n");
                assert(!"Unsupported framebuffer format");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                LOGE("Framebuffer incomplete, missing attachment\n");
                assert(!"Framebuffer incomplete, missing attachment");
                break;
            //case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
            //    PRINTF(("Framebuffer incomplete, attached images must have same dimensions\n"));
            //    assert(!"Framebuffer incomplete, attached images must have same dimensions");
            //    break;
            //case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
            //    PRINTF(("Framebuffer incomplete, attached images must have same format\n"));
            //    assert(!"Framebuffer incomplete, attached images must have same format");
            //    break;
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                LOGE("Framebuffer incomplete, missing draw buffer\n");
                assert(!"Framebuffer incomplete, missing draw buffer");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                LOGE("Framebuffer incomplete, missing read buffer\n");
                assert(!"Framebuffer incomplete, missing read buffer");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                LOGE("Framebuffer incomplete attachment\n");
                assert(!"Framebuffer incomplete attachment");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                LOGE("Framebuffer incomplete multisample\n");
                assert(!"Framebuffer incomplete multisample");
                break;
            default:
                LOGE("Error %x\n", status);
                assert(!"unknown FBO Error");
                break;
        }
        return false;
    }
    //------------------------------------------------------------------------------
    // 
    //------------------------------------------------------------------------------
    inline GLuint create()
    {
        GLuint fb;
        glGenFramebuffers(1, &fb);
        return fb;
    }

    //------------------------------------------------------------------------------
    // 
    //------------------------------------------------------------------------------
    inline void bind(GLuint framebuffer)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer); 
    }

    //------------------------------------------------------------------------------
    // 
    //------------------------------------------------------------------------------
    inline bool attachTexture2DTarget(GLuint framebuffer, GLuint textureID, int colorAttachment, GLenum target=GL_TEXTURE_2D)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer); 
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+colorAttachment, target, textureID, 0);
        return true;//CheckStatus();
    }

    //------------------------------------------------------------------------------
    // 
    //------------------------------------------------------------------------------
    inline bool attachTexture2D(GLuint framebuffer, GLuint textureID, int colorAttachment, int samples)
    {
        return attachTexture2DTarget(framebuffer, textureID, colorAttachment, samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D);
    }
    //------------------------------------------------------------------------------
    // 
    //------------------------------------------------------------------------------
    inline bool detachColorTexture(GLuint framebuffer, int colorAttachment, int samples)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer); 
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+colorAttachment, 
            samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, 0, 0);
        return true;//CheckStatus();
    }
    //------------------------------------------------------------------------------
    // 
    //------------------------------------------------------------------------------
    #ifdef USE_RENDERBUFFERS
    inline bool attachRenderbuffer(GLuint framebuffer, GLuint rb, int colorAttachment)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer); 
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+colorAttachment, GL_RENDERBUFFER, rb);
        return true;//CheckStatus();
    }
    //------------------------------------------------------------------------------
    // 
    //------------------------------------------------------------------------------
    inline bool attachDSTRenderbuffer(GLuint framebuffer, GLuint dstrb)
    {
        bool bRes;
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer); 
        //glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, dstrb);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, dstrb);
        return true;//CheckStatus() ;
    }
    #endif
    //------------------------------------------------------------------------------
    // 
    //------------------------------------------------------------------------------
    inline bool attachDSTTexture2DTarget(GLuint framebuffer, GLuint textureDepthID, GLenum target=GL_TEXTURE_2D)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer); 
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, textureDepthID, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, target, textureDepthID, 0);
        return true;//CheckStatus();
    }

    //------------------------------------------------------------------------------
    // 
    //------------------------------------------------------------------------------
    inline bool attachDSTTexture2D(GLuint framebuffer, GLuint textureDepthID, int msaaRaster)
    {
        return attachDSTTexture2DTarget(framebuffer, textureDepthID, (msaaRaster > 1) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D);
    }
    //------------------------------------------------------------------------------
    // 
    //------------------------------------------------------------------------------
    inline bool detachDSTTexture(GLuint framebuffer, int msaaRaster)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        GLenum target = (msaaRaster > 1) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, 0, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, target, 0, 0);
        
        return true;//CheckStatus();
    }

    //------------------------------------------------------------------------------
    // 
    //------------------------------------------------------------------------------
    inline void deleteFBO(GLuint fbo)
    {
        glDeleteFramebuffers(1, &fbo);
    }

    //------------------------------------------------------------------------------
    // 
    //------------------------------------------------------------------------------
    inline void blitFBO(GLuint srcFBO, GLuint dstFBO,
        GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLenum filtering)
    {
        glBindFramebuffer( GL_READ_FRAMEBUFFER, srcFBO);
        glBindFramebuffer( GL_DRAW_FRAMEBUFFER, dstFBO);
        // GL_NEAREST is needed when Stencil/depth are involved
        glBlitFramebuffer( srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT, filtering );
        glBindFramebuffer( GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0);
    }
    //------------------------------------------------------------------------------
    // 
    //------------------------------------------------------------------------------
    inline void blitFBONearest(GLuint srcFBO, GLuint dstFBO,
        GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1)
    {
        blitFBO(srcFBO, dstFBO,srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, GL_NEAREST);
    }
    //------------------------------------------------------------------------------
    // 
    //------------------------------------------------------------------------------
    inline void blitFBOLinear(GLuint srcFBO, GLuint dstFBO,
        GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1)
    {
        blitFBO(srcFBO, dstFBO,srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, GL_LINEAR);
    }

} // namespace fbo
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
namespace texture
{
    inline GLuint create(int w, int h, int samples, int coverageSamples, GLenum intfmt, GLenum fmt, GLuint textureID=0)
    {
        if(samples <= 1)
        {
          if(textureID == 0)
              glCreateTextures(GL_TEXTURE_2D, 1, &textureID);
          glTextureStorage2D(textureID, 1, intfmt, w, h);
          glTextureParameterf( textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameterf( textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameterf( textureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameterf( textureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        } else {
            if(textureID == 0)
                glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &textureID);
            // Note: fixed-samples set to GL_TRUE, otherwise it could fail when attaching to FBO having render-buffer !!
            if(coverageSamples > 1)
            {
                glTextureImage2DMultisampleCoverageNV(textureID, GL_TEXTURE_2D_MULTISAMPLE, coverageSamples, samples, intfmt, w, h, GL_TRUE);
            } else {
                glTextureStorage2DMultisample(textureID, samples, intfmt, w, h, GL_TRUE);
            }
        }
        return textureID;
    }
    //------------------------------------------------------------------------------
    // 
    //------------------------------------------------------------------------------
    inline GLuint createRGBA8(int w, int h, int samples, int coverageSamples=0, GLuint textureID=0)
    {
        return create(w, h, samples, coverageSamples, GL_RGBA8, GL_RGBA, textureID);
    }

    //------------------------------------------------------------------------------
    // 
    //------------------------------------------------------------------------------
    inline GLuint createDST(int w, int h, int samples, int coverageSamples=0, GLuint textureID=0)
    {
        return create(w, h, samples, coverageSamples, GL_DEPTH24_STENCIL8, GL_DEPTH24_STENCIL8, textureID);
    }
    //------------------------------------------------------------------------------
    // 
    //------------------------------------------------------------------------------
    inline void deleteTexture(GLuint texture)
    {
        glDeleteTextures(1, &texture);
    }
} // namespace texture
//------------------------------------------------------------------------------
// Render-buffers should be forgotten. Thing of the past
//------------------------------------------------------------------------------
#ifdef USE_RENDERBUFFERS
namespace renderbuffer
{

inline GLuint createRenderBuffer(int w, int h, int samples, int coverageSamples, GLenum fmt)
{
    int query;
    GLuint rb;
    glGenRenderbuffers(1, &rb);
    glBindRenderbuffer(GL_RENDERBUFFER, rb);
    if (coverageSamples) 
    {
        glRenderbufferStorageMultisampleCoverageNV( GL_RENDERBUFFER, coverageSamples, samples, fmt,
                                                    w, h);
        glGetRenderbufferParameteriv( GL_RENDERBUFFER, GL_RENDERBUFFER_COVERAGE_SAMPLES_NV, &query);
        if ( query < coverageSamples)
            rb = 0;
        else if ( query > coverageSamples) 
        {
            // report back the actual number
            coverageSamples = query;
            LOGW("Warning: coverage samples is now %d\n", coverageSamples);
        }
        glGetRenderbufferParameteriv( GL_RENDERBUFFER, GL_RENDERBUFFER_COLOR_SAMPLES_NV, &query);
        if ( query < samples)
            rb = 0;
        else if ( query > samples) 
        {
            // report back the actual number
            samples = query;
            LOGW("Warning: depth-samples is now %d\n", samples);
        }
    }
    else 
    {
        // create a regular MSAA color buffer
        glRenderbufferStorageMultisample( GL_RENDERBUFFER, samples, fmt, w, h);
        // check the number of samples
        glGetRenderbufferParameteriv( GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &query);

        if ( query < samples) 
            rb = 0;
        else if ( query > samples) 
        {
            samples = query;
            LOGW("Warning: depth-samples is now %d\n", samples);
        }
    }
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    return rb;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
inline GLuint createRenderBufferRGBA8(int w, int h, int samples, int coverageSamples)
{
    return createRenderBuffer(w, h, samples, coverageSamples, GL_RGBA8);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
inline GLuint createRenderBufferD24S8(int w, int h, int samples, int coverageSamples)
{
    return createRenderBuffer(w, h, samples, coverageSamples, GL_DEPTH24_STENCIL8);
}
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
inline GLuint createRenderBufferS8(int w, int h, int samples, int coverageSamples)
{
    return createRenderBuffer(w, h, samples, coverageSamples, GL_STENCIL_INDEX8);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
#ifdef USE_RENDERBUFFERS
inline void deleteRenderBuffer(GLuint rb)
{
    glDeleteRenderbuffers(1, &rb);
}
#endif
} // namespace renderbuffer
#endif
#endif //#ifndef __HELPERFBO__
