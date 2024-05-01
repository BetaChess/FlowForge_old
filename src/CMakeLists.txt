cmake_minimum_required(VERSION 3.20)

## Compile C libary
set(STB_INTERNAL_LIB_NAME stb_internal)

add_library(${STB_INTERNAL_LIB_NAME} STATIC renderer/stb_image.c)

target_include_directories(${STB_INTERNAL_LIB_NAME}
						   PUBLIC ../vendor/stb
)

## Configuration for the C++ Project
set(SOURCES
	# Dear ImGui sources
	${IMGUI_SOURCES}
	# Project sources
	pch.hpp
	main.cpp
	core/logger.hpp
	core/logger.cpp
	renderer/vulkan/window.cpp
	renderer/vulkan/window.hpp
	application.hpp
	application.cpp
	renderer/vulkan/vulkan_context.hpp
	renderer/glfw_context.hpp
	renderer/glfw_context.cpp
	renderer/vulkan/vulkan_context.cpp
	renderer/vulkan/device.hpp
	renderer/vulkan/device.cpp
	renderer/vulkan/swapchain.hpp
	renderer/vulkan/swapchain.cpp
	renderer/vulkan/image.hpp
	renderer/vulkan/image.cpp
	renderer/vulkan/render_pass.hpp
	renderer/vulkan/render_pass.cpp
	renderer/vulkan/command_buffer.hpp
	renderer/vulkan/command_buffer.cpp
	renderer/vulkan/frame_buffer.hpp
	renderer/vulkan/frame_buffer.cpp
	renderer/vulkan/vulkan_fence.hpp
	renderer/vulkan/vulkan_fence.cpp
	renderer/vulkan/renderer.hpp
	renderer/vulkan/renderer.cpp
	renderer/vulkan/imgui_instance.hpp
	renderer/vulkan/imgui_instance.cpp
	renderer/vulkan/shaders/object_shader.hpp
	renderer/vulkan/shaders/object_shader.cpp
	renderer/vulkan/shaders/shader_stage.hpp
	renderer/vulkan/shaders/pipeline.hpp
	renderer/vulkan/shaders/shader_utils.hpp
	renderer/vulkan/shaders/shader_stage.cpp
	renderer/vulkan/shaders/pipeline.cpp
	renderer/vulkan/shaders/vertex.hpp
	renderer/vulkan/buffer.hpp
	renderer/vulkan/buffer.cpp
	renderer/vulkan/shaders/object_types.inl
	renderer/vulkan/descriptor.hpp
	renderer/vulkan/descriptor.cpp
	renderer/vulkan/resources/VulkanTexture.hpp
	renderer/vulkan/resources/VulkanTexture.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

# Precompiled header
target_precompile_headers(${PROJECT_NAME}
						  PUBLIC pch.hpp
)

set_target_properties(${PROJECT_NAME}
					  PROPERTIES
					  CXX_STANDARD 20
					  CXX_STANDARD_REQUIRED YES
					  CXX_EXTENSIONS NO
)

target_include_directories(${PROJECT_NAME}
						   PUBLIC $ENV{VULKAN_SDK}/include/
						   PUBLIC ../vendor/glfw/include/
						   PUBLIC ${IMGUI_DIR}/backends/
						   PUBLIC ${IMGUI_DIR}
						   PUBLIC ../vendor/spdlog/include
						   PUBLIC ../vendor/stb
)

target_link_directories(${PROJECT_NAME}
						PRIVATE ../vendor/glfw/src
)

target_link_libraries(${PROJECT_NAME}
					  glfw
					  ${Vulkan_LIBRARY}
					  ${STB_INTERNAL_LIB_NAME}
)

include_directories(.)


############## Build SHADERS #######################

# Find all vertex and fragment sources within shaders directory
# taken from VBlancos vulkan tutorial
# https://github.com/vblanco20-1/vulkan-guide/blob/all-chapters/CMakeLists.txt
find_program(GLSL_VALIDATOR glslangValidator HINTS
			 ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}
			 /usr/bin
			 /usr/local/bin
			 ${VULKAN_SDK_PATH}/Bin
			 ${VULKAN_SDK_PATH}/Bin32
			 $ENV{VULKAN_SDK}/Bin/
			 $ENV{VULKAN_SDK}/Bin32/
)

# get all .vert and .frag files in shaders directory
file(GLOB_RECURSE GLSL_SOURCE_FILES
	 "${PROJECT_SOURCE_DIR}/assets/shaders/*.frag"
	 "${PROJECT_SOURCE_DIR}/assets/shaders/*.vert"
)

message("${GLSL_SOURCE_FILES}")

foreach (GLSL ${GLSL_SOURCE_FILES})
	get_filename_component(FILE_NAME ${GLSL} NAME)
	set(SPIRV "${PROJECT_SOURCE_DIR}/assets/shaders/${FILE_NAME}.spv")
	add_custom_command(
			OUTPUT ${SPIRV}
			COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
			DEPENDS ${GLSL})
	list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach (GLSL)

add_custom_target(
		${PROJECT_NAME}_Shaders
		SOURCES ${GLSL_SOURCE_FILES}
		DEPENDS ${SPIRV_BINARY_FILES}
)

add_custom_command(
		TARGET ${PROJECT_NAME}_Shaders POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		${PROJECT_SOURCE_DIR}/assets/shaders ${CMAKE_CURRENT_BINARY_DIR}/assets/shaders
		COMMENT "Copying shaders" VERBATIM
)

add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_Shaders)

