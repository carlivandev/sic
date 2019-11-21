cmake_minimum_required(VERSION 3.1)

##
## PROJECT
## name and version
##
project(plf_colony VERSION 3.7.0 LANGUAGES CXX)

set(PLF_COLONY_TARGET_NAME               ${PROJECT_NAME})
set(PLF_COLONY_INCLUDE_BUILD_DIR "${PROJECT_SOURCE_DIR}/include/")
    message(STATUS "Using the single-header code from ${PLF_COLONY_INCLUDE_BUILD_DIR}")

##
## TARGET
## create target and add include path
##
add_library(${PLF_COLONY_TARGET_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PLF_COLONY_TARGET_NAME} ALIAS ${PLF_COLONY_TARGET_NAME})
if (${CMAKE_VERSION} VERSION_LESS "3.8.0")
    target_compile_features(${PLF_COLONY_TARGET_NAME} INTERFACE cxx_range_for)
else()
    target_compile_features(${PLF_COLONY_TARGET_NAME} INTERFACE cxx_std_11)
endif()

target_include_directories(
    ${PLF_COLONY_TARGET_NAME}
    INTERFACE
    $<BUILD_INTERFACE:${PLF_COLONY_INCLUDE_BUILD_DIR}>
    $<INSTALL_INTERFACE:include>
)