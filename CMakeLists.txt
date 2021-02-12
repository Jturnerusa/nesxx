cmake_minimum_required(VERSION 3.17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
project(nesxx)
add_executable(${PROJECT_NAME}
               cpu_debug.hxx
               rom.cxx
               rom.hxx
               bus.cxx
               bus.hxx
               cpu.cxx
               cpu.hxx
               main.cxx
               ppu.hxx
               ppu.cxx
               frame.hxx
               frame.cxx
               config.hxx)

option(HEADLESS off)
if(HEADLESS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "HEADLESS")
    include_directories(.)
    target_link_libraries(${PROJECT_NAME})
else()
    find_package(SDL2 REQUIRED)
    include_directories(. ${SDL2_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} SDL2::SDL2)
endif()
