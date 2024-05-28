#pragma once

#include "object_types.inl"
#include "pipeline.hpp"
#include "renderer/vulkan/buffer.hpp"
#include "renderer/vulkan/descriptor.hpp"
#include "shader_stage.hpp"

namespace flwfrg
{
class VulkanContext;

class VulkanObjectShader
{
public:
	GlobalUniformObject global_ubo{};

public:
	explicit VulkanObjectShader() = default;
	explicit VulkanObjectShader(VulkanContext *context, VulkanTexture* default_diffuse);
	~VulkanObjectShader() = default;

	// Not copyable or movable
	VulkanObjectShader(const VulkanObjectShader &) = delete;
	VulkanObjectShader &operator=(const VulkanObjectShader &) = delete;
	VulkanObjectShader(VulkanObjectShader && other);
	VulkanObjectShader &operator=(VulkanObjectShader && other);

	// Methods

	[[nodiscard]] const VulkanDescriptorPool &get_global_descriptor_pool() const { return global_descriptor_pool_; };
	void update_global_state(float delta_time);
	void update_object(GeometryRenderData data);

	void use();

	[[nodiscard]] uint32_t acquire_resources();
	void release_resources(uint32_t object_id);

private:
	VulkanContext *context_ = nullptr;

	std::vector<VulkanShaderStage> stages{};

	VulkanDescriptorPool global_descriptor_pool_{};
	VulkanDescriptorPool local_descriptor_pool_{};
	VulkanDescriptorSetLayout global_descriptor_set_layout_{};
	VulkanDescriptorSetLayout local_descriptor_set_layout_{};

	// One set per frame (max 3)
	std::array<VkDescriptorSet, 3> global_descriptor_sets_{};
	std::array<bool, 3> global_descriptor_updated_{};

	// Global uniform buffer
	VulkanBuffer global_uniform_buffer_{};

	// local object uniform buffer
	VulkanBuffer local_uniform_buffer_{};
	uint32_t object_uniform_buffer_index = 0;// Todo: manage a free list instead

	std::array<ObjectShaderObjectState, VULKAN_OBJECT_SHADER_MAX_OBJECT_COUNT> object_states_{}; // Todo: Make dynamic later

	// Pointers to default textures
	VulkanTexture* default_diffuse_{};
	
	VulkanPipeline pipeline_{};

	// Static members

	static constexpr uint16_t shader_stage_count = 2;
	static constexpr const char *shader_file_name = "object_shader";
};

}// namespace flwfrg