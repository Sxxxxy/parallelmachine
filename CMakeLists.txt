cmake_minimum_required(VERSION 3.22.1)
project(parallelmachine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -DNDEBUG -DIL_STD -DSC_STD -DOPTV_STD")

set(OPTV_ROOT_DIR "/home/daoio01/ProgramFiles/optverse")
set(OPTV_INCLUDE_DIR "${OPTV_ROOT_DIR}/include")
set(OPTV_LIB_DIR "${OPTV_ROOT_DIR}/lib")
include_directories(${OPTV_INCLUDE_DIR})
find_library(OPTV_LIB optverse PATHS ${OPTV_LIB_DIR})
include_directories(${PROJECT_SOURCE_DIR})

add_executable(parallelmachine main.cpp TIMER.cpp)

target_link_libraries(parallelmachine ${OPTV_LIB})
