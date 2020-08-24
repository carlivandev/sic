cmake_minimum_required(VERSION 3.14.2)
project(sic_test_game)
add_compile_options("/std:c++latest")
add_compile_options("/wd26498")

add_subdirectory(libraries/sic)
set (TEST_EXTRA_LIBS ${TEST_EXTRA_LIBS} sic)

include_directories(include)


include_directories(
	libraries/sic/libraries/glm-0.9.9.6)

file(GLOB SOURCES "src/*.cpp")
 
add_executable(sic_test_game ${SOURCES})

target_link_libraries(sic_test_game ${TEST_EXTRA_LIBS})

set_target_properties(sic_test_game PROPERTIES 
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
	
target_compile_options(sic_test_game PRIVATE /W4)