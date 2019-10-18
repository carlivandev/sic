cmake_minimum_required(VERSION 3.14.2)
project(impuls)
set(CMAKE_BUILD_TYPE Debug)
add_compile_options("/std:c++latest")

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_definitions(
	-DTW_STATIC
	-DTW_NO_LIB_PRAGMA
	-DTW_NO_DIRECT3D
	-DGLEW_STATIC
)

function(add_headers VAR)
  set(headers ${${VAR}})
  foreach (header ${ARGN})
    set(headers ${headers} include/impuls/${header})
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

add_headers(IMPULS_HEADERS
	bucket_allocator.h
	bucket_allocator_view.h
	component.h
	defines.h
	event.h
	object.h
	object_base.h
	pch.h
	scheduler.h
	serializer.h
	snapshot.h
	system.h
	thread_naming.h
	threadpool.h
	type.h
	type_index.h
	world.h
	world_context.h
	file_management.h
	asset_management.h
	asset_types.h
	system_window.h
	system_input.h
	input.h
	gl_includes.h
	system_editor_view_controller.h
	view.h
	system_model.h
	transform.h
	)

add_sources(IMPULS_SOURCES
	component.cpp
	pch.cpp
	scheduler.cpp
	threadpool.cpp
	world.cpp
	world_context.cpp
	file_management.cpp
	asset_management.cpp
	system_window.cpp
	system_input.cpp
	input.cpp
	system_editor_view_controller.cpp
	system_model.cpp)

add_subdirectory(libraries/fmt)
set (EXTRA_LIBS ${EXTRA_LIBS} fmt)

add_subdirectory(libraries/glfw-3.3)
set (EXTRA_LIBS ${EXTRA_LIBS} glfw)

add_subdirectory(libraries/glm-0.9.9.6)
set (EXTRA_LIBS ${EXTRA_LIBS} glm_static)

add_subdirectory(libraries/glew)
set (EXTRA_LIBS ${EXTRA_LIBS} libglew_static)

add_subdirectory(libraries/assimp-master)
set (EXTRA_LIBS ${EXTRA_LIBS} assimp)

include_directories(
	libraries/stb
	libraries/glew/include
	libraries/glm-0.9.9.6)
	
add_library(impuls STATIC ${IMPULS_SOURCES} ${IMPULS_HEADERS} include/impuls)

find_package(OpenGL REQUIRED)

if (OPENGL_FOUND)
    target_include_directories(impuls PUBLIC ${OPENGL_INCLUDE_DIR})
	set (EXTRA_LIBS ${EXTRA_LIBS} ${OPENGL_gl_LIBRARY})
	set (EXTRA_LIBS ${EXTRA_LIBS} ${OPENGL_glu_LIBRARY})
endif()

target_link_libraries(impuls ${EXTRA_LIBS})

target_include_directories(impuls PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

  target_compile_options(impuls PRIVATE /W4)
  target_compile_options(assimp PRIVATE /W0)