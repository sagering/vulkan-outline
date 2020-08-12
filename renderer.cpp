#include "renderer.h"

#include <algorithm>
#include <array>
#include <iostream>

#include "vk_init.h"
#include "vk_utils.h"

Renderer::Renderer(VulkanWindow* window)
  : VulkanBase(window)
{
  initialize();
  createDescriptorPool();
  createPipeline();
  createBuffersAndSamplers();
  createDescriptorSets();
}

std::string
LoadFile(const char* _filename)
{
  std::string buff;

  FILE* file = 0;
  fopen_s(&file, _filename, "rb");
  if (file) {
    fseek(file, 0, SEEK_END);
    size_t bytes = ftell(file);

    buff.resize(bytes);

    fseek(file, 0, SEEK_SET);
    fread(&(*buff.begin()), 1, bytes, file);
    fclose(file);
    return buff;
  }

  return buff;
}

VkShaderModule
LoadShaderModule(VkDevice device, const char* filename)
{
  std::string buff = LoadFile(filename);
  auto result =
    vkuCreateShaderModule(device, buff.size(), (uint32_t*)buff.data(), nullptr);
  ASSERT_VK_VALID_HANDLE(result);
  return result;
}

void
Renderer::initialize()
{
  // load shader modules
  fragmentShaderModule = LoadShaderModule(device, "simple.frag.spv");
  vertexShaderModule = LoadShaderModule(device, "simple.vert.spv");

  outlineFragmentShader = LoadShaderModule(device, "outline.frag.spv");
  outlineVertexShader = LoadShaderModule(device, "outline.vert.spv");
}

Renderer::~Renderer()
{
  destroyDescriptorSets();
  destroyBuffersAndSamplers();
  destroyPipeline();
}

void
Renderer::createPipeline()
{
  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask =
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  pipeline =
    GraphicsPipeline::GetBuilder()
      .SetDevice(device)
      .SetVertexShader(vertexShaderModule)
      .SetFragmentShader(fragmentShaderModule)
      .SetVertexBindings({ Vertex::GetBindingDescription() })
      .SetVertexAttributes(Vertex::GetAttributeDescriptions())
      .SetDescriptorSetLayouts(
        { { { 0,
              VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
              1,
              VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },
            { 1,
              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
              1,
              VK_SHADER_STAGE_FRAGMENT_BIT } } })
      .SetViewports({ { 0.0f,
                        0.0f,
                        (float)swapchain->imageExtent.width,
                        (float)swapchain->imageExtent.height,
                        0.0f,
                        1.0f } })
      .SetScissors(
        { { { 0, 0 },
            { swapchain->imageExtent.width, swapchain->imageExtent.height } } })
      .SetColorBlendAttachments({ colorBlendAttachment })
      .SetDepthWriteEnable(VK_TRUE)
      .SetRenderPass(renderPass)
      .Build();

  outlinePipeline =
    GraphicsPipeline::GetBuilder()
      .SetDevice(device)
      .SetVertexShader(outlineVertexShader)
      .SetFragmentShader(outlineFragmentShader)
      .SetVertexBindings({ Vertex::GetBindingDescription() })
      .SetVertexAttributes(Vertex::GetAttributeDescriptions())
      .SetDescriptorSetLayouts(
        { { { 0,
              VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
              1,
              VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },
            { 1,
              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
              1,
              VK_SHADER_STAGE_FRAGMENT_BIT } } })
      .SetViewports({ { 0.0f,
                        0.0f,
                        (float)swapchain->imageExtent.width,
                        (float)swapchain->imageExtent.height,
                        0.0f,
                        1.0f } })
      .SetScissors(
        { { { 0, 0 },
            { swapchain->imageExtent.width, swapchain->imageExtent.height } } })
      .SetColorBlendAttachments({ colorBlendAttachment })
      .SetRenderPass(renderPass)
      .SetDepthWriteEnable(VK_FALSE)
      .Build();
}

void
Renderer::destroyPipeline()
{
  delete pipeline;
  pipeline = nullptr;
}

void
Renderer::createDescriptorPool()
{
  std::vector<VkDescriptorPoolSize> poolSizes = {
    vkiDescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
    vkiDescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
  };
  auto info =
    vkiDescriptorPoolCreateInfo(1, poolSizes.size(), poolSizes.data());
  vkCreateDescriptorPool(device, &info, nullptr, &descriptorPool);
}

