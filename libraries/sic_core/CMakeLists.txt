cmake_minimum_required(VERSION 3.17.1)
project(sic_core)
set(CMAKE_BUILD_TYPE Debug)

add_compile_options("/std:c++latest")

function(add_headers VAR)
  set(headers ${${VAR}})
  foreach (header ${ARGN})
    set(headers ${headers} include/sic/core/${header})
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

add_headers(SIC_CORE_HEADERS
	thread_context.h
	thread_temporary_storage_instance.h
	temporary_string.h
	type_restrictions.h
	bucket_allocator.h
	bucket_allocator_view.h
	colony.h
	leavable_queue.h
	double_buffer.h
	component.h
	defines.h
	event.h
	delegate.h
	object.h
	object_base.h
	serializer.h
	system.h
	thread_naming.h
	threadpool.h
	type.h
	type_index.h
	engine.h
	engine_job_scheduling.h
	scene.h
	engine_context.h
	scene_context.h
	weak_ref.h
	update_list.h
	magic_enum.h
	logger.h
	random.h
	async_result.h
	)

add_sources(SIC_CORE_SOURCES
	thread_context.cpp
	thread_temporary_storage_instance.cpp
	temporary_string.cpp
	component.cpp
	serializer.cpp
	system.cpp
	threadpool.cpp
	engine.cpp
	engine_job_scheduling.cpp
	scene.cpp
	engine_context.cpp
	scene_context.cpp
	logger.cpp
	random.cpp
	async_result.cpp
	)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(libraries/fmt)
set (EXTRA_LIBS ${EXTRA_LIBS} fmt)
set_target_properties (fmt PROPERTIES FOLDER sic_ext)

add_subdirectory(libraries/crossguid-master)
set (EXTRA_LIBS ${EXTRA_LIBS} crossguid)
set_target_properties (crossguid PROPERTIES FOLDER sic_ext)

add_subdirectory(libraries/nlohmann-json)
set (EXTRA_LIBS ${EXTRA_LIBS} nlohmann_json)

include_directories(
	libraries/crossguid-master/include
	libraries/nlohmann-json/include)
	
add_library(sic_core STATIC ${SIC_CORE_SOURCES} ${SIC_CORE_HEADERS} include/sic/core)

target_link_libraries(sic_core ${EXTRA_LIBS})

target_include_directories(sic_core PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

  target_compile_options(sic_core PRIVATE /W4)

set_target_properties(sic_core PROPERTIES UNITY_BUILD ON)