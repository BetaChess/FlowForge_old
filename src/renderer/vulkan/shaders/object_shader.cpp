#include "pch.hpp"

#include "object_shader.hpp"

#include <utility>

#include "../vulkan_context.hpp"
#include "renderer/vulkan/resources/VulkanTexture.hpp"
#include "vertex.hpp"

namespace flwfrg
{
VulkanObjectShader::VulkanObjectShader(VulkanContext *context, VulkanTexture *default_diffuse)
	: context_(context), default_diffuse_(default_diffuse)
{
	assert(context != nullptr);

	VkShaderStageFlagBits stage_types[shader_stage_count] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

	// Iterate over each stage
	for (uint16_t i = 0; i < shader_stage_count; i++)
	{
		// Create a shader stage
		std::optional<VulkanShaderStage> stage = VulkanShaderStage::create_shader_module(context_, shader_file_name, stage_types[i]);

		if (!stage.has_value())
		{
			throw std::runtime_error("Failed to create shader stage");
		}

		stages.emplace_back(std::move(stage.value()));
	}

	// Global descriptors
	VkDescriptorSetLayoutBinding global_ubo_layout_binding{};
	global_ubo_layout_binding.binding = 0;
	global_ubo_layout_binding.descriptorCount = 1;
	global_ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	global_ubo_layout_binding.pImmutableSamplers = nullptr;
	global_ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = 1;
	layout_info.pBindings = &global_ubo_layout_binding;
	global_descriptor_set_layout_ = VulkanDescriptorSetLayout(context_, layout_info);

	// Global descriptor pool
	VkDescriptorPoolSize global_pool_size{};
	global_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	global_pool_size.descriptorCount = context_->get_swapchain().get_image_count();

	// Local/object descriptors
	const uint32_t local_sampler_count = 1;
	std::array<VkDescriptorType, VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT> descriptor_types = {
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER};
	std::array<VkDescriptorSetLayoutBinding, VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT> bindings{};
	for (uint32_t i = 0; i < VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT; i++)
	{
		bindings[i].binding = i;
		bindings[i].descriptorCount = 1;
		bindings[i].descriptorType = descriptor_types[i];
		bindings[i].pImmutableSamplers = nullptr;
		bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	}

	// Create descriptor set layout
	VkDescriptorSetLayoutCreateInfo local_layout_create_info{};
	local_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	local_layout_create_info.bindingCount = VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT;
	local_layout_create_info.pBindings = bindings.data();
	// Create and check result
	local_descriptor_set_layout_ = VulkanDescriptorSetLayout(context_, local_layout_create_info);

	// Local layout pool
	std::array<VkDescriptorPoolSize, VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT> local_pool_sizes{};
	local_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	local_pool_sizes[0].descriptorCount = VULKAN_OBJECT_SHADER_MAX_OBJECT_COUNT;

	local_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	local_pool_sizes[1].descriptorCount = local_sampler_count * VULKAN_OBJECT_SHADER_MAX_OBJECT_COUNT;

	VkDescriptorPoolCreateInfo local_pool_info{};
	local_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	local_pool_info.poolSizeCount = local_pool_sizes.size();
	local_pool_info.pPoolSizes = local_pool_sizes.data();
	local_pool_info.maxSets = VULKAN_OBJECT_SHADER_MAX_OBJECT_COUNT;

	// Create local/object descriptor pool
	local_descriptor_pool_ = VulkanDescriptorPool(context_, local_pool_info);

	// Image sampler pool
	VkDescriptorPoolSize image_sampler_pool_size{};
	image_sampler_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	image_sampler_pool_size.descriptorCount = 1;

	std::vector<VkDescriptorPoolSize> pool_sizes = {
			global_pool_size,
			image_sampler_pool_size};

	VkDescriptorPoolCreateInfo global_pool_info{};
	global_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	global_pool_info.poolSizeCount = pool_sizes.size();
	global_pool_info.pPoolSizes = pool_sizes.data();
	global_pool_info.maxSets = context_->get_swapchain().get_image_count() + 1;
	global_pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	global_descriptor_pool_ = VulkanDescriptorPool(context_, global_pool_info);

	// Pipeline creation
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(context_->get_window().get_width());
	viewport.height = static_cast<float>(context_->get_window().get_height());
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = {context_->get_window().get_width(), context_->get_window().get_height()};

	// Attributes
	auto binding_description = Vertex3d::get_binding_description();

	// Descriptor set layouts
	std::vector<VkDescriptorSetLayout> descriptor_set_layouts = {
			global_descriptor_set_layout_.get(),
			local_descriptor_set_layout_.get()};


	// Stages
	std::vector<VkPipelineShaderStageCreateInfo> stage_create_infos{};
	stage_create_infos.resize(shader_stage_count);
	for (uint16_t i = 0; i < shader_stage_count; i++)
	{
		stage_create_infos[i] = stages[i].get_shader_stage_create_info();
	}

	// Create the pipeline
	auto created_pipeline = VulkanPipeline::create_pipeline(context_,
															context_->get_renderpass(),
															binding_description,
															descriptor_set_layouts,
															stage_create_infos,
															viewport,
															scissor,
															false);

	// Check that it was created
	if (!created_pipeline.has_value())
	{
		throw std::runtime_error("Failed to create pipeline");
	}

	pipeline_ = std::move(created_pipeline.value());

	// Create global uniform buffer
	global_uniform_buffer_ = VulkanBuffer(context_, sizeof(GlobalUniformObject) * 3,
										  static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT),
										  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
										  true);

