#pragma once

#include <imgui_impl_vulkan.h>

namespace aito
{
class VulkanContext;

class ImGuiInstance
{
public:
	ImGuiInstance() = default;
	explicit ImGuiInstance(VulkanContext *context);
	~ImGuiInstance();

	// Not copyable but movable
	ImGuiInstance(const ImGuiInstance &) = delete;
	ImGuiInstance &operator=(const ImGuiInstance &) = delete;
	ImGuiInstance(ImGuiInstance &&other) noexcept;
	ImGuiInstance &operator=(ImGuiInstance &&other) noexcept;


private:
	VulkanContext *vulkan_context_ = nullptr;

	bool context_created_ = false;
};

}// namespace aito