#include "pch.hpp"

#include "renderer.hpp"

#include <imgui_impl_glfw.h>

namespace aito
{

VulkanRenderer::VulkanRenderer(uint32_t initial_width, uint32_t initial_height, std::string window_name)
	: window_name_(std::move(window_name)), window_(initial_width, initial_height, window_name_), vulkan_context_(window_)
{
}
VulkanRenderer::~VulkanRenderer()
{
}

bool VulkanRenderer::begin_frame(float delta_time)
{
	vulkan_context_.frame_delta_time_ = delta_time;
	
	glfwPollEvents();
	if (window_.should_close())
		return false;

	// Wait for the fence of the frame we wish to write to.
	if (!vulkan_context_.get_current_frame_fence_in_flight().wait(std::numeric_limits<uint64_t>::max()))
	{
		AITO_WARN("Failure to wait for fence in flight");
		return false;
	}

	// Get the next image index
	if (!vulkan_context_.swapchain_.acquire_next_image(
				std::numeric_limits<uint64_t>::max(),
				vulkan_context_.image_avaliable_semaphores_[vulkan_context_.current_frame_],
				VK_NULL_HANDLE,
				&vulkan_context_.image_index_))
	{
		AITO_WARN("Failed to acquire next image");
		return false;
	}

	VulkanCommandBuffer &command_buffer = vulkan_context_.graphics_command_buffers_[vulkan_context_.image_index_];
	command_buffer.reset();
	command_buffer.begin(false, false, false);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(window_.get_width());
	viewport.height = static_cast<float>(window_.get_height());
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = {window_.get_width(), window_.get_height()};

	vkCmdSetViewport(command_buffer.get_handle(), 0, 1, &viewport);
	vkCmdSetScissor(command_buffer.get_handle(), 0, 1, &scissor);

	vulkan_context_.main_renderpass_.set_render_area({0, 0, window_.get_width(), window_.get_height()});

	// Begin the render pass.
	vulkan_context_.main_renderpass_.begin(command_buffer, vulkan_context_.get_frame_buffer_handle());

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::ShowDemoWindow();

	return true;
}
void VulkanRenderer::update_global_state(glm::mat4 projection, glm::mat4 view)
{
	// Get the current command buffer
	VulkanCommandBuffer &command_buffer = vulkan_context_.get_command_buffer();

	vulkan_context_.object_shader_.use();

	vulkan_context_.object_shader_.global_ubo.projection = projection;
	vulkan_context_.object_shader_.global_ubo.view = view;

	vulkan_context_.object_shader_.update_global_state(vulkan_context_.get_delta_time());
}

bool VulkanRenderer::end_frame()
{
	VulkanCommandBuffer &command_buffer = vulkan_context_.graphics_command_buffers_[vulkan_context_.image_index_];

	// ImGui rendering
	ImGui::Render();
	ImDrawData *main_draw_data = ImGui::GetDrawData();
	const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);
	// if (!main_is_minimized)
	// 	FrameRender(wd, main_draw_data);
	//
	// // Update and Render additional Platform Windows
	// if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	// {
	// 	ImGui::UpdatePlatformWindows();
	// 	ImGui::RenderPlatformWindowsDefault();
	// }
	//
	// // Present Main Platform Window
	// if (!main_is_minimized)
	// 	FramePresent(wd);

	ImGui_ImplVulkan_RenderDrawData(main_draw_data, command_buffer.get_handle());

	// ImGui render end

	vulkan_context_.main_renderpass_.end(command_buffer);

	command_buffer.end();

	// Wait for the previous frame to not use the image
	if (auto *fence = vulkan_context_.get_image_index_frame_fence_in_flight())
	{
		if (!fence->wait(std::numeric_limits<uint64_t>::max()))
		{
			AITO_WARN("Failed to wait for image index fence in flight");
			return false;
		}
	}

	// Set the fence as image in flight
	vulkan_context_.images_in_flight_[vulkan_context_.image_index_] = &vulkan_context_.get_current_frame_fence_in_flight();

	// Reset the fence
	vulkan_context_.get_current_frame_fence_in_flight().reset();

	// Submit the queue
	VkPipelineStageFlags flags[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

	command_buffer.submit(
			vulkan_context_.device_.graphics_queue_,
			vulkan_context_.image_avaliable_semaphores_[vulkan_context_.current_frame_],
			vulkan_context_.queue_complete_semaphores_[vulkan_context_.current_frame_],
			vulkan_context_.get_current_frame_fence_in_flight().get_handle(),
			flags);

	// Give the image back to the swapchain
	if (!vulkan_context_.swapchain_.present(
				vulkan_context_.device_.graphics_queue_,
				vulkan_context_.device_.present_queue_,
				vulkan_context_.queue_complete_semaphores_[vulkan_context_.current_frame_],
				vulkan_context_.image_index_))
	{
		AITO_WARN("Failed to present swap chain image");
		return false;
	}

	return true;
}

}// namespace aito