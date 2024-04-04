#include "pch.hpp"

#include "pipeline.hpp"

#include "../command_buffer.hpp"
#include "renderer/vulkan/vulkan_context.hpp"
#include "vertex.hpp"

#include <array>

namespace aito
{
VulkanPipeline::~VulkanPipeline()
{
	if (handle_ != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(context_->logical_device(), handle_, nullptr);
	}
	if (pipeline_layout_ != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(context_->logical_device(), pipeline_layout_, nullptr);
	}
}
VulkanPipeline::VulkanPipeline(VulkanPipeline &&other) noexcept
	: context_(other.context_),
	  handle_(other.handle_),
	  pipeline_layout_(other.pipeline_layout_)
{
	other.handle_ = VK_NULL_HANDLE;
	other.pipeline_layout_ = VK_NULL_HANDLE;
}

std::optional<VulkanPipeline> VulkanPipeline::create_pipeline(VulkanContext *context, const VulkanRenderpass &renderpass, const std::vector<VkVertexInputAttributeDescription> &attributes, const std::vector<VkDescriptorSetLayout> &descriptor_set_layouts, const std::vector<VkPipelineShaderStageCreateInfo> &stages, VkViewport viewport, VkRect2D scissor, bool is_wireframe)
{
	assert(context != nullptr);
	
	VulkanPipeline return_pipeline{context};

	// Viewport state
	VkPipelineViewportStateCreateInfo viewport_state{};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor;

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = is_wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	// Multisampling
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	// Depth and stencil
	VkPipelineDepthStencilStateCreateInfo depth_stencil{};
	depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil.depthTestEnable = VK_TRUE;
	depth_stencil.depthWriteEnable = VK_TRUE;
	depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_stencil.depthBoundsTestEnable = VK_FALSE;
	depth_stencil.stencilTestEnable = VK_FALSE;

	// Color blend attachment state
	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_TRUE;
	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

	// Color blend state create info
	VkPipelineColorBlendStateCreateInfo color_blending{};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.logicOpEnable = VK_FALSE;
	color_blending.logicOp = VK_LOGIC_OP_COPY;
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = &color_blend_attachment;

	// Dynamic state
	std::array<VkDynamicState, 3> dynamic_states = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_LINE_WIDTH};

	// Dynamic state create info
	VkPipelineDynamicStateCreateInfo dynamic_state{};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
	dynamic_state.pDynamicStates = dynamic_states.data();

	// Vertex input
	VkVertexInputBindingDescription vertex_binding_description{};
	vertex_binding_description.binding = 0;
	vertex_binding_description.stride = sizeof(Vertex3d);
	vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	// Attributes
	VkPipelineVertexInputStateCreateInfo vertex_input_info{};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &vertex_binding_description;
	vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
	vertex_input_info.pVertexAttributeDescriptions = attributes.data();

	// Input assembly
	VkPipelineInputAssemblyStateCreateInfo input_assembly{};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	// Pipeline layout
	VkPipelineLayoutCreateInfo pipeline_layout_info{};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	// Push constants
	VkPushConstantRange push_constant_range;
	push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	push_constant_range.offset = sizeof(glm::mat4) * 0;
	push_constant_range.size = sizeof(glm::mat4) * 2;
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pPushConstantRanges = &push_constant_range;

	// Descriptor set layouts
	pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size());
	pipeline_layout_info.pSetLayouts = descriptor_set_layouts.data();

	// Create the pipeline layout
	if (vkCreatePipelineLayout(context->logical_device(), &pipeline_layout_info, nullptr, &return_pipeline.pipeline_layout_) != VK_SUCCESS)
	{
		AITO_ERROR("Failed to create pipeline layout");
		return std::nullopt;
	}

	// Create the pipeline
	VkGraphicsPipelineCreateInfo pipeline_info{};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = static_cast<uint32_t>(stages.size());
	pipeline_info.pStages = stages.data();
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly;

	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pDepthStencilState = &depth_stencil;
	pipeline_info.pColorBlendState = &color_blending;
	pipeline_info.pDynamicState = &dynamic_state;
	pipeline_info.pTessellationState = nullptr;

	pipeline_info.layout = return_pipeline.pipeline_layout_;

	pipeline_info.renderPass = renderpass.get_handle();
	pipeline_info.subpass = 0;
	pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
	pipeline_info.basePipelineIndex = -1;

	// Create the pipeline
	if (vkCreateGraphicsPipelines(context->logical_device(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &return_pipeline.handle_) != VK_SUCCESS)
	{
		AITO_ERROR("Failed to create graphics pipeline");
		return std::nullopt;
	}

	return return_pipeline;
}

void VulkanPipeline::bind(VulkanCommandBuffer &command_buffer, VkPipelineBindPoint bind_point) const
{
	vkCmdBindPipeline(command_buffer.get_handle(), bind_point, handle_);
}
VulkanPipeline::VulkanPipeline(VulkanContext *context)
	: context_(context)
{
}

}// namespace aito