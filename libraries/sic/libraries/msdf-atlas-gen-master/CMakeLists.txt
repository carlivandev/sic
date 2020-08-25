cmake_minimum_required(VERSION 3.14.2)
project(msdfgen-atlas)
add_compile_options("/std:c++latest")
add_compile_options("/wd26498")

include_directories(
	msdf-atlas-gen
	artery-font-format)

file(GLOB_RECURSE msdfgen_atlas_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	"msdf-atlas-gen/*.h"
	"msdf-atlas-gen/*.hpp"
)

file(GLOB_RECURSE msdfgen_atlas_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	"msdf-atlas-gen/*.cpp"
)

add_library(msdfgen-atlas ${msdfgen_atlas_SOURCES} ${msdfgen_atlas_HEADERS} "msdf-atlas-gen/")