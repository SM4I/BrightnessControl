file(GLOB source_files "./*.cpp")
file(GLOB include_files "./*.h")

add_library(imgui ${source_files} ${include_files})

target_include_directories(imgui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(imgui glfw)
