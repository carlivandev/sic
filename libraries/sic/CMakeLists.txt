cmake_minimum_required(VERSION 3.17.1)
project(sic)
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
    set(headers ${headers} include/sic/${header})
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

add_headers(SIC_HEADERS
	file_management.h
	system_file.h
	asset.h
	asset_model.h
	asset_material.h
	asset_texture.h
	asset_font.h
	asset_types.h
	shader_parser.h
	system_asset.h
	system_window.h
	system_input.h
	system_ui.h
	system_view.h
	material_dynamic_parameters.h
	debug_draw_helpers.h
	input.h
	gl_includes.h
	opengl_draw_strategies.h
	opengl_buffer.h
	opengl_vertex_attributes.h
	opengl_vertex_buffer_array.h
	opengl_render_target.h
	opengl_program.h
	opengl_texture.h
	opengl_uniform_block.h
	opengl_engine_uniform_blocks.h
	opengl_draw_interface_debug_lines.h
	opengl_draw_interface_instanced.h
	renderer_shape_draw_functions.h
	system_editor_view_controller.h
	component_view.h
	system_model.h
	system_renderer.h
	system_renderer_state_swapper.h
	state_render_scene.h
	component_transform.h
	color.h
	)

add_sources(SIC_SOURCES
	file_management.cpp
	system_file.cpp
	asset.cpp
	asset_model.cpp
	asset_material.cpp
	asset_texture.cpp
	asset_font.cpp
	shader_parser.cpp
	system_asset.cpp
	system_window.cpp
	system_input.cpp
	system_ui.cpp
	system_view.cpp
	material_dynamic_parameters.cpp
	debug_draw_helpers.cpp
	input.cpp
	opengl_render_target.cpp
	opengl_program.cpp
	opengl_texture.cpp
	opengl_draw_interface_debug_lines.cpp
	opengl_draw_interface_instanced.cpp
	renderer_shape_draw_functions.cpp
	system_editor_view_controller.cpp
	component_view.cpp
	system_model.cpp
	system_renderer.cpp
	system_renderer_state_swapper.cpp
	state_render_scene.cpp
	component_transform.cpp
	)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set (EXTRA_LIBS ${EXTRA_LIBS} sic_core)

set (EXTRA_LIBS ${EXTRA_LIBS} fmt)
set (EXTRA_LIBS ${EXTRA_LIBS} crossguid)
set (EXTRA_LIBS ${EXTRA_LIBS} nlohmann_json)

add_subdirectory(libraries/glfw-3.3)
set (EXTRA_LIBS ${EXTRA_LIBS} glfw)
set_target_properties (glfw PROPERTIES FOLDER sic_ext)

add_subdirectory(libraries/glm-0.9.9.6)
set (EXTRA_LIBS ${EXTRA_LIBS} glm_static)
set_target_properties (glm_static PROPERTIES FOLDER sic_ext)

add_subdirectory(libraries/glew)
set (EXTRA_LIBS ${EXTRA_LIBS} libglew_static)
set_target_properties (libglew_static PROPERTIES FOLDER sic_ext)

include_directories(
	libraries/stb
	libraries/glew/include
	libraries/glm-0.9.9.6
	../sic_core/include
	../sic_core/libraries/crossguid-master/include
	../sic_core/libraries/nlohmann-json/include
	)
	
add_library(sic STATIC ${SIC_SOURCES} ${SIC_HEADERS} include/sic)

find_package(OpenGL REQUIRED)

if (OPENGL_FOUND)
    target_include_directories(sic PUBLIC ${OPENGL_INCLUDE_DIR})
	set (EXTRA_LIBS ${EXTRA_LIBS} ${OPENGL_gl_LIBRARY})
	set (EXTRA_LIBS ${EXTRA_LIBS} ${OPENGL_glu_LIBRARY})
endif()

target_link_libraries(sic ${EXTRA_LIBS})

target_include_directories(sic PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

target_compile_options(sic PRIVATE /W4)

set_target_properties(sic PROPERTIES UNITY_BUILD ON)