void
Renderer::destroyDescriptorPool()
{
  vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void
Renderer::createDescriptorSets()
{
  auto info = vkiDescriptorSetAllocateInfo(
    descriptorPool, 1, pipeline->descriptorSetLayouts.data());
  vkAllocateDescriptorSets(device, &info, &descriptorSet);

  auto bufferInfo = vkiDescriptorBufferInfo(ubo, 0, sizeof(glm::mat4));
  auto write = vkiWriteDescriptorSet(descriptorSet,
                                     0,
                                     0,
                                     1,
                                     VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                     nullptr,
                                     &bufferInfo,
                                     nullptr);

  vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

  auto imageInfo = vkiDescriptorImageInfo(
    textureSampler, textureView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  write = vkiWriteDescriptorSet(descriptorSet,
                                1,
                                0,
                                1,
                                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                &imageInfo,
                                nullptr,
                                nullptr);

  vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}

void
Renderer::destroyDescriptorSets()
{
  vkFreeDescriptorSets(device, descriptorPool, 1, &descriptorSet);
}

void
Renderer::createBuffersAndSamplers()
{
  std::vector<Vertex> vertices = {
    { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } },
    { { 1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } },
    { { -1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },

    { { 1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } },
    { { -1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
    { { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
  };

  vertexCount = vertices.size();
  vertexBufferOffset = 0;

  VkDeviceSize size = vertexCount * sizeof(Vertex);
  vertexBuffer = vkuCreateBuffer(device,
                                 size,
                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                 VK_SHARING_MODE_EXCLUSIVE,
                                 {});
  vertexBufferMemory =
    vkuAllocateBufferMemory(device,
                            physicalDeviceProps.memProps,
                            vertexBuffer,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                            true);
  vkuTransferData(device, vertexBufferMemory, 0, size, vertices.data());

  ubo = vkuCreateBuffer(device,
                        sizeof(Ubo),
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_SHARING_MODE_EXCLUSIVE,
                        {});
  uboMemory = vkuAllocateBufferMemory(device,
                                      physicalDeviceProps.memProps,
                                      ubo,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                      true);

  // Texture

  std::vector<float> checkerboard = {
    0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f, 0.f, 1.f,
    0.f, 1.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f,
    1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f,
    1.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f,
    0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f, 0.f, 1.f,
    0.f, 1.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f,
    1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f,
  };

  VkFormat textureFormat = VK_FORMAT_R32_SFLOAT;
  VkExtent3D textureExtent = { 10, 10, 1 };
  auto textureInfo = vkiImageCreateInfo(VK_IMAGE_TYPE_2D,
                                        textureFormat,
                                        textureExtent,
                                        1,
                                        1,
                                        VK_SAMPLE_COUNT_1_BIT,
                                        VK_IMAGE_TILING_OPTIMAL,
                                        VK_IMAGE_USAGE_SAMPLED_BIT |
                                          VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                        VK_SHARING_MODE_EXCLUSIVE,
                                        VK_QUEUE_FAMILY_IGNORED,
                                        nullptr,
                                        VK_IMAGE_LAYOUT_UNDEFINED);

  texture = VK_NULL_HANDLE;
  ASSERT_VK_SUCCESS(vkCreateImage(device, &textureInfo, nullptr, &texture));

  auto textureViewInfo = vkiImageViewCreateInfo(
    texture,
    VK_IMAGE_VIEW_TYPE_2D,
    textureFormat,
    {},
    vkiImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1));

  textureMemory =
    vkuAllocateImageMemory(device, physicalDeviceProps.memProps, texture, true);

  textureView = VK_NULL_HANDLE;
  ASSERT_VK_SUCCESS(
    vkCreateImageView(device, &textureViewInfo, nullptr, &textureView));

  vkuTransferImageData(device,
                       physicalDeviceProps.memProps,
                       cmdPool,
                       queue,
                       texture,
                       textureFormat,
                       textureExtent,
                       VK_IMAGE_LAYOUT_UNDEFINED,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                       checkerboard.size() * sizeof(checkerboard[0]),
                       checkerboard.data());

  auto samplerInfo = vkiSamplerCreateInfo(VK_FILTER_NEAREST,
                                          VK_FILTER_NEAREST,
                                          VK_SAMPLER_MIPMAP_MODE_LINEAR,
                                          VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                          VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                          VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                          0.f,
                                          VK_FALSE,
                                          0.f,
                                          VK_FALSE,
                                          VK_COMPARE_OP_NEVER,
                                          0.f,
                                          0.f,
                                          VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
                                          VK_FALSE);

  textureSampler = VK_NULL_HANDLE;
  ASSERT_VK_SUCCESS(
    vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler));
}

void
Renderer::destroyBuffersAndSamplers()
{
  vkDestroySampler(device, textureSampler, nullptr);

  vkDestroyImageView(device, textureView, nullptr);
  vkDestroyImage(device, texture, nullptr);
  vkFreeMemory(device, textureMemory, nullptr);

  vkDestroyBuffer(device, ubo, nullptr);
  vkFreeMemory(device, uboMemory, nullptr);

  vkDestroyBuffer(device, vertexBuffer, nullptr);
  vkFreeMemory(device, vertexBufferMemory, nullptr);
}

void
Renderer::recordCommandBuffer(uint32_t idx)
{
  ASSERT_VK_SUCCESS(
    vkWaitForFences(device, 1, &fences[idx], true, (uint64_t)-1));
  ASSERT_VK_SUCCESS(vkResetFences(device, 1, &fences[idx]));
  ASSERT_VK_SUCCESS(vkResetCommandBuffer(commandBuffers[idx], 0));

  VkCommandBufferBeginInfo beginInfo = vkiCommandBufferBeginInfo(nullptr);
  ASSERT_VK_SUCCESS(vkBeginCommandBuffer(commandBuffers[idx], &beginInfo));

  VkClearValue clearValues[] = { { 0.0f, 0.0f, 0.0f, 1.0f }, { 1.f, 0 } };
  VkRenderPassBeginInfo renderPassInfo =
    vkiRenderPassBeginInfo(renderPass,
                           framebuffers[idx],
                           { { 0, 0 }, swapchain->imageExtent },
                           2,
                           clearValues);

  vkCmdBeginRenderPass(
    commandBuffers[idx], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindVertexBuffers(
    commandBuffers[idx], 0, 1, &vertexBuffer, &vertexBufferOffset);
  vkCmdBindDescriptorSets(commandBuffers[idx],
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline->pipelineLayout,
                          0,
                          1,
                          &descriptorSet,
                          0,
                          nullptr);
  vkCmdBindPipeline(commandBuffers[idx],
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    outlinePipeline->pipeline);
  vkCmdDraw(commandBuffers[idx], static_cast<uint32_t>(vertexCount), 1, 0, 0);
  vkCmdBindPipeline(
    commandBuffers[idx], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
  vkCmdDraw(commandBuffers[idx], static_cast<uint32_t>(vertexCount), 1, 0, 0);
  vkCmdEndRenderPass(commandBuffers[idx]);
  ASSERT_VK_SUCCESS(vkEndCommandBuffer(commandBuffers[idx]));
}

void
Renderer::drawFrame(const Ubo& ubo)
{
  uint32_t nextImageIdx = -1;
  ASSERT_VK_SUCCESS(vkAcquireNextImageKHR(device,
                                          swapchain->handle,
                                          UINT64_MAX,
                                          imageAvailableSemaphore,
                                          VK_NULL_HANDLE,
                                          &nextImageIdx));

  recordCommandBuffer(nextImageIdx);
  vkuTransferData(device, uboMemory, 0, sizeof(Ubo), (void*)(&ubo));

  VkPipelineStageFlags waitStages[] = {
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
  };
  VkSubmitInfo submitInfo = vkiSubmitInfo(1,
                                          &imageAvailableSemaphore,
                                          waitStages,
                                          1,
                                          &commandBuffers[nextImageIdx],
                                          1,
                                          &renderFinishedSemaphore);
  ASSERT_VK_SUCCESS(vkQueueSubmit(queue, 1, &submitInfo, fences[nextImageIdx]));

  VkPresentInfoKHR presentInfo = vkiPresentInfoKHR(
    1, &renderFinishedSemaphore, 1, &swapchain->handle, &nextImageIdx, nullptr);
  ASSERT_VK_SUCCESS(vkQueuePresentKHR(queue, &presentInfo));
}

void
Renderer::OnSwapchainReinitialized()
{
  destroyPipeline();
  createPipeline();
}
