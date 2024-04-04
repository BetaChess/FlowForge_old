#include "pch.hpp"

#include "imgui_instance.hpp"

#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "vulkan_context.hpp"

namespace flwfrg
{


ImGuiInstance::ImGuiInstance(VulkanContext *context)
	: vulkan_context_(context), context_created_(true)
{
	assert(context != nullptr);

	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	
	// Init glfw for imgui
	ImGui_ImplGlfw_InitForVulkan(vulkan_context_->get_window().get_glfw_window_ptr(), true);

	ImGui_ImplVulkan_InitInfo init_info = {};
	vulkan_context_->populate_imgui_init_info(init_info);
	
	ImGui_ImplVulkan_Init(&init_info);
}
ImGuiInstance::~ImGuiInstance()
{
	if (context_created_)
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}
ImGuiInstance::ImGuiInstance(ImGuiInstance &&other) noexcept
	: vulkan_context_(other.vulkan_context_), context_created_(other.context_created_)
{
	other.context_created_ = false;
}
ImGuiInstance &ImGuiInstance::operator=(ImGuiInstance &&other) noexcept
{
	if (this != &other)
	{
		vulkan_context_ = other.vulkan_context_;
		context_created_ = other.context_created_;
		other.context_created_ = false;
	}
	return *this;
}
}// namespace flwfrg