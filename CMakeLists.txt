cmake_minimum_required(VERSION 3.5)

project(hexxer LANGUAGES C)

find_package(SDL2 REQUIRED)

file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/src/*.c
                          ${CMAKE_SOURCE_DIR}/include/*.h)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
