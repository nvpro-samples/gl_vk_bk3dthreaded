/*-----------------------------------------------------------------------
    Copyright (c) 2013, NVIDIA. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
     * Neither the name of its contributors may be used to endorse 
       or promote products derived from this software without specific
       prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
    OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    feedback to tlorach@nvidia.com (Tristan Lorach)

    Note: this section of the code is showing a basic implementation of
    Command-lists using a binary format called bk3d.
    This format has no value for command-list. However you will see that
    it allows to use pre-baked art asset without too parsing: all is
    available from structures in the file (after pointer resolution)

*/ //--------------------------------------------------------------------

#include "gl_nv_command_list.h"

#include <main.h> // for get proc

#define GL_TERMINATE_SEQUENCE_COMMAND_NV                      0x0000
#define GL_NOP_COMMAND_NV                                     0x0001
#define GL_DRAW_ELEMENTS_COMMAND_NV                           0x0002
#define GL_DRAW_ARRAYS_COMMAND_NV                             0x0003
#define GL_DRAW_ELEMENTS_STRIP_COMMAND_NV                     0x0004
#define GL_DRAW_ARRAYS_STRIP_COMMAND_NV                       0x0005
#define GL_DRAW_ELEMENTS_INSTANCED_COMMAND_NV                 0x0006
#define GL_DRAW_ARRAYS_INSTANCED_COMMAND_NV                   0x0007
#define GL_ELEMENT_ADDRESS_COMMAND_NV                         0x0008
#define GL_ATTRIBUTE_ADDRESS_COMMAND_NV                       0x0009
#define GL_UNIFORM_ADDRESS_COMMAND_NV                         0x000a
#define GL_BLEND_COLOR_COMMAND_NV                             0x000b
#define GL_STENCIL_REF_COMMAND_NV                             0x000c
#define GL_LINE_WIDTH_COMMAND_NV                              0x000d
#define GL_POLYGON_OFFSET_COMMAND_NV                          0x000e
#define GL_ALPHA_REF_COMMAND_NV                               0x000f
#define GL_VIEWPORT_COMMAND_NV                                0x0010
#define GL_SCISSOR_COMMAND_NV                                 0x0011


PFNGLCREATESTATESNVPROC __glewCreateStatesNV;
PFNGLDELETESTATESNVPROC __glewDeleteStatesNV;
PFNGLISSTATENVPROC __glewIsStateNV;
PFNGLSTATECAPTURENVPROC __glewStateCaptureNV;
PFNGLDRAWCOMMANDSSTATESNVPROC __glewDrawCommandsStatesNV;
PFNGLDRAWCOMMANDSSTATESADDRESSNVPROC __glewDrawCommandsStatesAddressNV;
PFNGLCREATECOMMANDLISTSNVPROC __glewCreateCommandListsNV;
PFNGLDELETECOMMANDLISTSNVPROC __glewDeleteCommandListsNV;
PFNGLISCOMMANDLISTNVPROC __glewIsCommandListNV;
PFNGLLISTDRAWCOMMANDSSTATESCLIENTNVPROC __glewListDrawCommandsStatesClientNV;
PFNGLCOMMANDLISTSEGMENTSNVPROC __glewCommandListSegmentsNV;
PFNGLCOMPILECOMMANDLISTNVPROC __glewCompileCommandListNV;
PFNGLCALLCOMMANDLISTNVPROC __glewCallCommandListNV;
PFNGLGETCOMMANDHEADERNVPROC __glewGetCommandHeaderNV;
PFNGLGETSTAGEINDEXNVPROC __glewGetStageIndexNV;

PFNGLDRAWCOMMANDSNVPROC __glewDrawCommandsNV;



static int initedNVcommandList = 0;

int initNVcommandList()
{
  if (initedNVcommandList) return __glewCreateStatesNV != NULL;

  __glewCreateStatesNV = (PFNGLCREATESTATESNVPROC)NVPWindow::sysGetProcAddress("glCreateStatesNV");
  __glewDeleteStatesNV = (PFNGLDELETESTATESNVPROC)NVPWindow::sysGetProcAddress("glDeleteStatesNV");
  __glewIsStateNV = (PFNGLISSTATENVPROC)NVPWindow::sysGetProcAddress("glIsStateNV");
  __glewStateCaptureNV = (PFNGLSTATECAPTURENVPROC)NVPWindow::sysGetProcAddress("glStateCaptureNV");
  __glewDrawCommandsStatesNV = (PFNGLDRAWCOMMANDSSTATESNVPROC)NVPWindow::sysGetProcAddress("glDrawCommandsStatesNV");
  __glewDrawCommandsStatesAddressNV = (PFNGLDRAWCOMMANDSSTATESADDRESSNVPROC)NVPWindow::sysGetProcAddress("glDrawCommandsStatesAddressNV");
  __glewCreateCommandListsNV = (PFNGLCREATECOMMANDLISTSNVPROC)NVPWindow::sysGetProcAddress("glCreateCommandListsNV");
  __glewDeleteCommandListsNV = (PFNGLDELETECOMMANDLISTSNVPROC)NVPWindow::sysGetProcAddress("glDeleteCommandListsNV");
  __glewIsCommandListNV = (PFNGLISCOMMANDLISTNVPROC)NVPWindow::sysGetProcAddress("glIsCommandListNV");
  __glewListDrawCommandsStatesClientNV = (PFNGLLISTDRAWCOMMANDSSTATESCLIENTNVPROC)NVPWindow::sysGetProcAddress("glListDrawCommandsStatesClientNV");
  __glewCommandListSegmentsNV = (PFNGLCOMMANDLISTSEGMENTSNVPROC)NVPWindow::sysGetProcAddress("glCommandListSegmentsNV");
  __glewCompileCommandListNV = (PFNGLCOMPILECOMMANDLISTNVPROC)NVPWindow::sysGetProcAddress("glCompileCommandListNV");
  __glewCallCommandListNV = (PFNGLCALLCOMMANDLISTNVPROC)NVPWindow::sysGetProcAddress("glCallCommandListNV");
  __glewGetCommandHeaderNV = (PFNGLGETCOMMANDHEADERNVPROC)NVPWindow::sysGetProcAddress("glGetCommandHeaderNV");
  __glewGetStageIndexNV = (PFNGLGETSTAGEINDEXNVPROC)NVPWindow::sysGetProcAddress("glGetStageIndexNV");

  __glewDrawCommandsNV = (PFNGLDRAWCOMMANDSNVPROC)NVPWindow::sysGetProcAddress("glDrawCommandsNV");

  initedNVcommandList = 1;

  // DEBUG
  //__glewCreateStatesNV = NULL;

  return __glewCreateStatesNV != NULL;
}

