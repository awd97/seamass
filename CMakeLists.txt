cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
project(seamass C CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(OpenMP)
if(OPENMP_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS} ${CXX_EXTRA_FLAGS}")
endif()

add_subdirectory(kernel/intel) # could replace with nvidia implementation
add_subdirectory(io)
add_subdirectory(asrl)
add_subdirectory(core)
add_subdirectory(topdown)
add_subdirectory(peak)
add_subdirectory(image)
add_subdirectory(commandline)