	// Allocate global descriptor sets
	std::vector<VkDescriptorSetLayout> global_layouts = {
			global_descriptor_set_layout_.get(),
			global_descriptor_set_layout_.get(),
			global_descriptor_set_layout_.get()};

	VkDescriptorSetAllocateInfo allocate_info{};
	allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocate_info.descriptorPool = global_descriptor_pool_.get();
	allocate_info.descriptorSetCount = global_layouts.size();
	allocate_info.pSetLayouts = global_layouts.data();
	// Allocate it
	if (vkAllocateDescriptorSets(context_->logical_device(), &allocate_info, global_descriptor_sets_.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate descriptor sets");
	}

	// Create local uniform buffer
	local_uniform_buffer_ = VulkanBuffer(context_, sizeof(LocalUniformObject) * VULKAN_OBJECT_SHADER_MAX_OBJECT_COUNT,
										 static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT),
										 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
										 true);
}

VulkanObjectShader::VulkanObjectShader(VulkanObjectShader &&other)
	: context_(other.context_),
	  stages(std::move(other.stages)),
	  global_descriptor_pool_(std::move(other.global_descriptor_pool_)),
	  local_descriptor_pool_(std::move(other.local_descriptor_pool_)),
	  global_descriptor_set_layout_(std::move(other.global_descriptor_set_layout_)),
	  local_descriptor_set_layout_(std::move(other.local_descriptor_set_layout_)),
	  global_descriptor_sets_(std::move(other.global_descriptor_sets_)),
	  global_descriptor_updated_(std::move(other.global_descriptor_updated_)),
	  global_uniform_buffer_(std::move(other.global_uniform_buffer_)),
	  local_uniform_buffer_(std::move(other.local_uniform_buffer_)),
	  object_uniform_buffer_index(other.object_uniform_buffer_index),
	  object_states_(std::move(other.object_states_)),
	  default_diffuse_(other.default_diffuse_),
	  pipeline_(std::move(other.pipeline_))
{
	other.context_ = nullptr;
	other.default_diffuse_ = nullptr;
}
VulkanObjectShader &VulkanObjectShader::operator=(VulkanObjectShader &&other)
{
	if (this != &other)
	{
		context_ = other.context_;
		stages = std::move(other.stages);
		global_descriptor_pool_ = std::move(other.global_descriptor_pool_);
		local_descriptor_pool_ = std::move(other.local_descriptor_pool_);
		global_descriptor_set_layout_ = std::move(other.global_descriptor_set_layout_);
		local_descriptor_set_layout_ = std::move(other.local_descriptor_set_layout_);
		global_descriptor_sets_ = std::move(other.global_descriptor_sets_);
		global_descriptor_updated_ = std::move(other.global_descriptor_updated_);
		global_uniform_buffer_ = std::move(other.global_uniform_buffer_);
		local_uniform_buffer_ = std::move(other.local_uniform_buffer_);
		object_uniform_buffer_index = other.object_uniform_buffer_index;
		object_states_ = std::move(other.object_states_);
		default_diffuse_ = other.default_diffuse_;
		pipeline_ = std::move(other.pipeline_);

		other.context_ = nullptr;
		other.default_diffuse_ = nullptr;
	}
	return *this;
}

