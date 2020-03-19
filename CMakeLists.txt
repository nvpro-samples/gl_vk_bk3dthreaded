cmake_minimum_required(VERSION 2.8)
set(PROJNAME gl_vk_bk3dthreaded)
Project(${PROJNAME})
Message(STATUS "-------------------------------")
Message(STATUS "Processing Project ${PROJNAME}:")

#####################################################################################
# offer the choice of having shared_sources as a sub-folder... good for packaging a sample
#
if(NOT BASE_DIRECTORY) # if not defined, it means this cmake file was called as the first entry point and not included
  # check if the external repository is outside or inside the project (as a sub-module)
  # testing the file CMakeLists_include.txt because when sub-modules are not cloned, the folders are still there...
  # we also assume here that if shared_sources is there, shared_external is, too...
  SET(BASE_DIRECTORY "" CACHE FILEPATH "folder containing shared_sources")
  SET(ADD_SUBDIR_BELOW 1)
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/shared_sources/CMakeLists_include.txt)
    SET(BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../shared_sources/CMakeLists_include.txt)
    SET(BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..)
  elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../shared_sources/CMakeLists_include.txt)
    SET(BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../..)
  else()
  endif()
endif(NOT BASE_DIRECTORY)

if(EXISTS ${BASE_DIRECTORY}/shared_sources/CMakeLists_include.txt)
  INCLUDE(${BASE_DIRECTORY}/shared_sources/CMakeLists_include.txt)
else()
  Message(FATAL_ERROR "could not find base directory, please set BASE_DIRECTORY to folder containing shared_sources")
endif()
    
_add_project_definitions(${PROJNAME})

#####################################################################################
# additions from packages needed for this sample
# add refs  in LIBRARIES_OPTIMIZED
# add refs  in LIBRARIES_DEBUG
# add files in PACKAGE_SOURCE_FILES
#
_add_package_OpenGL()
_add_package_VulkanSDK()
_add_package_ImGUI()
_add_package_ZLIB()

#####################################################################################
# process the rest of some cmake code that needs to be done *after* the packages add
_add_shared_sources_lib()

