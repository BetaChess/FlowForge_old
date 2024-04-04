#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace flwfrg
{

struct Vertex3d
{
	glm::vec4 position{0};

	static std::vector<VkVertexInputAttributeDescription> get_binding_description()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

		attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex3d, position)});

		return attributeDescriptions;
	}
	
};

}