void VulkanObjectShader::update_global_state(float delta_time)
{
	VulkanCommandBuffer &command_buffer = context_->get_command_buffer();
	auto image_index = context_->image_index();

	VkDescriptorSet global_descriptor = global_descriptor_sets_[image_index];

	if (!global_descriptor_updated_[image_index])
	{
		// Configure the descriptors for the given index
		uint32_t range = sizeof(GlobalUniformObject);
		uint64_t offset = sizeof(GlobalUniformObject) * image_index;

		// Copy data to buffer
		global_uniform_buffer_.load_data(&global_ubo, offset, range, 0);

		VkDescriptorBufferInfo buffer_info{};
		buffer_info.buffer = global_uniform_buffer_.get_handle();
		buffer_info.offset = offset;
		buffer_info.range = range;

		// Update the descriptor sets

		// Global ubo
		VkWriteDescriptorSet ubo_descriptor_write{};
		ubo_descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		ubo_descriptor_write.dstSet = global_descriptor;
		ubo_descriptor_write.dstBinding = 0;
		ubo_descriptor_write.dstArrayElement = 0;
		ubo_descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		ubo_descriptor_write.descriptorCount = 1;
		ubo_descriptor_write.pBufferInfo = &buffer_info;

		vkUpdateDescriptorSets(context_->logical_device(), 1, &ubo_descriptor_write, 0, nullptr);
		global_descriptor_updated_[image_index] = true;
	}

	// Bind descriptor set
	vkCmdBindDescriptorSets(command_buffer.get_handle(),
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							pipeline_.layout(),
							0,
							1,
							&global_descriptor,
							0,
							nullptr);
}

