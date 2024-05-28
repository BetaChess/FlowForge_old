#pragma once

#include "buffer.hpp"
#include "device.hpp"
#include "imgui_instance.hpp"
#include "render_pass.hpp"
#include "shaders/object_shader.hpp"
#include "shaders/vertex.hpp"
#include "swapchain.hpp"
#include "vulkan_fence.hpp"
#include "window.hpp"

#include <imgui_impl_vulkan.h>

namespace flwfrg
{
class VulkanRenderer;
class VulkanImage;

class VulkanInstance
{
public:
	VulkanInstance();
	~VulkanInstance();

	constexpr operator VkInstance() const
	{
		return instance_;
	}

private:
	VkInstance instance_;


	[[nodiscard]] static bool validation_layers_supported(const std::vector<const char *> &layers);
	[[nodiscard]] static std::vector<const char *> get_required_extensions();
	static void check_glfw_required_instance_extensions();

	const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
};

class VulkanDebugMessenger
{
public:
	explicit VulkanDebugMessenger(const VulkanInstance &instance);
	~VulkanDebugMessenger();

private:
	const VulkanInstance &instance_;

	VkDebugUtilsMessengerEXT debug_messenger_;
};

class VulkanSurface
{
public:
	VulkanSurface(const VulkanInstance &instance, const Window &window);
	~VulkanSurface();

	constexpr operator VkSurfaceKHR() const
	{
		return surface_;
	}

private:
	const VulkanInstance &instance_;
	const Window &window_;
	VkSurfaceKHR surface_;
};

class VulkanContext
{
public:
	VulkanContext(Window &window);
	~VulkanContext();

	[[nodiscard]] inline const Window &get_window() const noexcept { return window_; }
	inline VkDevice logical_device() { return device_.logical_device_; };
	[[nodiscard]] inline const VulkanDevice &vulkan_device() const { return device_; };
	[[nodiscard]] inline uint32_t image_index() const { return image_index_; };
	inline VulkanCommandBuffer &get_command_buffer() { return graphics_command_buffers_[image_index_]; };
	inline VulkanFence &get_current_frame_fence_in_flight() { return in_flight_fences_[current_frame_]; };
	inline VulkanFence *get_image_index_frame_fence_in_flight() { return images_in_flight_[image_index_]; };
	inline VkFramebuffer get_frame_buffer_handle() { return swapchain_.frame_buffers_[image_index_].get_handle(); };
	inline const VulkanRenderpass &get_renderpass() { return main_renderpass_; };
	inline const VulkanSwapchain &get_swapchain() { return swapchain_; };
	[[nodiscard]] inline float get_delta_time() const { return frame_delta_time_; };

	void populate_imgui_init_info(ImGui_ImplVulkan_InitInfo &out_init_info);
	[[nodiscard]] VkRenderPass get_main_render_pass() const { return main_renderpass_.get_handle(); };

	void init_imgui();
	void set_default_diffuse_texture(VulkanTexture* new_default);

	int32_t find_memory_index(uint32_t type_filter, VkMemoryPropertyFlags memory_flags);

private:
#ifdef NDEBUG
	static constexpr bool enable_validation_layers_ = false;
#else
	static constexpr bool enable_validation_layers_ = true;
#endif
	Window &window_;
	VulkanInstance instance_{};
#ifndef NDEBUG
	VulkanDebugMessenger debugMessenger_{instance_};
#endif
	float frame_delta_time_;
	
	VulkanSurface surface_{instance_, window_};
	VulkanDevice device_{this};

	VulkanSwapchain swapchain_{this};
	VulkanRenderpass main_renderpass_{
			this,
			{0, 0, window_.get_width(), window_.get_height()},
			{0, 0, 0.2f, 1.0f},
			1.0f,
			0};

	VulkanBuffer vertex_buffer_{this,
								sizeof(Vertex3d) * 1024 * 1024,
								static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
								VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
								false};
	VulkanBuffer index_buffer_{this,
							   sizeof(uint32_t) * 1024 * 1024,
							   static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
							   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
							   false};

	std::vector<VulkanCommandBuffer> graphics_command_buffers_{};

	std::vector<VkSemaphore> image_avaliable_semaphores_;
	std::vector<VkSemaphore> queue_complete_semaphores_;

	std::vector<VulkanFence> in_flight_fences_;
	std::vector<VulkanFence *> images_in_flight_;

	uint32_t image_index_;
	uint32_t current_frame_;

	VulkanObjectShader object_shader_{};

	ImGuiInstance imgui_instance_{};

	///// Private methods

	void create_command_buffers();
	void regenerate_framebuffers();

	// Friend classes
	friend VulkanDevice;
	friend VulkanSwapchain;
	friend VulkanInstance;
	friend VulkanImage;
	friend VulkanRenderpass;
	friend VulkanCommandBuffer;
	friend VulkanFrameBuffer;
	friend VulkanFence;
	friend VulkanRenderer;

	// Static

	static void resize_callback(void *context);
};

}// namespace flwfrg