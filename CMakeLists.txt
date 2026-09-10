cmake_minimum_required(VERSION 3.16)
project(memlab CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB FUENTES_NIVELES ${CMAKE_SOURCE_DIR}/src/niveles/*.cpp)
set(FUENTES_COMUNES src/util.cpp src/ui.cpp src/juego.cpp ${FUENTES_NIVELES})

add_library(memlab_nucleo STATIC ${FUENTES_COMUNES})
target_include_directories(memlab_nucleo PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_options(memlab_nucleo PRIVATE -Wall -Wextra -Wpedantic)

add_executable(memlab src/main.cpp)
target_link_libraries(memlab PRIVATE memlab_nucleo)

enable_testing()
add_executable(memlab_pruebas tests/pruebas.cpp)
target_link_libraries(memlab_pruebas PRIVATE memlab_nucleo)
add_test(NAME pruebas COMMAND memlab_pruebas)