void VulkanObjectShader::update_object(GeometryRenderData data)
{
	VulkanCommandBuffer &command_buffer = context_->get_command_buffer();
	auto image_index = context_->image_index();

	vkCmdPushConstants(command_buffer.get_handle(), pipeline_.layout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &data.model);

	// Obtain material data
	ObjectShaderObjectState *object_state = &object_states_[data.object_id];
	VkDescriptorSet object_descriptor_set = object_state->descriptor_sets[image_index];

	// Todo: check if it actually needs to update
	std::array<VkWriteDescriptorSet, VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT> descriptor_writes{};
	uint32_t descriptor_count = 0;
	uint32_t descriptor_index = 0;

	// Descriptor 0
	uint32_t range = sizeof(LocalUniformObject);
	uint64_t offset = sizeof(LocalUniformObject) * data.object_id;
	LocalUniformObject lbo;

	// Todo: get diffuse color from material

	local_uniform_buffer_.load_data(&lbo, offset, range, 0);

	// Only update if the descriptor hasn't already been updated
	if (object_state->descriptor_states[descriptor_index].generations[image_index] == std::numeric_limits<uint32_t>::max())
	{
		VkDescriptorBufferInfo buffer_info{};
		buffer_info.buffer = local_uniform_buffer_.get_handle();
		buffer_info.offset = offset;
		buffer_info.range = range;

		descriptor_writes[descriptor_count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[descriptor_count].dstSet = object_descriptor_set;
		descriptor_writes[descriptor_count].dstBinding = descriptor_index;
		descriptor_writes[descriptor_count].dstArrayElement = 0;
		descriptor_writes[descriptor_count].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_writes[descriptor_count].descriptorCount = 1;
		descriptor_writes[descriptor_count].pBufferInfo = &buffer_info;

		descriptor_count++;
		object_state->descriptor_states[descriptor_index].generations[image_index] = 1;
	}
	descriptor_index++;

	const uint32_t sampler_count = 1;
	std::array<VkDescriptorImageInfo, 1> image_infos;
	for (uint32_t sampler_index = 0; sampler_index < sampler_count; sampler_index++)
	{
		VulkanTexture* texture = data.textures[sampler_index];
		auto& descriptor_generation = object_state->descriptor_states[descriptor_index].generations[image_index];

		if (texture->get_generation() == std::numeric_limits<uint32_t>::max())
		{
			texture = default_diffuse_;

			descriptor_generation = std::numeric_limits<uint32_t>::max();
		}

		if (texture && (descriptor_generation != texture->get_generation() || descriptor_generation == std::numeric_limits<uint32_t>::max()))
		{
			image_infos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			image_infos[0].imageView = texture->get_image().get_image_view();
			image_infos[0].sampler = texture->get_sampler();
			
			descriptor_writes[descriptor_count].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_writes[descriptor_count].dstSet = object_descriptor_set;
			descriptor_writes[descriptor_count].dstBinding = descriptor_index;
			descriptor_writes[descriptor_count].dstArrayElement = 0;
			descriptor_writes[descriptor_count].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptor_writes[descriptor_count].descriptorCount = image_infos.size();
			descriptor_writes[descriptor_count].pImageInfo = image_infos.data();
			
			descriptor_count++;
			object_state->descriptor_states[descriptor_index].generations[image_index] = texture->get_generation();

			// If not using default texture, sync the generation. 
			if (texture->get_generation() != std::numeric_limits<uint32_t>::max()) {
				descriptor_generation = texture->get_generation();
			}
			descriptor_index++;
		}
	}

	if (descriptor_count > 0)
	{
		vkUpdateDescriptorSets(context_->logical_device(), descriptor_count, descriptor_writes.data(), 0, nullptr);
	}

	// Bind descriptor set
	vkCmdBindDescriptorSets(command_buffer.get_handle(),
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							pipeline_.layout(),
							1,
							1,
							&object_descriptor_set,
							0,
							nullptr);
}

void VulkanObjectShader::use()
{
	pipeline_.bind(context_->get_command_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS);
}

uint32_t VulkanObjectShader::acquire_resources()
{
	// TODO: free list
	uint32_t object_id = object_uniform_buffer_index;
	object_uniform_buffer_index++;

	auto &object_state = object_states_[object_id];
	for (VulkanDescriptorState &descriptor_state: object_state.descriptor_states)
	{
		for (uint32_t &generation: descriptor_state.generations)
		{
			generation = std::numeric_limits<uint32_t>::max();
		}
	}

	// Allocate descriptor sets
	std::array<VkDescriptorSetLayout, 3> layouts = {
			local_descriptor_set_layout_.get(),
			local_descriptor_set_layout_.get(),
			local_descriptor_set_layout_.get()};

	VkDescriptorSetAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = local_descriptor_pool_.get();
	alloc_info.descriptorSetCount = layouts.size();
	alloc_info.pSetLayouts = layouts.data();

	// Do the allocation and check the result
	if (vkAllocateDescriptorSets(context_->logical_device(), &alloc_info, object_state.descriptor_sets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate descriptor sets");
	}

	return object_id;
}

void VulkanObjectShader::release_resources(uint32_t object_id)
{
	ObjectShaderObjectState &object_state = object_states_[object_id];

	// Free the descriptor sets of the object state and check the resutl
	if (vkFreeDescriptorSets(context_->logical_device(),
							 local_descriptor_pool_.get(),
							 object_state.descriptor_sets.size(),
							 object_state.descriptor_sets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to free descriptor sets");
	}

	// set generations to an invalid state
	for (VulkanDescriptorState &descriptor_state: object_state.descriptor_states)
	{
		for (uint32_t &generation: descriptor_state.generations)
		{
			generation = std::numeric_limits<uint32_t>::max();
		}
	}


	// TODO: add the object id back into the pool
}

}// namespace flwfrg
