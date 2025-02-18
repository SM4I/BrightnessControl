cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project("BSA")
add_subdirectory("glfw")
add_subdirectory("imgui")


add_executable(BSA WIN32 src/main.cpp)
include_directories(BSA ${CMAKE_SOURCE_DIR}/glfw/include)
include_directories(BSA ${CMAKE_SOURCE_DIR}/imgui)
target_link_libraries(BSA opengl32 dxva2 imgui glfw)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "BSA")
if(WIN32)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
			"${CMAKE_SOURCE_DIR}/src/logo.ico"
			"${CMAKE_SOURCE_DIR}/src/Roboto_medium.ttf"
			$<TARGET_FILE_DIR:BSA>)
	target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:/INCREMENTAL>)
	target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:/ZI>)
	target_sources(BSA PRIVATE src/app_icon.rc)
endif()