#####################################################################################
# Source files for this project
#
file(GLOB SOURCE_FILES *.cpp *.hpp *.inl *.h *.c mt/*.cpp mt/*.h VK_nvidia/*.c VK_nvidia/*.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/VK_nvidia)

#####################################################################################
# download model for this demo to run. Avoids using GitHub for this
# TODO: we need to put the models online. Should happen next week (>Dec.22)
#
unset(FILELISTOUT)
set(FILELIST
"SubMarine_134.bk3d.gz"
)
_download_files(
  "${FILELIST}"
  "http://developer.download.nvidia.com/ProGraphics/nvpro-samples"
  ${DOWNLOAD_TARGET_DIR}
  0
  FILELISTOUT
)
# you can download more :
if(0)
  set(FILELIST 
  "Jet_134.bk3d.gz"
  "Driveline_v134.bk3d.gz"
  "Body_v134.bk3d.gz"
  "Camera_134.bk3d.gz"
  "ConceptCar_134.bk3d.gz"
  "Eiffel_133.bk3d.gz"
  "Smobby_134.bk3d.gz"
  )
  _download_files(
    "${FILELIST}"
    "http://developer.download.nvidia.com/ProGraphics/nvpro-samples"
    ${DOWNLOAD_TARGET_DIR}
    0
    FILELISTOUT
  )
endif()
#####################################################################################
# GLSL to SPIR-V custom build
#
#more than one file can be given: _compile_GLSL("GLSL_mesh.vert;GLSL_mesh.frag" "GLSL_mesh.spv" GLSL_SOURCES)
# the SpirV validator is fine as long as files are for different pipeline stages (entry points still need to be main())
#_compile_GLSL(<source(s)> <target spv> <LIST where files are appended>)
UNSET(GLSL_SOURCES)
UNSET(SPV_OUTPUT)
_compile_GLSL("GLSL/GLSL_mesh.vert" "GLSL/GLSL_mesh_vert.spv" GLSL_SOURCES SPV_OUTPUT)
_compile_GLSL("GLSL/GLSL_mesh.frag" "GLSL/GLSL_mesh_frag.spv" GLSL_SOURCES SPV_OUTPUT)
_compile_GLSL("GLSL/GLSL_mesh_lines.frag" "GLSL/GLSL_mesh_lines_frag.spv" GLSL_SOURCES SPV_OUTPUT)
_compile_GLSL("GLSL/GLSL_mesh_lines.vert" "GLSL/GLSL_mesh_lines_vert.spv" GLSL_SOURCES SPV_OUTPUT)
_compile_GLSL("GLSL/GLSL_grid.vert" "GLSL/GLSL_grid_vert.spv" GLSL_SOURCES SPV_OUTPUT)
_compile_GLSL("GLSL/GLSL_grid.frag" "GLSL/GLSL_grid_frag.spv" GLSL_SOURCES SPV_OUTPUT)
source_group(GLSL_Files FILES ${GLSL_SOURCES})

#####################################################################################
# additional files from helpers
#
#LIST(APPEND COMMON_SOURCE_FILES 
#  ${BASE_DIRECTORY}/shared_sources/nvgl/WindowInertiaCamera.h
#  ${BASE_DIRECTORY}/shared_sources/nvh/TimeSampler.h
#  ${BASE_DIRECTORY}/shared_sources/nvh/InertiaCamera.h
#  ${BASE_DIRECTORY}/shared_sources/nvmath/nvmath.inl
#  ${BASE_DIRECTORY}/shared_sources/nvmath/nvmath.h
#  ${BASE_DIRECTORY}/shared_sources/nvmath/nvmath_types.h
#  ${BASE_DIRECTORY}/shared_sources/nvh/profiler.hpp
#  ${BASE_DIRECTORY}/shared_sources/nvh/profiler.cpp
#)
#####################################################################################
# Executable
#
if(WIN32 AND NOT GLUT_FOUND)
  add_definitions(/wd4267) #remove size_t to int warning
  add_definitions(/wd4996) #remove printf warning
  add_definitions(/wd4244) #remove double to float conversion warning
  add_definitions(/wd4305) #remove double to float truncation warning
else()
  # allow gcc to be tolerant on some issues. TODO:should remove this option
  add_definitions(-fpermissive)
endif()
add_executable(${PROJNAME} ${SOURCE_FILES} ${COMMON_SOURCE_FILES} ${PACKAGE_SOURCE_FILES} ${GLSL_SOURCES})

#####################################################################################
# common source code needed for this sample
#
source_group(common FILES 
  ${COMMON_SOURCE_FILES}
)
#####################################################################################
# Linkage
#
target_link_libraries(${PROJNAME} optimized
    ${LIBRARIES_OPTIMIZED}
    ${PLATFORM_LIBRARIES}
    shared_sources
)
target_link_libraries(${PROJNAME} debug
    ${LIBRARIES_DEBUG}
    ${PLATFORM_LIBRARIES}
    shared_sources
)

#####################################################################################
# copies binaries that need to be put next to the exe files (ZLib, etc.)
#
_copy_binaries_to_target( ${PROJNAME} )

# additional copies for standalone run from install folder
install(FILES ${SPV_OUTPUT} CONFIGURATIONS Release DESTINATION "bin_${ARCH}/SPV_${PROJNAME}")
install(FILES ${SPV_OUTPUT} CONFIGURATIONS Debug DESTINATION "bin_${ARCH}_debug/SPV_${PROJNAME}")
install(FILES ${FILELISTOUT} CONFIGURATIONS Debug DESTINATION "bin_${ARCH}_debug")
install(FILES ${FILELISTOUT} CONFIGURATIONS Release DESTINATION "bin_${ARCH}")
