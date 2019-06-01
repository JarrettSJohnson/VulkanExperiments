/*
* UI overlay class using ImGui
*
* Copyright (C) 2017 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <vector>

#include <vulkan/vulkan.h>
#include "Buffer.hpp"
#include "Device.hpp"

#include <imgui.h>
#include <glm/glm.hpp>

class UIOverlay 
{
public:
  UIOverlay(Device& paramDevice);
	Device* device;
	vk::Queue queue;

	vk::SampleCountFlagBits rasterizationSamples = vk::SampleCountFlagBits::e1;
	uint32_t subpass{};

	Buffer vertexBuffer;
	Buffer indexBuffer;
	int32_t vertexCount{};
	int32_t indexCount{};

	vk::UniqueDescriptorPool descriptorPool;
	vk::UniqueDescriptorSetLayout descriptorSetLayout;
	vk::DescriptorSet descriptorSet;
	vk::UniquePipelineLayout pipelineLayout;
	vk::UniquePipeline pipeline;

	vk::UniqueDeviceMemory fontMemory;
	vk::UniqueImage fontImage{};
	vk::UniqueImageView fontView{};
	vk::UniqueSampler sampler{};

	struct PushConstBlock {
		glm::vec2 scale;
		glm::vec2 translate;
	};
    PushConstBlock pushConstBlock{};
	bool visible{};
	bool updated{};
	float scale{1.0f};

	void preparePipeline(const vk::RenderPass renderPass);
	void prepareResources();

	bool update();
        void draw(std::uint32_t width, std::uint32_t height,
            const vk::CommandBuffer commandBuffer);
	void resize(uint32_t width, uint32_t height);

	void freeResources();

	bool header(const char* caption);
	bool checkBox(const char* caption, bool* value);
	bool checkBox(const char* caption, int32_t* value);
	bool inputFloat(const char* caption, float* value, float step, uint32_t precision);
	bool sliderFloat(const char* caption, float* value, float min, float max);
	bool sliderInt(const char* caption, int32_t* value, int32_t min, int32_t max);
	bool comboBox(const char* caption, int32_t* itemindex, std::vector<std::string> items);
	bool button(const char* caption);
	void text(const char* formatstr, ...);
};
