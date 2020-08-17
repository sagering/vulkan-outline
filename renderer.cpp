#include "renderer.h"

#include <algorithm>
#include <array>
#include <iostream>

#include "data.h"
#include "vk_init.h"
#include "vk_utils.h"

Renderer::Renderer(VulkanWindow* window)
  : VulkanBase(window)
{
  createResources();
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
Renderer::createResources()
{
  // shader modules
  preFragmentShader = LoadShaderModule(device, "pre.frag.spv");
  preVertexShader = LoadShaderModule(device, "pre.vert.spv");

  postFragmentShader = LoadShaderModule(device, "post.frag.spv");
  postVertexShader = LoadShaderModule(device, "post.vert.spv");

  // pipelines
  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask =
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  prePipeline =
    GraphicsPipeline::GetBuilder()
      .SetDevice(device)
      .SetVertexShader(preVertexShader)
      .SetFragmentShader(preFragmentShader)
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
      .SetDepthTestEnable(VK_TRUE)
      .SetRenderPass(renderPassPre)
      .Build();

  postPipeline =
    GraphicsPipeline::GetBuilder()
      .SetDevice(device)
      .SetVertexShader(postVertexShader)
      .SetFragmentShader(postFragmentShader)
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
      .SetRenderPass(renderPassPost)
      .SetDepthWriteEnable(VK_FALSE)
      .SetDepthTestEnable(VK_FALSE)
      .Build();

  // vertex buffer
  std::vector<float> floats = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f,
                                -1.0f, 1.0f,  0.0f, 1.0f, -1.0f, 0.0f,
                                -1.0f, 1.0f,  0.0f, 1.0f, 1.0f,  0.0f };

  floats.insert(floats.end(), teapot.begin(), teapot.end());

  VkDeviceSize size = floats.size() * sizeof(float);

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

  vkuTransferData(device, vertexBufferMemory, 0, size, floats.data());

  // ubo
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
  // descriptor pool
  std::vector<VkDescriptorPoolSize> poolSizes = {
    vkiDescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
    vkiDescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
  };
  auto info =
    vkiDescriptorPoolCreateInfo(1, poolSizes.size(), poolSizes.data());
  vkCreateDescriptorPool(device, &info, nullptr, &descriptorPool);

  // descriptor sets
  auto allocInfo = vkiDescriptorSetAllocateInfo(
    descriptorPool, 1, prePipeline->descriptorSetLayouts.data());
  vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);

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

  auto imageInfo =
    vkiDescriptorImageInfo(colorImageSampler,
                           colorImageView,
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

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

Renderer::~Renderer()
{
  vkQueueWaitIdle(queue);
  destroyResources();
}

void
Renderer::destroyResources()
{
  vkDestroyBuffer(device, ubo, nullptr);
  vkFreeMemory(device, uboMemory, nullptr);

  vkDestroyBuffer(device, vertexBuffer, nullptr);
  vkFreeMemory(device, vertexBufferMemory, nullptr);

  vkDestroyDescriptorPool(device, descriptorPool, nullptr);

  // shaders
  vkDestroyShaderModule(device, preFragmentShader, nullptr);
  vkDestroyShaderModule(device, preVertexShader, nullptr);
  vkDestroyShaderModule(device, postFragmentShader, nullptr);
  vkDestroyShaderModule(device, postVertexShader, nullptr);

  // pipelines
  delete prePipeline;
  prePipeline = nullptr;
  delete postPipeline;
  postPipeline = nullptr;
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

  // prepass
  {
    VkClearValue clearValues[] = { { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.f, 0 } };

    VkRenderPassBeginInfo renderPassInfo =
      vkiRenderPassBeginInfo(renderPassPre,
                             framebufferPre,
                             { { 0, 0 }, swapchain->imageExtent },
                             2,
                             clearValues);

    vkCmdBeginRenderPass(
      commandBuffers[idx], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkDeviceSize vbufferOffset = 6 * sizeof(float) * 3;
    vkCmdBindVertexBuffers(
      commandBuffers[idx], 0, 1, &vertexBuffer, &vbufferOffset);

    vkCmdBindDescriptorSets(commandBuffers[idx],
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            prePipeline->pipelineLayout,
                            0,
                            1,
                            &descriptorSet,
                            0,
                            nullptr);

    vkCmdBindPipeline(commandBuffers[idx],
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      prePipeline->pipeline);

    vkCmdDraw(commandBuffers[idx], teapot.size() / 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffers[idx]);
  }

  // postpass
  {
    VkClearValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f }; // dummy, not used
    VkRenderPassBeginInfo renderPassInfo =
      vkiRenderPassBeginInfo(renderPassPost,
                             framebuffersPost[idx],
                             { { 0, 0 }, swapchain->imageExtent },
                             1,
                             &clearValue);

    vkCmdBeginRenderPass(
      commandBuffers[idx], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkDeviceSize vbufferOffset = 0;
    vkCmdBindVertexBuffers(
      commandBuffers[idx], 0, 1, &vertexBuffer, &vbufferOffset);

    vkCmdBindDescriptorSets(commandBuffers[idx],
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            postPipeline->pipelineLayout,
                            0,
                            1,
                            &descriptorSet,
                            0,
                            nullptr);

    vkCmdBindPipeline(commandBuffers[idx],
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      postPipeline->pipeline);

    vkCmdDraw(commandBuffers[idx], 6, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffers[idx]);
  }

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
  destroyResources();
  createResources();
}
