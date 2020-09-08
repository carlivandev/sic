cmake_minimum_required(VERSION 3.17.1)
project(sic_editor)
set(CMAKE_BUILD_TYPE Debug)

add_compile_options("/std:c++latest")

function(add_headers VAR)
  set(headers ${${VAR}})
  foreach (header ${ARGN})
    set(headers ${headers} include/sic_editor/${header})
  endforeach()
  set(${VAR} ${headers} PARENT_SCOPE)
endfunction()

function(add_sources VAR)
  set(sources ${${VAR}})
  foreach (source ${ARGN})
    set(sources ${sources} src/${source})
  endforeach()
  set(${VAR} ${sources} PARENT_SCOPE)
endfunction()

add_headers(SIC_EDITOR_HEADERS
	asset_import.h
	)

add_sources(SIC_EDITOR_SOURCES
	asset_import.cpp
	)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set (EXTRA_LIBS ${EXTRA_LIBS} sic_core)
set (EXTRA_LIBS ${EXTRA_LIBS} sic)

add_subdirectory(libraries/assimp-master)
set (EXTRA_LIBS ${EXTRA_LIBS} assimp)
set_target_properties (assimp PROPERTIES FOLDER sic_ext)
set_target_properties (IrrXML PROPERTIES FOLDER sic_ext)
set_target_properties (UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER sic_ext)
set_target_properties (zlib PROPERTIES FOLDER sic_ext)
set_target_properties (zlibstatic PROPERTIES FOLDER sic_ext)

set (FREETYPE_LIBRARY ${PROJECT_BINARY_DIR}/libraries/freetype-2.10.2/${CMAKE_BUILD_TYPE}/freetyped.lib)
set (FREETYPE_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/libraries/freetype-2.10.2/include)

add_subdirectory(libraries/freetype-2.10.2)
set_target_properties (freetype PROPERTIES FOLDER sic_ext)
set (EXTRA_LIBS ${EXTRA_LIBS} freetype)

add_subdirectory(libraries/msdfgen-master)
set (EXTRA_LIBS ${EXTRA_LIBS} msdfgen)
set (EXTRA_LIBS ${EXTRA_LIBS} msdfgen-ext)
set_target_properties (msdfgen PROPERTIES FOLDER sic_ext)
set_target_properties (msdfgen-ext PROPERTIES FOLDER sic_ext)

target_include_directories(msdfgen PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/libraries/freetype-2.10.2/include>)

add_subdirectory(libraries/msdf-atlas-gen-master)
set (EXTRA_LIBS ${EXTRA_LIBS} msdfgen-atlas)
set_target_properties (msdfgen-atlas PROPERTIES FOLDER sic_ext)

target_include_directories(msdfgen-atlas PUBLIC  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/libraries/msdfgen-master>)
target_include_directories(msdfgen-atlas PUBLIC  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/libraries/msdfgen-master/include>)

include_directories(
	libraries/stb
	libraries/freetype-2.10.2/include
	libraries/msdfgen-master/include
	libraries/msdf-atlas-gen-master
	../sic/libraries/glew/include
	../sic/libraries/glm-0.9.9.6)
	
add_library(sic_editor STATIC ${SIC_EDITOR_SOURCES} ${SIC_EDITOR_HEADERS} include/sic_editor)

target_link_libraries(sic_editor ${EXTRA_LIBS})

target_include_directories(sic_editor PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

  target_compile_options(sic_editor PRIVATE /W4)
  target_compile_options(assimp PRIVATE /W0)

set_target_properties(sic_editor PROPERTIES UNITY_BUILD ON)