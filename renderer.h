#pragma once

#include <glm\glm.hpp>
#include <tuple>

#include "graphics_pipeline.h"
#include "vk_base.h"

struct Vertex
{
  glm::vec3 pos;

  static VkVertexInputBindingDescription GetBindingDescription()
  {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
  }

  static std::vector<VkVertexInputAttributeDescription>
  GetAttributeDescriptions()
  {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(1);

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);
    return attributeDescriptions;
  }
};

struct Renderer : VulkanBase
{
public:
  Renderer(VulkanWindow* window);
  ~Renderer();

  struct Ubo
  {
    glm::mat4 vp;
    glm::mat4 m;
	glm::ivec2 res;
  };

  void drawFrame(const Ubo&);

private:
  virtual void OnSwapchainReinitialized();

  GraphicsPipeline* postPipeline;
  VkShaderModule postVertexShader;
  VkShaderModule postFragmentShader;

  GraphicsPipeline* prePipeline;
  VkShaderModule preVertexShader;
  VkShaderModule preFragmentShader;

  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;

  VkDescriptorPool descriptorPool;
  VkDescriptorSet descriptorSet;

  VkBuffer ubo;
  VkDeviceMemory uboMemory;

  void recordCommandBuffer(uint32_t idx);

private:
  void createResources();
  void destroyResources();
};
