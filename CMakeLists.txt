cmake_minimum_required(VERSION 3.20)

project(tracer)

set(IMGUI_DIR ../vendor/imgui)
set(IMGUI_SOURCES ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_tables.cpp ${IMGUI_DIR}/imgui_widgets.cpp)

# Find the libraries using https://github.com/SaschaWillems/Vulkan/blob/master/CMakeLists.txt
# Note: MIT License
# Use FindVulkan module added with CMAKE 3.7
if (NOT CMAKE_VERSION VERSION_LESS 3.7.0)
	message(STATUS "Using module to find Vulkan")
	find_package(Vulkan)
endif ()
if (UNIX AND NOT APPLE)
	set(LINUX TRUE)
endif ()

if (WIN32)
	if (NOT Vulkan_FOUND)
		find_library(Vulkan_LIBRARY NAMES vulkan-1 vulkan PATHS ${CMAKE_SOURCE_DIR}/libs/vulkan)
		if (Vulkan_LIBRARY)
			set(Vulkan_FOUND ON)
			message("Using bundled Vulkan library version")
		endif ()
	endif ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
elseif (LINUX)
	if (NOT Vulkan_FOUND)
		find_library(Vulkan_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/libs/vulkan" REQUIRED)
		if (Vulkan_LIBRARY)
			set(Vulkan_FOUND ON)
			message("Using bundled Vulkan library version")
		endif ()
	endif ()
endif (WIN32)

# add subdirectories
add_subdirectory(vendor/glfw)
add_subdirectory(vendor/spdlog)

add_subdirectory(src)